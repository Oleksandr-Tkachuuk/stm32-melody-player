/*
 * bt.h
 *
 *  Created on: Jan 30, 2026
 *      Author: AndriiLobach
 */

#ifndef __BT_H__
#define __BT_H__

#include "stm32f3xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Playback state controlled via BT commands */
typedef enum
{
  BT_STATE_STOPPED = 0,
  BT_STATE_PLAYING = 1
} bt_state_t;

/* Shared BT state (read by other modules) */
typedef struct
{
  bt_state_t state;      /* Current state: STOPPED/PLAYING */
  uint8_t    melody_id;  /* Selected melody index (0..N) */
  uint8_t    led_mode;   /* Selected LED mode (0..N) */
} bt_context_t;

/**
 * @brief Initialize BT module.
 * @param huart Pointer to UART handle used for HC-05/HC-06 (e.g., &huart2).
 * @param ctx   Pointer to context structure (must remain valid).
 */
void bt_init(UART_HandleTypeDef *huart, bt_context_t *ctx);

/**
 * @brief Process received bytes from internal ring buffer.
 *        Call this frequently from the main loop.
 */
void bt_process_rx(void);

/**
 * @brief Send current status line ("PLAYING" or "STOPPED") via UART.
 */
void bt_send_status(void);

/**
 * @brief Returns 1 if a valid command was parsed since last clear.
 */
uint8_t bt_has_new_command(void);

/**
 * @brief Clear "new command" flag (call after you handled the command).
 */
void bt_clear_new_command_flag(void);

/**
 * @brief Get pointer to current BT context.
 * @return Pointer to context passed in bt_init().
 */
bt_context_t* bt_get_context(void);

#ifdef __cplusplus
}
#endif

#endif /* __BT_H__ */
