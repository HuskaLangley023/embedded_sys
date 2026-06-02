//
// Created by zhu_y on 2026/5/29.
//
#include "flash_led.h"

#define PF0_UART_SUCCESS_INTERVAL_MS 80U
#define PF0_UART_SUCCESS_TOGGLE_COUNT 4U
#define PF0_UART_ERROR_INTERVAL_MS 500U

#define PF0_UART_IDLE 0U
#define PF0_UART_SUCCESS 1U
#define PF0_UART_ERROR 2U

volatile uint8_t index = 0;

uint8_t str_buffer[DISPLAY_BUFFER_MAX_LEN + 1U] = "s523010910148zhuyixiao";
volatile uint8_t str_len = sizeof("s523010910148zhuyixiao") - 1U;
volatile int8_t dir = 1;
volatile uint8_t window_pos = 0;

volatile uint32_t delay_time = FAST_SCROLL_TIME_MS;
volatile uint32_t scroll_tick = 0;
volatile uint32_t pf0_tick = 0;
volatile bool pf0_on = false;

enum SPEEDLEVEL speed_level = FAST;

static volatile uint8_t pf0_uart_state = PF0_UART_IDLE;
static volatile uint8_t pf0_uart_success_toggle_count = 0;

static void PF0_Write(bool on) {
    pf0_on = on;
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, on ? GPIO_PIN_0 : 0);
}

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
    PF0_Write(false);
}

bool SetDisplayText(const char *text) {
    uint32_t len = 0;

    while (text[len] != '\0') {
        len++;
        if (len > SEG7_DIGITS || len > DISPLAY_BUFFER_MAX_LEN) {
            return false;
        }
    }

    if (len == 0U) {
        return false;
    }

    SysTickIntDisable();
    memcpy(str_buffer, text, len);
    str_buffer[len] = '\0';
    str_len = (uint8_t) len;
    window_pos = 0;
    index = 0;
    SysTickIntEnable();

    return true;
}

void SetScrollDelayMs(uint32_t delay_ms) {
    delay_time = delay_ms;

    if (delay_ms == FAST_SCROLL_TIME_MS) {
        speed_level = FAST;
    } else if (delay_ms == SLOW_SCROLL_TIME_MS) {
        speed_level = SLOW;
    } else if (delay_ms == STOP_SCROLL_TIME_MS) {
        speed_level = STOP;
    }

    scroll_tick = 0;
    pf0_tick = 0;
    PF0_Write(false);
}

void PF0_UpdateLocalMode(void) {
    if (delay_time != STOP_SCROLL_TIME_MS) {
        pf0_tick++;
        if (pf0_tick >= delay_time) {
            pf0_tick = 0;
            PF0_Write(!pf0_on);
        }
    } else if (pf0_on || pf0_tick != 0) {
        pf0_tick = 0;
        PF0_Write(false);
    }
}

void PF0_UpdateUartMode(void) {
    if (pf0_uart_state == PF0_UART_SUCCESS) {
        pf0_tick++;
        if (pf0_tick >= PF0_UART_SUCCESS_INTERVAL_MS) {
            pf0_tick = 0;
            pf0_uart_success_toggle_count++;
            PF0_Write(!pf0_on);

            if (pf0_uart_success_toggle_count >= PF0_UART_SUCCESS_TOGGLE_COUNT) {
                pf0_uart_state = PF0_UART_IDLE;
                pf0_uart_success_toggle_count = 0;
                PF0_Write(false);
            }
        }
    } else if (pf0_uart_state == PF0_UART_ERROR) {
        pf0_tick++;
        if (pf0_tick >= PF0_UART_ERROR_INTERVAL_MS) {
            pf0_tick = 0;
            PF0_Write(!pf0_on);
        }
    } else if (pf0_on || pf0_tick != 0) {
        pf0_tick = 0;
        PF0_Write(false);
    }
}

void PF0_ResetLocalMode(void) {
    pf0_tick = 0;
    PF0_Write(false);
}

void PF0_ResetUartMode(void) {
    pf0_uart_state = PF0_UART_IDLE;
    pf0_uart_success_toggle_count = 0;
    pf0_tick = 0;
    PF0_Write(false);
}

void PF0_RecordUartCommandSuccess(void) {
    pf0_uart_state = PF0_UART_SUCCESS;
    pf0_uart_success_toggle_count = 0;
    pf0_tick = 0;
    PF0_Write(true);
}

void PF0_RecordUartCommandError(void) {
    pf0_uart_state = PF0_UART_ERROR;
    pf0_uart_success_toggle_count = 0;
    pf0_tick = 0;
    PF0_Write(true);
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
        case 'A':
        case 'a':
            return 0x77;
        case 'B':
        case 'b':
            return 0x7c;
        case 'C':
        case 'c':
            return 0x39;
        case 'D':
        case 'd':
            return 0x5e;
        case 'E':
        case 'e':
            return 0x79;
        case 'F':
        case 'f':
            return 0x71;
        case 'H':
        case 'h':
            return 0x74;
        case 'I':
        case 'i':
            return 0x04;
        case 'L':
        case 'l':
            return 0x38;
        case 'O':
        case 'o':
            return 0x5c;
        case 'S':
        case 's':
            return 0x6d; // close to 5
        case 'U':
        case 'u':
            return 0x1c;
        case 'X':
        case 'x':
            return 0x76; // close to H
        case 'Y':
        case 'y':
            return 0x6e;
        case 'Z':
        case 'z':
            return 0x5b; // close to 2
        case '-':
            return 0x40;
        case '_':
            return 0x08;
        case ' ':
            return 0x00;
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
