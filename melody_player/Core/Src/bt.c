/*
 * bt.c
 *
 *  Created on: Jan 30, 2026
 *      Author: AndriiLobach
 */

#include "bt.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* ---------- buffer settings ---------- */
#define BT_RX_RING_SIZE 256
#define BT_LINE_SIZE    64

static UART_HandleTypeDef *s_huart = NULL;
static bt_context_t *s_ctx = NULL;

static volatile uint8_t s_new_cmd = 0;

/* ring buffer */
static volatile uint8_t  s_ring[BT_RX_RING_SIZE];
static volatile uint16_t s_w = 0;
static volatile uint16_t s_r = 0;

static uint8_t s_rx_byte = 0;

/* currently typed line */
static char s_line[BT_LINE_SIZE];
static uint16_t s_line_len = 0;

/* ---------- local functions ---------- */

/* Case-insensitive string compare (portable, avoids strcasecmp dependency) */
static int bt_stricmp(const char *a, const char *b)
{
  if (a == NULL || b == NULL) return (a == b) ? 0 : 1;

  while (*a && *b) {
    char ca = (char)toupper((unsigned char)*a);
    char cb = (char)toupper((unsigned char)*b);
    if (ca != cb) return (int)((unsigned char)ca - (unsigned char)cb);
    a++;
    b++;
  }
  return (int)((unsigned char)*a - (unsigned char)*b);
}

static void bt_uart_send(const char *s)
{
  if (!s_huart || !s) return;
  HAL_UART_Transmit(s_huart, (uint8_t*)s, (uint16_t)strlen(s), 200);
}

static void ring_push(uint8_t b)
{
  uint16_t next = (uint16_t)((s_w + 1) % BT_RX_RING_SIZE);
  if (next == s_r) {
    /* overflow: drop byte */
    return;
  }
  s_ring[s_w] = b;
  s_w = next;
}

static int ring_pop(uint8_t *out)
{
  if (s_r == s_w) return 0; /* empty */
  *out = s_ring[s_r];
  s_r = (uint16_t)((s_r + 1) % BT_RX_RING_SIZE);
  return 1;
}

static void str_trim(char *s)
{
  if (!s) return;

  /* trim left */
  while (*s && isspace((unsigned char)*s)) {
    memmove(s, s + 1, strlen(s)); /* shift left by one */
  }

  /* trim right */
  size_t n = strlen(s);
  while (n > 0 && isspace((unsigned char)s[n - 1])) {
    s[n - 1] = '\0';
    n--;
  }
}

static void bt_reply_ok(void)  { bt_uart_send("OK\r\n"); }

static void bt_reply_err(const char *msg)
{
    if (msg) {
        bt_uart_send("ERR: ");
        bt_uart_send(msg);
        bt_uart_send("\r\n");
    } else {
        bt_uart_send("ERR\r\n");
    }
}

static void bt_print_help(void)
{
    bt_uart_send("\r\n--- Supported Commands ---\r\n");
    bt_uart_send("START      : Start playing\r\n");
    bt_uart_send("STOP       : Stop playing\r\n");
    bt_uart_send("SET <n>    : Select melody (0-6)\r\n");
    bt_uart_send("MODE <n>   : Select LED mode (0-3)\r\n");
    bt_uart_send("STATUS     : Show current state\r\n");
    bt_uart_send("HELP or ?  : Show this menu\r\n");
}

void bt_send_status(void)
{
  if (!s_ctx) return;
  if (s_ctx->state == BT_STATE_PLAYING) bt_uart_send("PLAYING\r\n");
  else                                 bt_uart_send("STOPPED\r\n");
}

static int bt_stricmp_prefix(const char *line, const char *prefix)
{
    return strncasecmp(line, prefix, strlen(prefix));
}

/* Parser for one complete line */
static void bt_parse_line(char *line)
{
    str_trim(line);
    if (line[0] == '\0') return;

    // 1. HELP / ?
    if (bt_stricmp(line, "HELP") == 0 || strcmp(line, "?") == 0) {
        bt_print_help();
        return;
    }

    // 2. START
    if (bt_stricmp(line, "START") == 0) {
        s_ctx->state = BT_STATE_PLAYING;
        bt_reply_ok();
        s_new_cmd = 1;
        return;
    }

    // 3. STOP
    if (bt_stricmp(line, "STOP") == 0) {
        s_ctx->state = BT_STATE_STOPPED;
        bt_reply_ok();
        s_new_cmd = 1;
        return;
    }

    // 4. SET <n> Validation
    if (bt_stricmp_prefix(line, "SET") == 0) { // Check if it starts with SET
        unsigned int n;
        if (sscanf(line, "SET %u", &n) == 1) {
            if (n <= 255) {
                s_ctx->melody_id = (uint8_t)n;
                bt_reply_ok();
                s_new_cmd = 1;
            } else {
                bt_reply_err("Melody ID out of range (0-6)");
            }
        } else {
            bt_reply_err("Usage: SET <number>");
        }
        return;
    }

    // 5. MODE <n> Validation
    if (bt_stricmp_prefix(line, "MODE") == 0) {
        unsigned int n;
        if (sscanf(line, "MODE %u", &n) == 1) {
            if (n <= 3) { // Assuming modes 0-3
                s_ctx->led_mode = (uint8_t)n;
                bt_reply_ok();
                s_new_cmd = 1;
            } else {
                bt_reply_err("Mode out of range (0-3)");
            }
        } else {
            bt_reply_err("Usage: MODE <number>");
        }
        return;
    }

    // 6. Unknown Command - The Catch-All
    char err_buf[64];
    snprintf(err_buf, sizeof(err_buf), "Unknown command '%s'. Type HELP for info.", line);
    bt_reply_err(err_buf);
}

void bt_init(UART_HandleTypeDef *huart, bt_context_t *ctx)
{
  s_huart = huart;
  s_ctx = ctx;

  if (s_ctx) {
    s_ctx->state = BT_STATE_STOPPED;
    s_ctx->melody_id = 0;
    s_ctx->led_mode = 0;
  }

  /* start receiving 1 byte via interrupt */
  HAL_UART_Receive_IT(s_huart, &s_rx_byte, 1);

  bt_uart_send("BT READY\r\n");
  bt_send_status();
}

void bt_process_rx(void)
{
  uint8_t b;

  while (ring_pop(&b)) {

    /* end of line */
    if (b == '\r' || b == '\n') {
      if (s_line_len > 0) {
        s_line[s_line_len] = '\0';
        bt_parse_line(s_line);
        s_line_len = 0;
      }
      continue;
    }

    /* accumulate line */
    if (s_line_len < (BT_LINE_SIZE - 1)) {
      s_line[s_line_len++] = (char)b;
    } else {
      /* line is too long — reset */
      s_line_len = 0;
      bt_reply_err("Line too long");
    }
  }
}



uint8_t bt_has_new_command(void) { return s_new_cmd; }
void bt_clear_new_command_flag(void) { s_new_cmd = 0; }
bt_context_t* bt_get_context(void) { return s_ctx; }

/* ---------- HAL callback ---------- */
/* This callback is called when a byte is received */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (s_huart && huart->Instance == s_huart->Instance) {
    ring_push(s_rx_byte);

    /* restart reception of the next byte */
    HAL_UART_Receive_IT(s_huart, &s_rx_byte, 1);
  }
}
