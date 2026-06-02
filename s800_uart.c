//
// Created by zhu_y on 2026/5/29.
//
#include "s800_uart.h"
#include "flash_led.h"
#include "hw_ints.h"

#include <stdio.h>

#define UART_CMD_MAX_LEN 64U
#define UART_RESPONSE_MAX_LEN 96U
#define UART_SPEED_MAX_MS 9999U

static volatile char uart_cmd_buf[UART_CMD_MAX_LEN + 1U];
static volatile uint32_t uart_cmd_len = 0;
static volatile bool uart_cmd_ready = false;
static volatile bool uart_cmd_overflow = false;
static volatile bool uart_cmd_overflow_ready = false;

static uint8_t uart_mode = UART_MODE_LOCAL;

static bool AsciiIsSpace(char ch)
{
    return (ch == ' ') || (ch == '\t');
}

static bool AsciiIsDigit(char ch)
{
    return (ch >= '0') && (ch <= '9');
}

static char AsciiToUpper(char ch)
{
    if ((ch >= 'a') && (ch <= 'z')) {
        return (char)(ch - 'a' + 'A');
    }
    return ch;
}

static const char *SkipSpaces(const char *text)
{
    while (AsciiIsSpace(*text)) {
        text++;
    }
    return text;
}

static bool OnlySpacesLeft(const char *text)
{
    text = SkipSpaces(text);
    return *text == '\0';
}

static bool EqualsIgnoreCase(const char *left, const char *right)
{
    while ((*left != '\0') && (*right != '\0')) {
        if (AsciiToUpper(*left) != AsciiToUpper(*right)) {
            return false;
        }
        left++;
        right++;
    }
    return (*left == '\0') && (*right == '\0');
}

static bool StartsWithIgnoreCase(const char *text, const char *prefix)
{
    while (*prefix != '\0') {
        if (AsciiToUpper(*text) != AsciiToUpper(*prefix)) {
            return false;
        }
        text++;
        prefix++;
    }
    return true;
}

static bool FetchCommand(char *command, uint32_t command_size, bool *overflow)
{
    bool ready;
    uint32_t len;
    uint32_t i;

    if (!uart_cmd_ready) {
        return false;
    }

    IntDisable(INT_UART0);

    ready = uart_cmd_ready;
    if (!ready) {
        IntEnable(INT_UART0);
        return false;
    }

    *overflow = uart_cmd_overflow_ready;
    len = uart_cmd_len;
    if (len >= command_size) {
        len = command_size - 1U;
    }

    for (i = 0; i < len; i++) {
        command[i] = uart_cmd_buf[i];
    }
    command[len] = '\0';

    uart_cmd_len = 0;
    uart_cmd_ready = false;
    uart_cmd_overflow_ready = false;

    IntEnable(INT_UART0);
    return true;
}

static bool ParseModeParam(const char *param, uint8_t *mode)
{
    param = SkipSpaces(param);

    if (((param[0] == '0') || (param[0] == '1')) && OnlySpacesLeft(param + 1)) {
        *mode = (uint8_t)(param[0] - '0');
        return true;
    }
    return false;
}

static bool ParseSpeedMs(const char *param, uint32_t *speed_ms)
{
    uint32_t int_part = 0;
    uint32_t frac_part = 0;
    uint32_t frac_scale = 1;
    uint8_t frac_digits = 0;
    bool has_digit = false;

    param = SkipSpaces(param);

    while (AsciiIsDigit(*param)) {
        has_digit = true;
        int_part = int_part * 10U + (uint32_t)(*param - '0');
        if (int_part > (UART_SPEED_MAX_MS / 1000U)) {
            return false;
        }
        param++;
    }

    if (*param == '.') {
        param++;
        while (AsciiIsDigit(*param)) {
            has_digit = true;
            if (frac_digits >= 3U) {
                return false;
            }
            frac_part = frac_part * 10U + (uint32_t)(*param - '0');
            frac_scale *= 10U;
            frac_digits++;
            param++;
        }
    }

    if (!has_digit || !OnlySpacesLeft(param)) {
        return false;
    }

    while (frac_scale < 1000U) {
        frac_part *= 10U;
        frac_scale *= 10U;
    }

    *speed_ms = int_part * 1000U + frac_part;
    return *speed_ms <= UART_SPEED_MAX_MS;
}

static bool ValidateDisplayText(const char *text)
{
    uint32_t len = 0;

    while (text[len] != '\0') {
        len++;
        if (len > SEG7_DIGITS) {
            return false;
        }
    }

    return len > 0U;
}

static void FormatSpeed(char *buf, uint32_t buf_size)
{
    uint32_t ms = delay_time;
    uint32_t seconds = ms / 1000U;
    uint32_t fraction = ms % 1000U;
    char frac_buf[4];
    int i;

    if (fraction == 0U) {
        snprintf(buf, buf_size, "%lu", (unsigned long)seconds);
        return;
    }

    snprintf(frac_buf, sizeof(frac_buf), "%03lu", (unsigned long)fraction);
    for (i = 2; i > 0; i--) {
        if (frac_buf[i] != '0') {
            break;
        }
        frac_buf[i] = '\0';
    }
    snprintf(buf, buf_size, "%lu.%s", (unsigned long)seconds, frac_buf);
}

