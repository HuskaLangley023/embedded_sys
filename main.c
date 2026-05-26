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

// // 闪烁延时
#define FASTFLASHTIME 300000
#define SLOWFLASHTIME (FASTFLASHTIME * 20)
#define STOPFLASHTIME 0

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
uint8_t str_len;
int dir = 1;
uint8_t window_pos = 0;
uint32_t ui32SysClock;
uint32_t pj0_val = 0;
uint32_t pj1_val = 0;

uint8_t index = 0;

uint32_t delay_time = 0;

bool time_flag_1ms = false;

enum SPEEDLEVEL { FAST, STOP, SLOW } speed_level;

void LED_Flash(enum SPEEDLEVEL speed_level);
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
    S800_SysTick_Init();

    while (1) {
        int i;
        for (i = 0; i < 8; i++) {
            // 数码管第一位显示 1~8
            result = I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_OUTPUT_PORT1, data);

            result = I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_OUTPUT_PORT2, num);

            // LED1~LED8 依次点亮
            // PCA9557 控制 LED 为低电平点亮，所以用 ~(1 << i)
            result = I2C0_WriteByte(PCA9557_I2CADDR, PCA9557_OUTPUT, (uint8_t) (~(1 << i)));

            Delay(800000);
        }

        if (time_flag_1ms) {
            time_flag_1ms = false;
            // 读取按键
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
            }
            // LED_Flash(speed_level);

            window_pos += dir;
        }
    }
}

bool is_key_Pressed(uint32_t ui32Port, uint8_t ui8Pins) {
    if (GPIOPinRead(ui32Port, ui8Pins) == 0) {
        DelayMs(10); // 消抖延时

        if (GPIOPinRead(ui32Port, ui8Pins) == 0) {
            // 等待松手，避免一次按下触发多次
            while (GPIOPinRead(ui32Port, ui8Pins) == 0) {
            }
            DelayMs(10);
            return true;
        }
    }
    return false;
}


// LED 根据按键闪烁
void LED_Flash(enum SPEEDLEVEL speed_level) {
    switch (speed_level) {
        case FAST:
            delay_time = FASTFLASHTIME;
            break;
        case SLOW:
            delay_time = SLOWFLASHTIME;
            break;
        case STOP:
            delay_time = STOPFLASHTIME;
            break;
    }

    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 1);
    Delay(delay_time);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0);
    Delay(delay_time);
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

    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0); // 初始化i2c模块
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB); // 使用I2C模块0，引脚配置为I2C0SCL--PB2、I2C0SDA--PB3
    GPIOPinConfigure(GPIO_PB2_I2C0SCL); // 配置PB2为I2C0SCL
    GPIOPinConfigure(GPIO_PB3_I2C0SDA); // 配置PB3为I2C0SDA
    GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2); // I2C将GPIO_PIN_2用作SCL
    GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3); // I2C将GPIO_PIN_3用作SDA

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
    }; // 如果I2C0模块忙，等待
    //
    I2CMasterSlaveAddrSet(I2C0_BASE, DevAddr, false);
    // 设置主机要放到总线上的从机地址。false表示主机写从机，true表示主机读从机

    I2CMasterDataPut(I2C0_BASE, RegAddr); // 主机写设备寄存器地址
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START); // 执行重复写入操作
    while (I2CMasterBusy(I2C0_BASE)) {
    };

    rop = (uint8_t) I2CMasterErr(I2C0_BASE); // 调试用

    I2CMasterDataPut(I2C0_BASE, WriteData);
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH); // 执行重复写入操作并结束
    while (I2CMasterBusy(I2C0_BASE)) {
    };

    rop = (uint8_t) I2CMasterErr(I2C0_BASE); // 调试用

    return rop; // 返回错误类型，无错返回0
}

uint8_t I2C0_ReadByte(uint8_t DevAddr, uint8_t RegAddr) {
    uint8_t value, rop;
    while (I2CMasterBusy(I2C0_BASE)) {
    };
    I2CMasterSlaveAddrSet(I2C0_BASE, DevAddr, false);
    I2CMasterDataPut(I2C0_BASE, RegAddr);
    //	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_SEND); // 执行单词写入操作
    while (I2CMasterBusBusy(I2C0_BASE))
        ;
    rop = (uint8_t) I2CMasterErr(I2C0_BASE);
    Delay(1);
    // receive data
    I2CMasterSlaveAddrSet(I2C0_BASE, DevAddr, true); // 设置从机地址
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE); // 执行单次读操作
    while (I2CMasterBusBusy(I2C0_BASE))
        ;
    value = I2CMasterDataGet(I2C0_BASE); // 获取读取的数据
    Delay(1);
    return value;
}


void DelayMs(uint32_t ms) { SysCtlDelay((SysCtlClockGet() / 3000) * ms); }

// PLL 20 MHz
uint32_t SystemClock_PLL(void) {
    // 使用外部 25 MHz 晶振，PLL 输出 480 MHz VCO，分频得到 20 MHz
    uint32_t freq =
            SysCtlClockFreqSet(SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480 | SYSCTL_XTAL_25MHZ, 20000000);
    return freq;
}

void S800_SysTick_Init(void) {
    // 20MHz 时钟，1ms中断一次
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
            return 0x00; // 不显示
    }
}

void SysTick_Handler(void) {
    time_flag_1ms = true;

    str_len = sizeof(str_buffer)-1;

    uint8_t char_pos = (window_pos + index) % str_len;

    result = I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_OUTPUT_PORT1, Seg7Code_FromAscii(str_buffer[char_pos]));

    result = I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_OUTPUT_PORT2, (uint8_t) (1 << index));

    // LED1~LED8 依次点亮，低电平点亮
    result = I2C0_WriteByte(PCA9557_I2CADDR, PCA9557_OUTPUT, (uint8_t) (~(1 << window_pos)));

    index++;

    if (index >= 8) {
        index = 0;
    }
}


void Delay(uint32_t value) {
    uint32_t ui32Loop;
    for (ui32Loop = 0; ui32Loop < value; ui32Loop++) {
    };
}
