//
// Created by zhu_y on 2026/5/29.
//

#ifndef MAIN_H
#define MAIN_H

#include <stdbool.h>
#include <stdint.h>
#include "debug.h"
#include "gpio.h"
#include "hw_memmap.h"
#include "hw_types.h"
#include "interrupt.h"
#include "pin_map.h"
#include "string.h"
#include "sysctl.h"
#include "systick.h"

#define FAST_SCROLL_TIME_MS 200U
#define SLOW_SCROLL_TIME_MS 800U
#define STOP_SCROLL_TIME_MS 0U
#define SEG7_DIGITS 8U

extern uint32_t ui32SysClock;
extern volatile bool time_flag_1ms;

void DelayMs(uint32_t ms);
uint32_t SystemClock_PLL(void);
void S800_SysTick_Init(void);
void S800_GPIO_Init(void);

#endif // MAIN_H
