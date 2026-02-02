/*
 * ws2812.c
 *
 * Created on: Jan 30, 2026
 * Author: tkachuk
 */

#include "ws2812.h"
#include "melodies.h"

#if USE_BRIGHTNESS
	static uint8_t GlobalBrightness = 255;
#endif

static inline uint8_t Apply_Brightness(uint8_t color)
{
#if USE_BRIGHTNESS
	return (uint8_t)((color * GlobalBrightness) >> 8);
#else
	return color;
#endif
}

void WS2812_SetBrightness(uint8_t brightness)
{
#if USE_BRIGHTNESS
	GlobalBrightness = brightness;
#endif
}

uint8_t WS2812_GetBrightness(void)
{
#if USE_BRIGHTNESS
	return GlobalBrightness;
#else
	return 255;
#endif
}

extern TIM_HandleTypeDef htim1;

uint8_t LED_Data[MAX_LED][4];

// Raw PWM Data Buffer: 24 bits per LED + 50 reset pulses
// 64 LEDs * 24 bits = 1536 + 50 = ~1586 uint16_t values
uint16_t pwmData[(24 * MAX_LED) + 50];

volatile int datasentflag = 0;

// Map: Frequency -> RGB
const NoteColorMap_t NoteColors[] = {
    {NOTE_C4_HZ, 255, 0, 0},    // C = Red
    {NOTE_D4_HZ, 255, 127, 0},  // D = Orange
    {NOTE_E4_HZ, 255, 255, 0},  // E = Yellow
    {NOTE_F4_HZ, 0, 255, 0},    // F = Green
    {NOTE_G4_HZ, 0, 0, 255},    // G = Blue
    {NOTE_A4_HZ, 75, 0, 130},   // A = Indigo
    {NOTE_B4_HZ, 143, 0, 255},  // B = Violet
    {NOTE_C5_HZ, 255, 0, 0},    // C5 = Red (Higher)
    {NOTE_D5_HZ, 255, 127, 0},
    {NOTE_E5_HZ, 255, 255, 0},
    {NOTE_F5_HZ, 0, 255, 0},
    {NOTE_G5_HZ, 0, 0, 255},
    {0, 0, 0, 0}                // Pause/Silence = Off
};

void WS2812_Init(void)
{
    WS2812_Clear();
    WS2812_Send();
}

void Set_LED(int LEDnum, int Red, int Green, int Blue)
{
    if(LEDnum >= MAX_LED) return;

    LED_Data[LEDnum][0] = LEDnum;
    LED_Data[LEDnum][1] = Green;
    LED_Data[LEDnum][2] = Red;
    LED_Data[LEDnum][3] = Blue;
}

void WS2812_Send(void)
{
    uint32_t indx = 0;
    uint32_t color;

    // 1. Fill pwmData buffer based on LED_Data
    for (int i = 0; i < MAX_LED; i++)
    {
    	uint8_t g = Apply_Brightness(LED_Data[i][1]);
    	uint8_t r = Apply_Brightness(LED_Data[i][2]);
    	uint8_t b = Apply_Brightness(LED_Data[i][3]);

    	color = ((g << 16) | (r << 8) | b);

        for (int b = 23; b >= 0; b--)
        {
            if (color & (1 << b))
            {
                // Bit 1: ~2/3 duty cycle (58/89)
                pwmData[indx] = 58;
            }
            else
            {
                // Bit 0: ~1/3 duty cycle (29/89)
                pwmData[indx] = 29;
            }
            indx++;
        }
    }

    // 2. Add Reset Signal (Low Voltage > 50us)
    for (int i = 0; i < 50; i++)
    {
        pwmData[indx] = 0;
        indx++;
    }

    // 3. Start DMA Transfer
    HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *)pwmData, indx);

    // 4. Wait for transfer to complete
    while (!datasentflag) {};
    datasentflag = 0;
}

void WS2812_Clear(void)
{
    for (int i = 0; i < MAX_LED; i++)
    {
        Set_LED(i, 0, 0, 0);
    }
}

// Set a pixel using (x, y) coordinates where (0,0) is top-left
void Set_LED_XY(int x, int y, int r, int g, int b)
{
    if (x < 0 || x > 7 || y < 0 || y > 7) return;

    int index = (y * 8) + x;

    Set_LED(index, r, g, b);
}

void Draw_Bitmap(const uint8_t bitmap[8], int r, int g, int b)
{
    WS2812_Clear();

    for (int y = 0; y < 8; y++)
    {
        for (int x = 0; x < 8; x++)
        {
            if (bitmap[y] & (1 << (7 - x)))
            {
                Set_LED_XY(x, y, r, g, b);
            }
        }
    }
    WS2812_Send();
}

void WS2812_ShowNoteColor(uint16_t freq) {
    if (freq == 0) {
        WS2812_Clear();
        WS2812_Send();
        return;
    }

    for (int i = 0; i < (sizeof(NoteColors) / sizeof(NoteColorMap_t)); i++) {
        if (freq >= NoteColors[i].freq - 2 && freq <= NoteColors[i].freq + 2) {
            for (int led = 0; led < MAX_LED; led++) {
                Set_LED(led, NoteColors[i].r, NoteColors[i].g, NoteColors[i].b);
            }
            WS2812_Send();
            return;
        }
    }
}

// Interrupt сallback
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM1)
    {
        HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
        datasentflag = 1;
    }
}
