//
// Created by zhu_y on 2026/5/29.
//
#include "main.h"
#include "s800_i2c.h"
#include "flash_led.h"

void SysTick_Handler(void) {
    time_flag_1ms = true;

    if (delay_time != STOP_SCROLL_TIME_MS) {
        scroll_tick++;
        pf0_tick++;

        if (scroll_tick >= delay_time) {
            MoveWindow();
            scroll_tick = 0;
            I2C0_WriteByte(PCA9557_I2CADDR, PCA9557_OUTPUT, (uint8_t) (~(1 << (window_pos % SEG7_DIGITS))));
        }

        if (pf0_tick >= delay_time) {
            pf0_tick = 0;
            pf0_on = !pf0_on;
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, pf0_on ? GPIO_PIN_0 : 0);
        }
    } else if (pf0_on || pf0_tick != 0) {
        pf0_tick = 0;
        pf0_on = false;
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0);
    }

    uint8_t char_pos = (window_pos + index) % str_len;

    I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_OUTPUT_PORT2, 0x00);
    // Delay(1);
    I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_OUTPUT_PORT1, Seg7Code_FromAscii(str_buffer[char_pos]));

    I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_OUTPUT_PORT2, (uint8_t) (1 << index));

    index++;

    if (index >= SEG7_DIGITS) {
        index = 0;
    }
}
