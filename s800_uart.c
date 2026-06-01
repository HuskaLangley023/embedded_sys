//
// Created by zhu_y on 2026/5/29.
//
#include "s800_uart.h"

void UARTStringPut(uint8_t *cMessage)
{
    while(*cMessage!='\0')
        UARTCharPut(UART0_BASE,*(cMessage++));
}

void UARTStringPutNonBlocking(const char *cMessage)
{
    while(*cMessage!='\0')// 字符'\0'是C语言我i字符串自动添加的结束字符，ASCII码为00H。
        UARTCharPutNonBlocking(UART0_BASE,*(cMessage++));
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


    UARTStringPut((uint8_t *)"\r\nHello, world!\r\n");
}