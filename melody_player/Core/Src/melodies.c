/*
 * melodies.c
 *
 *  Created on: Jan 30, 2026
 *      Author: mirom
 */


#include "melodies.h"

/* Common rhythm helpers (durations in ms):
 *  80  = very short (staccato)
 * 120  = short
 * 160  = 1/8-ish
 * 240  = 1/4-ish
 * 320  = long
 */

#define P NOTE_PAUSE_HZ

/* -------- Melody 0: "Overworld Loop" (bright arpeggios) -------- */
static const melody_step_t melody0[] = {
  {NOTE_C4_HZ,120},{P,40},{NOTE_E4_HZ,120},{P,40},{NOTE_G4_HZ,160},{P,40},{NOTE_C5_HZ,240},{P,80},
  {NOTE_B4_HZ,120},{P,40},{NOTE_G4_HZ,120},{P,40},{NOTE_E4_HZ,160},{P,40},{NOTE_G4_HZ,240},{P,80},

  {NOTE_D4_HZ,120},{P,40},{NOTE_FS4_HZ,120},{P,40},{NOTE_A4_HZ,160},{P,40},{NOTE_D5_HZ,240},{P,80},
  {NOTE_C5_HZ,120},{P,40},{NOTE_A4_HZ,120},{P,40},{NOTE_FS4_HZ,160},{P,40},{NOTE_A4_HZ,240},{P,80},

  {NOTE_E4_HZ,120},{P,40},{NOTE_G4_HZ,120},{P,40},{NOTE_B4_HZ,160},{P,40},{NOTE_E5_HZ,240},{P,80},
  {NOTE_D5_HZ,120},{P,40},{NOTE_B4_HZ,120},{P,40},{NOTE_G4_HZ,160},{P,40},{NOTE_B4_HZ,240},{P,120},
};

/* -------- Melody 1: "Boss Alert" (tense, repeating motif) -------- */
static const melody_step_t melody1[] = {
  {NOTE_E4_HZ,160},{NOTE_DS4_HZ,160},{NOTE_E4_HZ,160},{NOTE_DS4_HZ,160},{NOTE_E4_HZ,240},{P,80},
  {NOTE_B3_HZ,160},{NOTE_C4_HZ,160},{NOTE_CS4_HZ,160},{NOTE_D4_HZ,160},{NOTE_DS4_HZ,240},{P,80},

  {NOTE_E4_HZ,120},{P,40},{NOTE_E4_HZ,120},{P,40},{NOTE_E4_HZ,120},{P,40},{NOTE_B4_HZ,240},{P,80},
  {NOTE_AS4_HZ,120},{P,40},{NOTE_A4_HZ,120},{P,40},{NOTE_GS4_HZ,120},{P,40},{NOTE_A4_HZ,240},{P,120},

  {NOTE_E4_HZ,120},{P,40},{NOTE_FS4_HZ,120},{P,40},{NOTE_G4_HZ,120},{P,40},{NOTE_A4_HZ,240},{P,80},
  {NOTE_G4_HZ,120},{P,40},{NOTE_FS4_HZ,120},{P,40},{NOTE_E4_HZ,120},{P,40},{NOTE_DS4_HZ,240},{P,160},
};

/* -------- Melody 2: "Victory Jingle" (short but busy) -------- */
static const melody_step_t melody2[] = {
  {NOTE_C5_HZ,120},{NOTE_E5_HZ,120},{NOTE_G5_HZ,160},{P,40},{NOTE_G5_HZ,120},{P,40},{NOTE_E5_HZ,160},{P,40},
  {NOTE_D5_HZ,120},{NOTE_F5_HZ,120},{NOTE_A4_HZ,160},{P,40},{NOTE_A4_HZ,120},{P,40},{NOTE_F5_HZ,160},{P,40},
  {NOTE_E5_HZ,120},{NOTE_G5_HZ,120},{NOTE_C5_HZ,240},{P,80},{NOTE_B4_HZ,120},{NOTE_G4_HZ,120},{NOTE_E4_HZ,320},{P,160},
};

