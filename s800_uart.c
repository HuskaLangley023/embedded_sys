//
// Created by zhu_y on 2026/5/29.
//
#include "s800_uart.h"
#include "crc.h"
#include "flash_led.h"
#include "hw_ints.h"

#define UART_CMD_MAX_LEN 64U
#define UART_SPEED_MAX_MS 9999U
// #define UART_HISTORY_DEPTH 10U
// #define UART_HISTORY_CMD_LEN UART_CMD_MAX_LEN
// #define UART_CRC_HEX_LEN 4U

static volatile char uart_cmd_buf[UART_CMD_MAX_LEN + 1U];
static volatile uint32_t uart_cmd_len = 0;
static volatile bool uart_cmd_ready = false;
static volatile bool uart_cmd_overflow = false;
static volatile bool uart_cmd_overflow_ready = false;

static volatile uint8_t uart_mode = UART_MODE_LOCAL;

// static char uart_history[UART_HISTORY_DEPTH][UART_HISTORY_CMD_LEN + 1U];
// static uint8_t uart_history_count = 0;
// static uint8_t uart_history_next = 0;
// static uint8_t uart_history_browse_offset = 0;
//
// static void SendLine(const char *message);

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

static void UInt32ToDec(uint32_t value, char *buf, uint32_t buf_size)
{
    uint32_t divisor;
    uint32_t pos;
    bool started;
    uint8_t digit;

    if (buf_size == 0U) {
        return;
    }

    pos = 0;
    if (value == 0U) {
        if (buf_size > 1U) {
            buf[pos++] = '0';
        }
        buf[pos] = '\0';
        return;
    }

    started = false;
    divisor = 1000000000UL;
    while (divisor > 0U) {
        digit = (uint8_t)(value / divisor);
        if (digit != 0U || started) {
            started = true;
            if ((pos + 1U) < buf_size) {
                buf[pos++] = (char)('0' + digit);
            }
        }
        value %= divisor;
        divisor /= 10U;
    }
    buf[pos] = '\0';
}
//
// static uint32_t StringLength(const char *text)
// {
//     uint32_t len;
//
//     len = 0;
//     while (text[len] != '\0') {
//         len++;
//     }
//     return len;
// }
//
// static void CopyStringLimited(char *dst, const char *src, uint32_t dst_size)
// {
//     uint32_t pos;
//
//     if (dst_size == 0U) {
//         return;
//     }
//
//     pos = 0;
//     while (src[pos] != '\0' && (pos + 1U) < dst_size) {
//         dst[pos] = src[pos];
//         pos++;
//     }
//     dst[pos] = '\0';
// }
//
// static int8_t HexDigitValue(char ch)
// {
//     if (ch >= '0' && ch <= '9') {
//         return (int8_t)(ch - '0');
//     }
//     if (ch >= 'a' && ch <= 'f') {
//         return (int8_t)(ch - 'a' + 10);
//     }
//     if (ch >= 'A' && ch <= 'F') {
//         return (int8_t)(ch - 'A' + 10);
//     }
//     return -1;
// }
//
// static bool ParseHex16(const char *text, uint16_t *value)
// {
//     uint8_t pos;
//     int8_t digit;
//     uint16_t parsed;
//
//     parsed = 0U;
//     for (pos = 0U; pos < UART_CRC_HEX_LEN; pos++) {
//         digit = HexDigitValue(text[pos]);
//         if (digit < 0) {
//             return false;
//         }
//         parsed = (uint16_t)((parsed << 4) | (uint16_t)digit);
//     }
//
//     if (!OnlySpacesLeft(text + UART_CRC_HEX_LEN)) {
//         return false;
//     }
//
//     *value = parsed;
//     return true;
// }
//
// static bool StripAndVerifyChecksum(char *command)
// {
//     uint32_t body_len;
//     uint16_t received_crc;
//     uint16_t calculated_crc;
//     char *caret;
//
//     caret = command;
//     while (*caret != '\0' && *caret != '^') {
//         caret++;
//     }
//
//     if (*caret == '\0') {
//         return true;
//     }
//
//     body_len = (uint32_t)(caret - command);
//     if (!ParseHex16(caret + 1, &received_crc)) {
//         return false;
//     }
//
//     calculated_crc = CRC16_Calc((uint8_t *)command, body_len);
//     if (calculated_crc != received_crc) {
//         return false;
//     }
//
//     command[body_len] = '\0';
//     return true;
// }
//
// static const char *HistoryGetRecent(uint8_t offset)
// {
//     uint8_t index;
//
//     if (offset >= uart_history_count) {
//         return 0;
//     }
//
//     index = (uint8_t)((uart_history_next + UART_HISTORY_DEPTH - 1U - offset) % UART_HISTORY_DEPTH);
//     return uart_history[index];
// }
//
// static void HistoryAdd(const char *command)
// {
//     CopyStringLimited(uart_history[uart_history_next], command, UART_HISTORY_CMD_LEN + 1U);
//
//     uart_history_next++;
//     if (uart_history_next >= UART_HISTORY_DEPTH) {
//         uart_history_next = 0;
//     }
//
//     if (uart_history_count < UART_HISTORY_DEPTH) {
//         uart_history_count++;
//     }
//
//     uart_history_browse_offset = 0;
// }
//
// static bool ShouldRecordHistory(const char *command)
// {
//     if (command[0] == '\0') {
//         return false;
//     }
//
//     if (EqualsIgnoreCase(command, "GET HISTORY")) {
//         return false;
//     }
//
//     if (EqualsIgnoreCase(command, "CLR HISTORY") || EqualsIgnoreCase(command, "CLEAR HISTORY")) {
//         return false;
//     }
//
//     return true;
// }
//
// static void HistoryDisplayRecord(const char *record)
// {
//     char display_text[SEG7_DIGITS + 1U];
//     uint8_t pos;
//
//     pos = 0U;
//     while (record[pos] != '\0' && pos < SEG7_DIGITS) {
//         display_text[pos] = record[pos];
//         pos++;
//     }
//
//     if (pos == 0U) {
//         CopyStringLimited(display_text, "EMPTY", sizeof(display_text));
//     } else {
//         display_text[pos] = '\0';
//     }
//
//     SetDisplayText(display_text);
// }
//
// static void HistorySendAll(void)
// {
//     uint8_t offset;
//     char number[4];
//     const char *record;
//
//     SendLine("HISTORY:");
//     if (uart_history_count == 0U) {
//         SendLine("EMPTY");
//         return;
//     }
//
//     offset = 0U;
//     while (offset < uart_history_count) {
//         record = HistoryGetRecent(offset);
//         UInt32ToDec((uint32_t)offset + 1U, number, sizeof(number));
//         UARTStringPut(number);
//         UARTStringPut(":");
//         SendLine(record);
//         offset++;
//     }
// }

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
    uint32_t pos;
    uint32_t int_pos;
    uint8_t frac_len;
    char int_buf[11];
    char frac_buf[4];

    if (buf_size == 0U) {
        return;
    }

    UInt32ToDec(seconds, int_buf, sizeof(int_buf));

    pos = 0;
    int_pos = 0;
    while (int_buf[int_pos] != '\0' && (pos + 1U) < buf_size) {
        buf[pos++] = int_buf[int_pos++];
    }

    if (fraction != 0U && (pos + 1U) < buf_size) {
        frac_buf[0] = (char)('0' + (fraction / 100U));
        frac_buf[1] = (char)('0' + ((fraction / 10U) % 10U));
        frac_buf[2] = (char)('0' + (fraction % 10U));
        frac_buf[3] = '\0';

        frac_len = 3U;
        while (frac_len > 1U && frac_buf[frac_len - 1U] == '0') {
            frac_len--;
        }

        buf[pos++] = '.';
        int_pos = 0;
        while (int_pos < frac_len && (pos + 1U) < buf_size) {
            buf[pos++] = frac_buf[int_pos++];
        }
    }

    buf[pos] = '\0';
}

