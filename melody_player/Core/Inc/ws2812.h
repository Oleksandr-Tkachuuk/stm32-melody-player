/*
 * ws2812.h
 *
 * Created on: Jan 30, 2026
 * Author: tkachuk
 */

#ifndef INC_WS2812_H_
#define INC_WS2812_H_

#include "main.h"

#define MAX_LED 64
#define USE_BRIGHTNESS 1 // 0 = false, 1 = true

typedef struct {
    uint16_t freq;
    uint8_t r;
    uint8_t g;
    uint8_t b;
} NoteColorMap_t;

// --- Function Prototypes ---
void WS2812_Init(void);

void Set_LED(int LEDnum, int Red, int Green, int Blue);

void WS2812_Send(void);

void WS2812_Clear(void);

// Helper to set a specific pixel by coordinates (0-7, 0-7)
void Set_LED_XY(int x, int y, int r, int g, int b);

// Function to find the color for a given frequency
void WS2812_ShowNoteColor(uint16_t freq);

// Function to draw a predefined pattern (character)
void Draw_Bitmap(const uint8_t bitmap[8], int r, int g, int b);

void WS2812_SetBrightness(uint8_t brightness);

uint8_t WS2812_GetBrightness(void);

#endif /* INC_WS2812_H_ */
