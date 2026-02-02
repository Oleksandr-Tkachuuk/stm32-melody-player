#include "player.h"
#include "speaker.h"
#include "ws2812.h"
#include "melodies.h"
#include "font8x8.h"
#include <string.h>


/* Private Types */
typedef struct {
    uint8_t  melody_id;
    uint16_t step_index;
    uint32_t step_time_left_ms;
    uint16_t current_freq;
} player_ctx_t;

/* Private Variables (Moved from main.c) */
static system_state_t system_state = SYS_STOPPED;
static player_ctx_t player = {0};
static uint8_t hills[8][8];
static uint32_t mode3_timer_ms = 0;
static uint8_t  mode3_letter_idx = 0;
static uint8_t  lazy_tail[MAX_LED];

static const uint8_t* mode3_letters[3] = { font_U, font_R, font_K };

/* --- Private Internal Functions (Add 'static') --- */

static uint8_t height_from_freq(uint16_t freq) {
    if (freq == 0) return 0;
    if (freq < 300) return 2;
    if (freq < 400) return 3;
    if (freq < 500) return 4;
    if (freq < 600) return 5;
    if (freq < 700) return 6;
    return 7;
}

static void note_color_from_freq(uint16_t freq,
                                 uint8_t *r,
                                 uint8_t *g,
                                 uint8_t *b)
{
    if (freq == 0) {
        *r = *g = *b = 0;
        return;
    }

    if (freq < 300)      { *r = 255; *g = 0;   *b = 0;   }
    else if (freq < 350) { *r = 255; *g = 127; *b = 0;   }
    else if (freq < 400) { *r = 255; *g = 255; *b = 0;   }
    else if (freq < 450) { *r = 0;   *g = 255; *b = 0;   }
    else if (freq < 550) { *r = 0;   *g = 0;   *b = 255; }
    else                 { *r = 75;  *g = 0;   *b = 130; }
}


static void color_from_height_xy(uint8_t y, uint8_t x,
                                 uint8_t *r, uint8_t *g, uint8_t *b)
{
    // y: 0 (low) ... 7 (high)

    // vertical gradient
    // yellow -> orange -> red
    const uint8_t r0 = 255, g0 = 255, b0 = 0;    // bottom (yellow)
    const uint8_t r1 = 255, g1 = 140, b1 = 0;    // mid (orange)
    const uint8_t r2 = 255, g2 = 0,   b2 = 0;    // upper (red)

    uint8_t ty = (y * 255) / 7;

    uint8_t br, bg, bb;

    if (ty < 128) {
        // bottom -> mid
        uint8_t t = ty * 2;
        br = r0 + ((r1 - r0) * t >> 8);
        bg = g0 + ((g1 - g0) * t >> 8);
        bb = b0 + ((b1 - b0) * t >> 8);
    } else {
        // mid -> up
        uint8_t t = (ty - 128) * 2;
        br = r1 + ((r2 - r1) * t >> 8);
        bg = g1 + ((g2 - g1) * t >> 8);
        bb = b1 + ((b2 - b1) * t >> 8);
    }

    //Light brightness (not color!)
    uint8_t lum = 170 + (x * 80) / 7;   // 170..250

    *r = (br * lum) >> 8;
    *g = (bg * lum) >> 8;
    *b = (bb * lum) >> 8;
}


static void led_mode_letters_urk(uint16_t freq)
{
    uint8_t r, g, b;
    note_color_from_freq(freq, &r, &g, &b);

    // draw current letter
    Draw_Bitmap(mode3_letters[mode3_letter_idx], r, g, b);
}



static int scheduler_ready_for_step(void)
{
    if (system_state != SYS_PLAYING)
        return 0;

    if (player.step_time_left_ms > 0) {
        player.step_time_left_ms--;
        return 0;
    }

    return 1;
}

static const melody_step_t* get_current_step(void)
{
    const melody_t *m = &g_melodies[player.melody_id];

    if (player.step_index >= m->length) {
        player.step_index = 0;
        return NULL;
    }

    return &m->steps[player.step_index];
}

static void process_audio(const melody_step_t *step)
{
    player.current_freq = step->freq_hz;
    Speaker_Set_Tone(step->freq_hz, 80);
}

static uint8_t approach(uint8_t current, uint8_t target)
{
    if (current < target) return current + 1;
    if (current > target) return current - 1;
    return current;
}

