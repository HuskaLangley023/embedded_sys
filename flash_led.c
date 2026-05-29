//
// Created by zhu_y on 2026/5/29.
//
#include "flash_led.h"

uint32_t pj0_val = 0;
uint32_t pj1_val = 0;

volatile uint8_t index = 0;

uint8_t str_buffer[] = "s523010910148zhuyixiao";
uint8_t str_len = sizeof(str_buffer) - 1U;
volatile int8_t dir = 1;
volatile uint8_t window_pos = 0;

volatile uint32_t delay_time = FAST_SCROLL_TIME_MS;
volatile uint32_t scroll_tick = 0;
volatile uint32_t pf0_tick = 0;
volatile bool pf0_on = false;

enum SPEEDLEVEL speed_level = FAST;

bool is_key_Pressed(uint32_t ui32Port, uint8_t ui8Pins) {
    if (GPIOPinRead(ui32Port, ui8Pins) == 0) {
        DelayMs(10); // ������ʱ

        if (GPIOPinRead(ui32Port, ui8Pins) == 0) {
            // �ȴ����֣�����һ�ΰ��´������
            while (GPIOPinRead(ui32Port, ui8Pins) == 0) {
            }
            DelayMs(10);
            return true;
        }
    }
    return false;
}

void LED_Flash(enum SPEEDLEVEL speed_level) {
    SetScrollSpeed(speed_level);

    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 1);
    DelayMs(delay_time);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0);
    DelayMs(delay_time);
}

void SetScrollSpeed(enum SPEEDLEVEL level) {
    switch (level) {
        case FAST:
            delay_time = FAST_SCROLL_TIME_MS;
            break;
        case SLOW:
            delay_time = SLOW_SCROLL_TIME_MS;
            break;
        case STOP:
            delay_time = STOP_SCROLL_TIME_MS;
            break;
    }

    scroll_tick = 0;
    pf0_tick = 0;
    pf0_on = false;
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0);
}

uint8_t Seg7Code_FromAscii(char ch) {
    switch (ch) {
        case '0':
            return 0x3f; // 0b00111111
        case '1':
            return 0x06; // 0b00000110
        case '2':
            return 0x5b; // 0b01011011
        case '3':
            return 0x4f; // 0b01001111
        case '4':
            return 0x66; // 0b01100110
        case '5':
            return 0x6d; // 0b01101101
        case '6':
            return 0x7d; // 0b01111101
        case '7':
            return 0x07; // 0b00000111
        case '8':
            return 0x7f; // 0b01111111
        case '9':
            return 0x6f; // 0b01101111
        case 'z':
            return 0x5b; // close to 2
        case 'h':
            return 0x74;
        case 'u':
            return 0x1c;
        case 'y':
            return 0x6e;
        case 'i':
            return 0x04;
        case 'x':
            return 0x76; // close to H
        case 'a':
            return 0x77;
        case 'o':
            return 0x5c;
        case 's':
            return 0x6d; // close to 5
        default:
            return 0x00; // ����ʾ
    }
}

void MoveWindow(void) {
    if (dir > 0) {
        window_pos++;

        if (window_pos >= str_len) {
            window_pos = 0;
        }
    } else {
        if (window_pos == 0) {
            window_pos = str_len - 1U;
        } else {
            window_pos--;
        }
    }
}
