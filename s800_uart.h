//
// Created by zhu_y on 2026/5/29.
//

#ifndef S800_UART_H
#define S800_UART_H

#include "main.h"
#include "uart.h"

void S800_UART_Init(void);
void UARTStringPut(uint8_t *cMessage);
void UARTStringPutNonBlocking(const char *cMessage);



#endif //S800_UART_H
