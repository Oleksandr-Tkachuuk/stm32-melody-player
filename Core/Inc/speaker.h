/*
 * speaker.h
 *
 *  Created on: Jan 29, 2026
 *      Author: mirom
 */

#ifndef INC_SPEAKER_H_
#define INC_SPEAKER_H_

#include <stdint.h>
#include "stm32f3xx_hal.h"

// pass TIM initialized in CubeMX
void Speaker_Init(TIM_HandleTypeDef *htim, uint32_t channel);

// plays a tone with a frequency freq_hz and with a 0-100 volume
void Speaker_Set_Tone(uint32_t freq_hz, uint8_t volume);

#endif /* INC_SPEAKER_H_ */
