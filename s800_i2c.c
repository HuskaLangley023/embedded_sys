//
// Created by zhu_y on 2026/5/29.
//
#include "s800_i2c.h"

#define I2C0_WAIT_TIMEOUT 50000U
#define I2C0_START_TIMEOUT 1000U
#define I2C0_ERR_TIMEOUT 0x80U
#define I2C0_BUS_SPEED_HZ 1000000U

static void I2C0_ShortDelay(void) {
    volatile uint32_t i;

    for (i = 0U; i < 32U; i++) {
    }
}

static uint8_t I2C0_WaitIdle(void) {
    uint32_t timeout;

    timeout = I2C0_WAIT_TIMEOUT;
    while (I2CMasterBusy(I2C0_BASE)) {
        if (timeout == 0U) {
            return I2C0_ERR_TIMEOUT;
        }
        timeout--;
    }

    return (uint8_t) I2CMasterErr(I2C0_BASE);
}

static uint8_t I2C0_WaitCommandDone(void) {
    uint32_t timeout;
    bool started;

    I2C0_ShortDelay();

    started = false;
    timeout = I2C0_START_TIMEOUT;
    while (!I2CMasterBusy(I2C0_BASE)) {
        if (timeout == 0U) {
            break;
        }
        timeout--;
    }
    if (I2CMasterBusy(I2C0_BASE)) {
        started = true;
    }
    if (!started) {
        return I2C0_ERR_TIMEOUT;
    }

    timeout = I2C0_WAIT_TIMEOUT;
    while (I2CMasterBusy(I2C0_BASE)) {
        if (timeout == 0U) {
            I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_ERROR_STOP);
            return I2C0_ERR_TIMEOUT;
        }
        timeout--;
    }

    return (uint8_t) I2CMasterErr(I2C0_BASE);
}

void S800_I2C0_Init(void) {
    uint32_t i2c_tpr;

    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0); // ��ʼ��i2cģ��
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB); // ʹ��I2Cģ��0����������ΪI2C0SCL--PB2��I2C0SDA--PB3
    GPIOPinConfigure(GPIO_PB2_I2C0SCL); // ����PB2ΪI2C0SCL
    GPIOPinConfigure(GPIO_PB3_I2C0SDA); // ����PB3ΪI2C0SDA
    GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2); // I2C��GPIO_PIN_2����SCL
    GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3); // I2C��GPIO_PIN_3����SDA

    I2CMasterInitExpClk(I2C0_BASE, ui32SysClock, true);
    i2c_tpr = ((ui32SysClock + (20U * I2C0_BUS_SPEED_HZ) - 1U) / (20U * I2C0_BUS_SPEED_HZ)) - 1U;
    HWREG(I2C0_BASE + I2C_O_MTPR) = i2c_tpr;
    I2CMasterEnable(I2C0_BASE);

    I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_CONFIG_PORT0, 0x0ff); // config port 0 as input
    I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_CONFIG_PORT1, 0x0); // config port 1 as output
    I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_CONFIG_PORT2, 0x0); // config port 2 as output

    I2C0_WriteByte(PCA9557_I2CADDR, PCA9557_CONFIG, 0x00); // config port as output
    I2C0_WriteByte(PCA9557_I2CADDR, PCA9557_OUTPUT, 0x0ff); // turn off the LED1-8
}

uint8_t I2C0_WriteByte(uint8_t DevAddr, uint8_t RegAddr, uint8_t WriteData) {
    uint8_t rop;

    rop = I2C0_WaitIdle();
    if (rop != I2C_MASTER_ERR_NONE) {
        return rop;
    }

    I2CMasterSlaveAddrSet(I2C0_BASE, DevAddr, false);
    // ��������Ҫ�ŵ������ϵĴӻ���ַ��false��ʾ����д�ӻ���true��ʾ�������ӻ�

    I2CMasterDataPut(I2C0_BASE, RegAddr); // ����д�豸�Ĵ�����ַ
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START); // ִ���ظ�д�����

    rop = I2C0_WaitCommandDone();
    if (rop != I2C_MASTER_ERR_NONE) {
        I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_ERROR_STOP);
        (void) I2C0_WaitCommandDone();
        return rop;
    }

    I2CMasterDataPut(I2C0_BASE, WriteData);
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH); // ִ���ظ�д�����������
    rop = I2C0_WaitCommandDone();

    return rop; // ���ش������ͣ��޴����0
}

uint8_t I2C0_ReadByte(uint8_t DevAddr, uint8_t RegAddr) {
    uint8_t value;
    uint8_t rop;

    rop = I2C0_WaitIdle();
    if (rop != I2C_MASTER_ERR_NONE) {
        return 0U;
    }

    I2CMasterSlaveAddrSet(I2C0_BASE, DevAddr, false);
    I2CMasterDataPut(I2C0_BASE, RegAddr);
    //	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_SEND); // ִ�е���д�����
    rop = I2C0_WaitCommandDone();
    if (rop != I2C_MASTER_ERR_NONE) {
        return 0U;
    }
    Delay(1);
    // receive data
    I2CMasterSlaveAddrSet(I2C0_BASE, DevAddr, true); // ���ôӻ���ַ
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE); // ִ�е��ζ�����
    rop = I2C0_WaitCommandDone();
    if (rop != I2C_MASTER_ERR_NONE) {
        return 0U;
    }
    value = I2CMasterDataGet(I2C0_BASE); // ��ȡ��ȡ������
    Delay(1);
    return value;
}

void Delay(uint32_t value) {
    volatile uint32_t ui32Loop;
    for (ui32Loop = 0; ui32Loop < value; ui32Loop++) {
    };
}
