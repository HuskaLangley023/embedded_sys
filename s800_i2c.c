//
// Created by zhu_y on 2026/5/29.
//
#include "s800_i2c.h"

void S800_I2C0_Init(void) {

    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0); // ��ʼ��i2cģ��
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB); // ʹ��I2Cģ��0����������ΪI2C0SCL--PB2��I2C0SDA--PB3
    GPIOPinConfigure(GPIO_PB2_I2C0SCL); // ����PB2ΪI2C0SCL
    GPIOPinConfigure(GPIO_PB3_I2C0SDA); // ����PB3ΪI2C0SDA
    GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2); // I2C��GPIO_PIN_2����SCL
    GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3); // I2C��GPIO_PIN_3����SDA

    I2CMasterInitExpClk(I2C0_BASE, ui32SysClock, true); // config I2C0 400k
    I2CMasterEnable(I2C0_BASE);

    I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_CONFIG_PORT0, 0x0ff); // config port 0 as input
    I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_CONFIG_PORT1, 0x0); // config port 1 as output
    I2C0_WriteByte(TCA6424_I2CADDR, TCA6424_CONFIG_PORT2, 0x0); // config port 2 as output

    I2C0_WriteByte(PCA9557_I2CADDR, PCA9557_CONFIG, 0x00); // config port as output
    I2C0_WriteByte(PCA9557_I2CADDR, PCA9557_OUTPUT, 0x0ff); // turn off the LED1-8
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
    uint8_t value;
    while (I2CMasterBusy(I2C0_BASE)) {
    };
    I2CMasterSlaveAddrSet(I2C0_BASE, DevAddr, false);
    I2CMasterDataPut(I2C0_BASE, RegAddr);
    //	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_SEND); // ִ�е���д�����
    while (I2CMasterBusBusy(I2C0_BASE))
        ;
    (void) I2CMasterErr(I2C0_BASE);
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

void Delay(uint32_t value) {
    uint32_t ui32Loop;
    for (ui32Loop = 0; ui32Loop < value; ui32Loop++) {
    };
}
