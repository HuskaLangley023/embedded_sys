#include <stdbool.h>
#include <stdint.h>
#include "debug.h"
#include "gpio.h"
#include "hw_i2c.h"
#include "hw_memmap.h"
#include "hw_types.h"
#include "i2c.h"
#include "interrupt.h"
#include "pin_map.h"
#include "sysctl.h"
#include "systick.h"
#include "string.h"

// // ��˸��ʱ
#define FAST_SCROLL_TIME_MS 200U
#define SLOW_SCROLL_TIME_MS 800U
#define STOP_SCROLL_TIME_MS 0U
#define SEG7_DIGITS 8U

//*****************************************************************************
//
// I2C GPIO chip address and resigster define
//
//*****************************************************************************
#define TCA6424_I2CADDR 0x22
#define PCA9557_I2CADDR 0x18

#define PCA9557_INPUT 0x00
#define PCA9557_OUTPUT 0x01
#define PCA9557_POLINVERT 0x02
#define PCA9557_CONFIG 0x03

#define TCA6424_CONFIG_PORT0 0x0c
#define TCA6424_CONFIG_PORT1 0x0d
#define TCA6424_CONFIG_PORT2 0x0e

#define TCA6424_INPUT_PORT0 0x00
#define TCA6424_INPUT_PORT1 0x01
#define TCA6424_INPUT_PORT2 0x02

#define TCA6424_OUTPUT_PORT0 0x04
#define TCA6424_OUTPUT_PORT1 0x05
#define TCA6424_OUTPUT_PORT2 0x06


uint8_t str_buffer[] = "523010910148";
uint8_t str_len = sizeof(str_buffer) - 1U;
volatile int8_t dir = 1;
volatile uint8_t window_pos = 0;
uint32_t ui32SysClock;
uint32_t pj0_val = 0;
uint32_t pj1_val = 0;

volatile uint8_t index = 0;

volatile uint32_t delay_time = FAST_SCROLL_TIME_MS;
volatile uint32_t scroll_tick = 0;
volatile uint32_t pf0_tick = 0;
volatile bool pf0_on = false;

volatile bool time_flag_1ms = false;

enum SPEEDLEVEL { FAST, STOP, SLOW } speed_level = FAST;

void LED_Flash(enum SPEEDLEVEL speed_level);
void SetScrollSpeed(enum SPEEDLEVEL level);
void MoveWindow(void);
uint32_t SystemClock_PLL(void);

void S800_SysTick_Init(void);
void Delay(uint32_t value);
void S800_GPIO_Init(void);
uint8_t I2C0_WriteByte(uint8_t DevAddr, uint8_t RegAddr, uint8_t WriteData);
uint8_t I2C0_ReadByte(uint8_t DevAddr, uint8_t RegAddr);
void S800_I2C0_Init(void);
void DelayMs(uint32_t ms);
bool is_key_Pressed(uint32_t ui32Port, uint8_t ui8Pins);
uint8_t result;

uint8_t data = 0x3f;
uint8_t num = 0xff;

int main(void) {
    ui32SysClock = SystemClock_PLL(); // PLL 120 MHz

    S800_GPIO_Init();
    S800_I2C0_Init();
    SetScrollSpeed(speed_level);
    S800_SysTick_Init();

    while (1) {
        if (time_flag_1ms) {
            time_flag_1ms = false;
            // read keys
            if (is_key_Pressed(GPIO_PORTJ_BASE, GPIO_PIN_0)) { // sw1
                dir = -dir;
            }
            if (is_key_Pressed(GPIO_PORTJ_BASE, GPIO_PIN_1)) { // sw2
                switch (speed_level) {
                    case FAST:
                        speed_level = SLOW;
                        break;
                    case SLOW:
                        speed_level = STOP;
                        break;
                    case STOP:
                        speed_level = FAST;
                        break;
                }
                SetScrollSpeed(speed_level);
            }
        }
    }
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


// LED ���ݰ�����˸
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

void S800_GPIO_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); // Enable PortF
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF))
        ; // Wait for the GPIO moduleF ready
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ); // Enable PortJ
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ))
        ; // Wait for the GPIO moduleJ ready

    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0); // Set PF0 as Output pin
    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1); // Set the PJ0,PJ1 as input pin
    GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}