/* -------- Melody 3: "Dungeon Walk" (minor-ish, stepping bass) -------- */
static const melody_step_t melody3[] = {
  {NOTE_A3_HZ,240},{P,40},{NOTE_E4_HZ,160},{P,40},{NOTE_A4_HZ,160},{P,40},{NOTE_G4_HZ,160},{P,40},
  {NOTE_A3_HZ,240},{P,40},{NOTE_E4_HZ,160},{P,40},{NOTE_A4_HZ,160},{P,40},{NOTE_G4_HZ,160},{P,80},

  {NOTE_F4_HZ,160},{P,40},{NOTE_E4_HZ,160},{P,40},{NOTE_DS4_HZ,160},{P,40},{NOTE_E4_HZ,240},{P,80},
  {NOTE_A3_HZ,240},{P,40},{NOTE_E4_HZ,160},{P,40},{NOTE_A4_HZ,160},{P,40},{NOTE_G4_HZ,160},{P,120},

  {NOTE_C4_HZ,160},{P,40},{NOTE_E4_HZ,160},{P,40},{NOTE_G4_HZ,160},{P,40},{NOTE_B4_HZ,240},{P,80},
  {NOTE_A4_HZ,160},{P,40},{NOTE_G4_HZ,160},{P,40},{NOTE_E4_HZ,240},{P,160},
};

/* -------- Melody 4: "Menu/Select" (chippy, syncopated) -------- */
static const melody_step_t melody4[] = {
  {NOTE_G4_HZ,120},{P,40},{NOTE_B4_HZ,120},{P,40},{NOTE_D5_HZ,160},{P,40},{NOTE_G5_HZ,200},{P,80},
  {NOTE_FS5_HZ,120},{P,40},{NOTE_E5_HZ,120},{P,40},{NOTE_D5_HZ,160},{P,40},{NOTE_B4_HZ,200},{P,80},

  {NOTE_A4_HZ,120},{P,40},{NOTE_CS5_HZ,120},{P,40},{NOTE_E5_HZ,160},{P,40},{NOTE_A5_HZ,200},{P,80}, /* A5 not defined; keep within range by using G5 instead */
};

/* Fix: keep within defined notes (<=800 Hz). Replace the A5 line with G5. */
static const melody_step_t melody4_fixed[] = {
  {NOTE_G4_HZ,120},{P,40},{NOTE_B4_HZ,120},{P,40},{NOTE_D5_HZ,160},{P,40},{NOTE_G5_HZ,200},{P,80},
  {NOTE_FS5_HZ,120},{P,40},{NOTE_E5_HZ,120},{P,40},{NOTE_D5_HZ,160},{P,40},{NOTE_B4_HZ,200},{P,80},

  {NOTE_A4_HZ,120},{P,40},{NOTE_CS5_HZ,120},{P,40},{NOTE_E5_HZ,160},{P,40},{NOTE_G5_HZ,200},{P,80},
  {NOTE_FS5_HZ,120},{P,40},{NOTE_E5_HZ,120},{P,40},{NOTE_CS5_HZ,160},{P,40},{NOTE_A4_HZ,240},{P,160},
};

/* -------- Melody table -------- */
const melody_t g_melodies[] = {
  { melody0,       (uint16_t)(sizeof(melody0)       / sizeof(melody0[0])) },
  { melody1,       (uint16_t)(sizeof(melody1)       / sizeof(melody1[0])) },
  { melody2,       (uint16_t)(sizeof(melody2)       / sizeof(melody2[0])) },
  { melody3,       (uint16_t)(sizeof(melody3)       / sizeof(melody3[0])) },
  { melody4_fixed, (uint16_t)(sizeof(melody4_fixed) / sizeof(melody4_fixed[0])) },
};

const uint8_t MELODY_COUNT =
  (uint8_t)(sizeof(g_melodies) / sizeof(g_melodies[0]));