//
// Created by zhu_y on 2026/5/29.
//

#ifndef S800_UART_H
#define S800_UART_H

#include "main.h"
#include "uart.h"

#define UART_MODE_LOCAL 0U
#define UART_MODE_CONTROL 1U

void S800_UART_Init(void);
void UARTStringPut(const char *cMessage);
void UARTStringPutNonBlocking(const char *cMessage);

void UARTCommand_RxByte(uint8_t byte);
void UARTCommand_Process(void);
uint8_t UARTCommand_GetMode(void);


#endif //S800_UART_H