static void led_mode_lazy_tail(uint16_t freq)
{
    uint8_t r, g, b;
    note_color_from_freq(freq, &r, &g, &b);

    // move tail
    for (int i = MAX_LED - 1; i > 0; i--) {
        lazy_tail[i] = lazy_tail[i - 1];
    }

    // head
    lazy_tail[0] = 255;

    WS2812_Clear();

    // draw tail
    for (int i = 0; i < MAX_LED; i++) {

        if (lazy_tail[i] == 0)
            continue;

        uint8_t brightness;

        if (i == 0)      brightness = 100;
        else if (i == 1) brightness = 60;
        else if (i == 2) brightness = 35;
        else if (i == 3) brightness = 20;
        else             brightness = 0;

        if (brightness == 0)
            lazy_tail[i] = 0;
        else
            lazy_tail[i] = (lazy_tail[i] * brightness) / 100;

        Set_LED(
            i,
            (r * lazy_tail[i]) >> 8,
            (g * lazy_tail[i]) >> 8,
            (b * lazy_tail[i]) >> 8
        );
    }

    WS2812_Send();
}




static void process_led(uint8_t mode)
{
    // ===== MODE 0 =====
    if (mode == 0) {
        WS2812_ShowNoteColor(player.current_freq);
    }

    // ===== MODE 1 =====
    else if (mode == 1) {

        // move right
        for (int y = 0; y < 8; y++) {
            for (int x = 7; x > 0; x--) {
                hills[y][x] = hills[y][x - 1];
            }
            hills[y][0] = 0;
        }

        // new column
        uint8_t h = height_from_freq(player.current_freq);
        for (int y = 0; y < h; y++) {
            hills[7 - y][0] = 1;
        }

        // mode 0 color
        uint8_t r, g, b;
        note_color_from_freq(player.current_freq, &r, &g, &b);

        // draw only active pixels
        WS2812_Clear();

        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                if (hills[y][x]) {
                    Set_LED_XY(x, y, r, g, b);
                }
            }
        }


        WS2812_Send();
    }
    // ===== MODE 2 =====
    else if (mode == 2) {

        // move to right
        for (int y = 0; y < 8; y++) {
            for (int x = 7; x > 0; x--) {
                hills[y][x] = hills[y][x - 1];
            }
            hills[y][0] = 0;
        }

        // new col
        uint8_t h = height_from_freq(player.current_freq);
        for (int y = 0; y < h; y++) {
            hills[7 - y][0] = 255;
        }

        // vertical gradient
        WS2812_Clear();

        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                if (hills[y][x]) {
                    uint8_t r, g, b;
                    uint8_t gy = 7 - y;
                    color_from_height_xy(gy, x, &r, &g, &b);
                    Set_LED_XY(x, y, r, g, b);
                }
            }
        }

        WS2812_Send();
    }

    //  MODE 3 (URK)
    else if (mode == 3) {
        led_mode_letters_urk(player.current_freq);
    }

   /* else if (mode == 4) {
        led_mode_lazy_tail(player.current_freq);
    }*/
}

static void finish_step(const melody_step_t *step)
{
    player.step_time_left_ms = step->dur_ms;
    player.step_index++;
}

void Player_Init(void) {
    system_state = SYS_STOPPED;
    memset(hills, 0, sizeof(hills));
    memset(lazy_tail, 0, sizeof(lazy_tail));
}

void Player_Start(uint8_t melody_id) {
    system_state = SYS_PLAYING;
    player.melody_id = melody_id % MELODY_COUNT;
    player.step_index = 0;
    player.step_time_left_ms = 0;
    memset(hills, 0, sizeof(hills));
    mode3_timer_ms = 0;
    mode3_letter_idx = 0;
}

void Player_Stop(void) {
    system_state = SYS_STOPPED;
    Speaker_Set_Tone(0, 0);
    player.current_freq = 0;
    WS2812_Clear();
    WS2812_Send();
}

void scheduler_tick_1ms(void)
{
  //  MODE 3 letter timer
  if (system_state == SYS_PLAYING && bt_ctx.led_mode == 3) {
      mode3_timer_ms++;
      if (mode3_timer_ms >= 1000) {   // 1 sek
          mode3_timer_ms = 0;
          mode3_letter_idx++;
          if (mode3_letter_idx >= 3)
              mode3_letter_idx = 0;
      }
  }

    //
    if (system_state == SYS_PLAYING) {

        if (bt_ctx.led_mode == 4) {

            led_mode_lazy_tail(player.current_freq);
        }
    }


    if (!scheduler_ready_for_step())
        return;

    const melody_step_t *step = get_current_step();
    if (!step)
        return;

    process_audio(step);

    // all modes only by freq
    if (bt_ctx.led_mode != 4) {
        process_led(bt_ctx.led_mode);
    }

    finish_step(step);
}
