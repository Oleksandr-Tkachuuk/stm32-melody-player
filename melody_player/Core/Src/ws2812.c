/*
 * ws2812.c
 *
 * Created on: Jan 30, 2026
 * Author: tkachuk
 */

#include "ws2812.h"

#if USE_BRIGHTNESS
	static uint8_t GlobalBrightness = 255; // 0..255
#endif

static inline uint8_t Apply_Brightness(uint8_t color)
{
#if USE_BRIGHTNESS
	return (uint8_t)((color * GlobalBrightness) >> 8); // /256 ≈ /255
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

// --- Variables ---
// External timer handle declared in main.c
extern TIM_HandleTypeDef htim1;

// LED Data Buffer: [LED_ID][Green, Red, Blue]
uint8_t LED_Data[MAX_LED][4];

// Raw PWM Data Buffer: 24 bits per LED + 50 reset pulses
// 64 LEDs * 24 bits = 1536 + 50 = ~1586 uint16_t values
uint16_t pwmData[(24 * MAX_LED) + 50];

// Flag to handle DMA state
volatile int datasentflag = 0;

// --- Functions ---

void WS2812_Init(void)
{
    // Initialize all LEDs to off
    WS2812_Clear();
    WS2812_Send();
}

void Set_LED(int LEDnum, int Red, int Green, int Blue)
{
    if(LEDnum >= MAX_LED) return; // Safety check

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
        // Pack GRB data
    	uint8_t g = Apply_Brightness(LED_Data[i][1]);
    	uint8_t r = Apply_Brightness(LED_Data[i][2]);
    	uint8_t b = Apply_Brightness(LED_Data[i][3]);

    	color = ((g << 16) | (r << 8) | b);

        // Iterate through 24 bits (Green -> Red -> Blue)
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
    // Note: We cast pwmData to uint32_t* because HAL expects it,
    // even though we are sending 16-bit values (DMA set to Half-Word usually)
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
    // SAFETY CHECK: Keep within 8x8 bounds
    if (x < 0 || x > 7 || y < 0 || y > 7) return;

    // CALCULATE INDEX
    // Standard Progressive: Row 0 is 0-7, Row 1 is 8-15
    int index = (y * 8) + x;

    // NOTE: If your matrix is "Zig-Zag" (Snake), uncomment the lines below:
//    if (y % 2 != 0) {
//        // On odd rows (1, 3, 5...), count backwards
//        index = (y * 8) + (7 - x);
//    }

    Set_LED(index, r, g, b);
}

// Draw a simple 8x8 bitmap
// bitmap: an array of 8 bytes. Each byte is a row.
void Draw_Bitmap(const uint8_t bitmap[8], int r, int g, int b)
{
    WS2812_Clear(); // Clear background first

    for (int y = 0; y < 8; y++)     // Iterate Rows
    {
        for (int x = 0; x < 8; x++) // Iterate Columns
        {
            // Check if the bit at position 'x' is set in row 'y'
            // We shift 1 to the left by (7-x) because bit 7 is usually the first pixel (left)
            if (bitmap[y] & (1 << (7 - x)))
            {
                Set_LED_XY(x, y, r, g, b);
            }
        }
    }
    WS2812_Send(); // Push to LEDs immediately
}

// --- Interrupt Callback ---
// This function is called automatically by HAL when DMA finishes
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM1) // Ensure it's the correct timer
    {
        HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
        datasentflag = 1;
    }
}