static void SendLine(const char *message)
{
    UARTStringPut(message);
    UARTStringPut("\r\n");
}

static void SendStatus(void)
{
    char speed[16];
    char response[UART_RESPONSE_MAX_LEN];

    FormatSpeed(speed, sizeof(speed));
    snprintf(response, sizeof(response), "STATUS:MODE=%u,SPEED=%s",
             (unsigned int)uart_mode, speed);
    SendLine(response);
}

static void SendSpeed(void)
{
    char speed[16];
    char response[UART_RESPONSE_MAX_LEN];

    FormatSpeed(speed, sizeof(speed));
    snprintf(response, sizeof(response), "SPEED:%s", speed);
    SendLine(response);
}

static void ExecuteCommand(const char *command)
{
    if (EqualsIgnoreCase(command, "GET STATUS")) {
        SendStatus();
        return;
    }

    if (EqualsIgnoreCase(command, "GET SPEED")) {
        SendSpeed();
        return;
    }

    if (StartsWithIgnoreCase(command, "SET MODE")) {
        uint8_t mode;

        if (!AsciiIsSpace(command[8])) {
            SendLine("ERROR: INVALID PARAM");
            return;
        }

        if (!ParseModeParam(command + 8, &mode)) {
            SendLine("ERROR: INVALID PARAM");
            return;
        }

        uart_mode = mode;
        SendStatus();
        return;
    }

    if (StartsWithIgnoreCase(command, "SET DISP")) {
        const char *text = SkipSpaces(command + 8);

        if (!AsciiIsSpace(command[8])) {
            SendLine("ERROR: INVALID PARAM");
            return;
        }

        if (uart_mode != UART_MODE_CONTROL) {
            SendLine("ERROR: NOT IN UART MODE");
            return;
        }

        if (!ValidateDisplayText(text)) {
            SendLine("ERROR: INVALID PARAM");
            return;
        }

        SetDisplayText(text);
        SendLine("OK");
        return;
    }

    if (StartsWithIgnoreCase(command, "SET SPEED")) {
        uint32_t speed_ms;

        if (!AsciiIsSpace(command[9])) {
            SendLine("ERROR: INVALID PARAM");
            return;
        }

        if (uart_mode != UART_MODE_CONTROL) {
            SendLine("ERROR: NOT IN UART MODE");
            return;
        }

        if (!ParseSpeedMs(command + 9, &speed_ms)) {
            SendLine("ERROR: INVALID PARAM");
            return;
        }

        SetScrollDelayMs(speed_ms);
        SendSpeed();
        return;
    }

    SendLine("ERROR: INVALID COMMAND");
}

void UARTStringPut(const char *cMessage)
{
    while(*cMessage!='\0')
        UARTCharPut(UART0_BASE,*(cMessage++));
}

void UARTStringPutNonBlocking(const char *cMessage)
{
    while(*cMessage!='\0')// 字符'\0'是C语言我i字符串自动添加的结束字符，ASCII码为00H。
        UARTCharPutNonBlocking(UART0_BASE,*(cMessage++));
}

void UARTCommand_RxByte(uint8_t byte)
{
    char ch = (char)byte;

    if ((ch == '\r') || (ch == '\n')) {
        return;
    }

    if (uart_cmd_ready) {
        return;
    }

    if (uart_cmd_overflow) {
        if (ch == '#') {
            uart_cmd_overflow = false;
            uart_cmd_overflow_ready = true;
            uart_cmd_ready = true;
        }
        return;
    }

    if (ch == '#') {
        uart_cmd_buf[uart_cmd_len] = '\0';
        uart_cmd_ready = true;
        return;
    }

    if (uart_cmd_len >= UART_CMD_MAX_LEN) {
        uart_cmd_len = 0;
        uart_cmd_overflow = true;
        return;
    }

    uart_cmd_buf[uart_cmd_len++] = ch;
}

void UARTCommand_Process(void)
{
    char command[UART_CMD_MAX_LEN + 1U];
    bool overflow;

    if (!FetchCommand(command, sizeof(command), &overflow)) {
        return;
    }

    if (overflow) {
        SendLine("ERROR: BUFFER OVERFLOW");
        return;
    }

    ExecuteCommand(command);
}

uint8_t UARTCommand_GetMode(void)
{
    return uart_mode;
}

void S800_UART_Init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);						//Enable PortA
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));			//Wait for the GPIO moduleA ready

    GPIOPinConfigure(GPIO_PA0_U0RX);												// Set GPIO A0 and A1 as UART pins.
    GPIOPinConfigure(GPIO_PA1_U0TX);

    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Configure the UART character format: 115,200, 8-N-1. FIFO触发水平为8个字节（16字节*4/8=8字节）。
    UARTConfigSetExpClk(UART0_BASE, ui32SysClock,115200,(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |UART_CONFIG_PAR_NONE));


    UARTStringPut("\r\nHello, world!\r\n");
}