void S800_I2C0_Init(void) {

    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0); // ��ʼ��i2cģ��
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB); // ʹ��I2Cģ��0����������ΪI2C0SCL--PB2��I2C0SDA--PB3
    GPIOPinConfigure(GPIO_PB2_I2C0SCL); // ����PB2ΪI2C0SCL
    GPIOPinConfigure(GPIO_PB3_I2C0SDA); // ����PB3ΪI2C0SDA
    GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2); // I2C��GPIO_PIN_2����SCL
    GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3); // I2C��GPIO_PIN_3����SDA

    I2CMasterInitExpClk(I2C0_BASE, ui32SysClock, true); // config I2C0 400k
    I2CMasterEnable(I2C0_BASE);

    result = I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_CONFIG_PORT0, 0x0ff); // config port 0 as input
    result = I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_CONFIG_PORT1, 0x0); // config port 1 as output
    result = I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_CONFIG_PORT2, 0x0); // config port 2 as output

    result = I2C0_WriteByte(PCA9557_I2CADDR, PCA9557_CONFIG, 0x00); // config port as output
    result = I2C0_WriteByte(PCA9557_I2CADDR, PCA9557_OUTPUT, 0x0ff); // turn off the LED1-8
}

uint8_t I2C0_WriteByte(uint8_t DevAddr, uint8_t RegAddr, uint8_t WriteData) {
    uint8_t rop;
    while (I2CMasterBusy(I2C0_BASE)) {
    }; // ���I2C0ģ��æ���ȴ�
    //
    I2CMasterSlaveAddrSet(I2C0_BASE, DevAddr, false);
    // ��������Ҫ�ŵ������ϵĴӻ���ַ��false��ʾ����д�ӻ���true��ʾ�������ӻ�

    I2CMasterDataPut(I2C0_BASE, RegAddr); // ����д�豸�Ĵ�����ַ
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START); // ִ���ظ�д�����
    while (I2CMasterBusy(I2C0_BASE)) {
    };

    rop = (uint8_t) I2CMasterErr(I2C0_BASE); // ������

    I2CMasterDataPut(I2C0_BASE, WriteData);
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH); // ִ���ظ�д�����������
    while (I2CMasterBusy(I2C0_BASE)) {
    };

    rop = (uint8_t) I2CMasterErr(I2C0_BASE); // ������

    return rop; // ���ش������ͣ��޴����0
}

uint8_t I2C0_ReadByte(uint8_t DevAddr, uint8_t RegAddr) {
    uint8_t value, rop;
    while (I2CMasterBusy(I2C0_BASE)) {
    };
    I2CMasterSlaveAddrSet(I2C0_BASE, DevAddr, false);
    I2CMasterDataPut(I2C0_BASE, RegAddr);
    //	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_SEND); // ִ�е���д�����
    while (I2CMasterBusBusy(I2C0_BASE))
        ;
    rop = (uint8_t) I2CMasterErr(I2C0_BASE);
    Delay(1);
    // receive data
    I2CMasterSlaveAddrSet(I2C0_BASE, DevAddr, true); // ���ôӻ���ַ
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE); // ִ�е��ζ�����
    while (I2CMasterBusBusy(I2C0_BASE))
        ;
    value = I2CMasterDataGet(I2C0_BASE); // ��ȡ��ȡ������
    Delay(1);
    return value;
}


void DelayMs(uint32_t ms) { SysCtlDelay((SysCtlClockGet() / 3000) * ms); }

// PLL 20 MHz
uint32_t SystemClock_PLL(void) {
    // ʹ���ⲿ 25 MHz ����PLL ��� 480 MHz VCO����Ƶ�õ� 20 MHz
    uint32_t freq =
            SysCtlClockFreqSet(SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480 | SYSCTL_XTAL_25MHZ, 20000000);
    return freq;
}

void S800_SysTick_Init(void) {
    // 20MHz ʱ�ӣ�1ms�ж�һ��
    SysTickPeriodSet(ui32SysClock / 1000);

    SysTickIntEnable();
    SysTickEnable();

    IntMasterEnable();
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
void SysTick_Handler(void) {
    time_flag_1ms = true;

    if (delay_time != STOP_SCROLL_TIME_MS) {
        scroll_tick++;
        pf0_tick++;

        if (scroll_tick >= delay_time) {
            scroll_tick = 0;
            MoveWindow();
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

    result = I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_OUTPUT_PORT1, Seg7Code_FromAscii(str_buffer[char_pos]));

    result = I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_OUTPUT_PORT2, (uint8_t) (1 << index));

    result = I2C0_WriteByte(PCA9557_I2CADDR, PCA9557_OUTPUT, (uint8_t) (~(1 << (window_pos % SEG7_DIGITS))));

    index++;

    if (index >= SEG7_DIGITS) {
        index = 0;
    }
}

void Delay(uint32_t value) {
    uint32_t ui32Loop;
    for (ui32Loop = 0; ui32Loop < value; ui32Loop++) {
    };
}
