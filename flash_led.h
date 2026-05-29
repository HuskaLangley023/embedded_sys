//
// Created by zhu_y on 2026/5/29.
//

#ifndef FLASH_LED_H
#define FLASH_LED_H

#include "main.h"

extern uint32_t pj0_val;
extern uint32_t pj1_val;

extern volatile uint8_t index;

extern uint8_t str_buffer[];
extern uint8_t str_len;
extern volatile int8_t dir;
extern volatile uint8_t window_pos;

extern volatile uint32_t delay_time;
extern volatile uint32_t scroll_tick;
extern volatile uint32_t pf0_tick;
extern volatile bool pf0_on;

enum SPEEDLEVEL { FAST, STOP, SLOW };

extern enum SPEEDLEVEL speed_level;

bool is_key_Pressed(uint32_t ui32Port, uint8_t ui8Pins);
void LED_Flash(enum SPEEDLEVEL speed_level);
void SetScrollSpeed(enum SPEEDLEVEL level);
void MoveWindow(void);
uint8_t Seg7Code_FromAscii(char ch);

#endif // FLASH_LED_H
