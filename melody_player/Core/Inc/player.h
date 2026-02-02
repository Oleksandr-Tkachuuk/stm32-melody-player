#ifndef PLAYER_H
#define PLAYER_H

#include "main.h"
#include "melodies.h"
#include "bt.h"

typedef enum {
    SYS_STOPPED = 0,
    SYS_PLAYING
} system_state_t;

void Player_Init(void);
void Player_Start(uint8_t melody_id);
void Player_Stop(void);
void scheduler_tick_1ms(void);

extern bt_context_t bt_ctx;

#endif
