//
// Created by zhu_y on 2026/5/29.
//
#include "main.h"
#include "s800_i2c.h"
#include "flash_led.h"
#include "s800_uart.h"

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

uint32_t rx_buf;
void UART0_Handler(void)
{
    int32_t uart0_int_status;
    uart0_int_status 	= UARTIntStatus(UART0_BASE, true);
    // 根据返回值uart0_int_status可以判断中断事件是接收中断、接收超时中断或发送中断等。详见本函数结束后的注释
    //本例程只适用于接收最多8字节的情形，满足实验指导书的要求。
    //若接收大于8字节例如10个字节，要在正常接收中断中接收8个字节，在超时接收中断中接收2个字节。
    UARTIntClear(UART0_BASE, uart0_int_status);				//Clear the asserted interrupts

    while(UARTCharsAvail(UART0_BASE))    		// Loop while there are characters in the receive FIFO.
    {
        ///Read the next character from the UART and write it back to the UART.
        rx_buf = UARTCharGetNonBlocking(UART0_BASE);
        UARTCharPutNonBlocking(UART0_BASE, rx_buf);  //非阻塞方式UART接收和发送
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1,GPIO_PIN_1 );
        //		Delay(1000);
    }
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1,0 );
}