static void SendLine(const char *message)
{
    UARTStringPut(message);
    UARTStringPut("\r\n");
}

static void SendStatus(void)
{
    char speed[16];

    FormatSpeed(speed, sizeof(speed));
    UARTStringPut("STATUS:MODE=");
    UARTStringPut((uart_mode == UART_MODE_CONTROL) ? "1" : "0");
    UARTStringPut(",SPEED=");
    UARTStringPut(speed);
    UARTStringPut("\r\n");
}

static void SendSpeed(void)
{
    char speed[16];

    FormatSpeed(speed, sizeof(speed));
    UARTStringPut("SPEED:");
    UARTStringPut(speed);
    UARTStringPut("\r\n");
}

static bool ExecuteCommand(const char *command)
{
    // if (EqualsIgnoreCase(command, "GET HISTORY")) {
    //     HistorySendAll();
    //     return true;
    // }
    //
    // if (EqualsIgnoreCase(command, "CLR HISTORY") || EqualsIgnoreCase(command, "CLEAR HISTORY")) {
    //     UARTHistory_Clear();
    //     SendLine("OK");
    //     return true;
    // }

    if (EqualsIgnoreCase(command, "GET STATUS")) {
        SendStatus();
        return true;
    }

    if (EqualsIgnoreCase(command, "GET SPEED")) {
        SendSpeed();
        return true;
    }

    if (StartsWithIgnoreCase(command, "SET MODE")) {
        uint8_t mode;

        if (!AsciiIsSpace(command[8])) {
            SendLine("ERROR: INVALID PARAM");
            return false;
        }

        if (!ParseModeParam(command + 8, &mode)) {
            SendLine("ERROR: INVALID PARAM");
            return false;
        }

        uart_mode = mode;
        if (uart_mode == UART_MODE_LOCAL) {
            PF0_ResetLocalMode();
        } else {
            PF0_ResetUartMode();
        }
        SendStatus();
        return true;
    }

    if (StartsWithIgnoreCase(command, "SET DISP")) {
        const char *text = SkipSpaces(command + 8);

        if (!AsciiIsSpace(command[8])) {
            SendLine("ERROR: INVALID PARAM");
            return false;
        }

        if (uart_mode != UART_MODE_CONTROL) {
            SendLine("ERROR: NOT IN UART MODE");
            return false;
        }

        if (!ValidateDisplayText(text)) {
            SendLine("ERROR: INVALID PARAM");
            return false;
        }

        SetDisplayText(text);
        SendLine("OK");
        return true;
    }

    if (StartsWithIgnoreCase(command, "SET SPEED")) {
        uint32_t speed_ms;

        if (!AsciiIsSpace(command[9])) {
            SendLine("ERROR: INVALID PARAM");
            return false;
        }

        if (uart_mode != UART_MODE_CONTROL) {
            SendLine("ERROR: NOT IN UART MODE");
            return false;
        }

        if (!ParseSpeedMs(command + 9, &speed_ms)) {
            SendLine("ERROR: INVALID PARAM");
            return false;
        }

        SetScrollDelayMs(speed_ms);
        SendSpeed();
        return true;
    }

    SendLine("ERROR: INVALID COMMAND");
    return false;
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

// void UARTHistory_Clear(void)
// {
//     uart_history_count = 0U;
//     uart_history_next = 0U;
//     uart_history_browse_offset = 0U;
// }
//
// void UARTHistory_ShowNext(void)
// {
//     const char *record;
//
//     if (uart_history_count == 0U) {
//         SendLine("HISTORY:EMPTY");
//         SetDisplayText("EMPTY");
//         return;
//     }
//
//     record = HistoryGetRecent(uart_history_browse_offset);
//     UARTStringPut("HISTORY:");
//     SendLine(record);
//     HistoryDisplayRecord(record);
//
//     uart_history_browse_offset++;
//     if (uart_history_browse_offset >= uart_history_count) {
//         uart_history_browse_offset = 0U;
//     }
// }

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
    bool command_ok;

    if (!FetchCommand(command, sizeof(command), &overflow)) {
        return;
    }

    if (overflow) {
        SendLine("ERROR: BUFFER OVERFLOW");
        if (uart_mode == UART_MODE_CONTROL) {
            PF0_RecordUartCommandError();
        }
        return;
    }

    // if (!StripAndVerifyChecksum(command)) {
    //     SendLine("ERROR: CHECKSUM");
    //     if (uart_mode == UART_MODE_CONTROL) {
    //         PF0_RecordUartCommandError();
    //     }
    //     return;
    // }

    command_ok = ExecuteCommand(command);
    // if (command_ok && ShouldRecordHistory(command)) {
    //     HistoryAdd(command);
    // }

    if (uart_mode == UART_MODE_CONTROL) {
        if (command_ok) {
            PF0_RecordUartCommandSuccess();
        } else {
            PF0_RecordUartCommandError();
        }
    }
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
