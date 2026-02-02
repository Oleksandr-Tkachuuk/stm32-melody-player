/*
 * melodies.h
 *
 *  Created on: Jan 30, 2026
 *      Author: mirom
 */

#ifndef INC_MELODIES_H_
#define INC_MELODIES_H_

#include <stdint.h>
#include "stm32f3xx_hal.h"


#define NOTE_PAUSE_HZ  0u

/* 3rd octave (subset) */
#define NOTE_A3_HZ     220u
#define NOTE_AS3_HZ    233u
#define NOTE_B3_HZ     247u

/* 4th octave */
#define NOTE_C4_HZ     262u
#define NOTE_CS4_HZ    277u
#define NOTE_D4_HZ     294u
#define NOTE_DS4_HZ    311u
#define NOTE_E4_HZ     330u
#define NOTE_F4_HZ     349u
#define NOTE_FS4_HZ    370u
#define NOTE_G4_HZ     392u
#define NOTE_GS4_HZ    415u
#define NOTE_A4_HZ     440u
#define NOTE_AS4_HZ    466u
#define NOTE_B4_HZ     494u

/* 5th octave (limited to <= 800 Hz) */
#define NOTE_C5_HZ     523u
#define NOTE_CS5_HZ    554u
#define NOTE_D5_HZ     587u
#define NOTE_DS5_HZ    622u
#define NOTE_E5_HZ     659u
#define NOTE_F5_HZ     698u
#define NOTE_FS5_HZ    740u
#define NOTE_G5_HZ     784u
#define NOTE_A5_HZ     880u
#define NOTE_DS5_HZ    622u
#define NOTE_GS5_HZ    831u



/* One tone step */
typedef struct {
  uint16_t freq_hz;   // 0 = pause
  uint16_t dur_ms;    // duration in milliseconds
} melody_step_t;

/* One melody */
typedef struct {
  const melody_step_t *steps;
  uint16_t length;
} melody_t;

/* Public access */
extern const melody_t g_melodies[];
extern const uint8_t  MELODY_COUNT;


#endif /* INC_MELODIES_H_ */