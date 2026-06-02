//
// Created by zhu_y on 2026/5/29.
//
#include "control_loop.h"
#include "main.h"
#include "flash_led.h"
#include "s800_uart.h"

void main_control_loop(void) {
    if (time_flag_1ms) {
        time_flag_1ms = false;

        if (UARTCommand_GetMode() != UART_MODE_LOCAL) {
            return;
        }

        // read keys
        if (is_key_Pressed(GPIO_PORTJ_BASE, GPIO_PIN_0)) { // sw1
            dir = -dir;
        }
        if (is_key_Pressed(GPIO_PORTJ_BASE, GPIO_PIN_1)) { // sw2
            switch (speed_level) {
                case FAST:
                    speed_level = SLOW;
                    break;
                case SLOW:
                    speed_level = STOP;
                    break;
                case STOP:
                    speed_level = FAST;
                    break;
            }
            SetScrollSpeed(speed_level);
        }
    }
}
