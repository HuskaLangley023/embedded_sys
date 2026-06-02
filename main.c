#include "main.h"
#include "s800_i2c.h"
#include "flash_led.h"
#include "control_loop.h"
#include "s800_uart.h"
#include "hw_ints.h"

uint32_t ui32SysClock;
volatile bool time_flag_1ms = false;

int main(void) {
    ui32SysClock = SystemClock_PLL(); // PLL 120 MHz

    S800_SysTick_Init();
    S800_GPIO_Init();
    S800_I2C0_Init();
    S800_UART_Init();

    IntPrioritySet(FAULT_SYSTICK, 0x00);
    IntPrioritySet(INT_UART0, 0x80);
    IntEnable(INT_UART0);
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);

    IntMasterEnable();

    SetScrollSpeed(speed_level);

    while (1) {
        UARTCommand_Process();
        main_control_loop();
    }
}

void S800_GPIO_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); // Enable PortF
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)); // Wait for the GPIO moduleF ready
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ); // Enable PortJ
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ)); // Wait for the GPIO moduleJ ready
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);						//Enable PortN
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION));			//Wait for the GPIO moduleN ready

    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0); // Set PF0 as Output pin

    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);			//Set PN0 as Output pin
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1);		//Set PN1 as Output pin

    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1); // Set the PJ0,PJ1 as input pin
    GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}

void DelayMs(uint32_t ms) { SysCtlDelay((SysCtlClockGet() / 3000) * ms); }

uint32_t SystemClock_PLL(void) {
    // PLL 120 MHz
    uint32_t freq =
            SysCtlClockFreqSet(SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480 | SYSCTL_XTAL_25MHZ, 120000000);
    return freq;
}

void S800_SysTick_Init(void) {
    // 20MHz
    SysTickPeriodSet(ui32SysClock / 1000);

    SysTickIntEnable();
    SysTickEnable();
}
