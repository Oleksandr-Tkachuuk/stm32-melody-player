/*
 * speaker.c
 *
 *  Created on: Jan 29, 2026
 *      Author: mirom
 */


#include "speaker.h"

static TIM_HandleTypeDef *s_htim = NULL;
static uint32_t s_channel = 0;

static const uint32_t TIM_CLK_HZ = 8000000UL;

void Speaker_Init(TIM_HandleTypeDef *htim, uint32_t channel){

	s_htim = htim;
	s_channel = channel;

	HAL_TIM_PWM_Start(s_htim, s_channel);
	__HAL_TIM_SET_COMPARE(s_htim, s_channel, 0);
}

void Speaker_Set_Tone(uint32_t freq_hz, uint8_t volume){

	if (s_htim == NULL) return;

    if (freq_hz == 0 || volume == 0)
    {
        __HAL_TIM_SET_COMPARE(s_htim, s_channel, 0);
        return;
    }

    if (volume > 100) volume = 100;

    uint32_t arr = (TIM_CLK_HZ / freq_hz);
    if (arr < 2) arr = 2;
    arr -= 1;

    __HAL_TIM_SET_AUTORELOAD(s_htim, arr);
    __HAL_TIM_SET_COUNTER(s_htim, 0);

    uint32_t ccr = ((arr + 1) * volume) / 100;
    __HAL_TIM_SET_COMPARE(s_htim, s_channel, ccr);
}
