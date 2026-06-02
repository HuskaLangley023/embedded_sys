/**
 ******************************************************************************
 * @file    crc.c
 * @brief   CRC calculation for UART command checksum.
 ******************************************************************************
 */

#include "crc.h"
#include <string.h>

#define CRC8_BASE 0xffU
#define CRC8_POLY 0x8cU
#define CRC16_BASE 0xffffU
#define CRC16_POLY 0x8408U

uint8_t CRC8_Calc(uint8_t *data, uint32_t len) {
    uint8_t crc8;
    uint8_t bit;

    crc8 = CRC8_BASE;
    while (len-- != 0U) {
        crc8 ^= *data++;
        for (bit = 0; bit < 8U; bit++) {
            if ((crc8 & 0x01U) != 0U) {
                crc8 = (uint8_t)((crc8 >> 1) ^ CRC8_POLY);
            } else {
                crc8 >>= 1;
            }
        }
    }

    return crc8;
}

uint8_t CRC8_Verify(uint8_t *data, uint32_t len) {
    uint8_t data_crc8;

    if ((data == 0) || (len <= 2U)) {
        return 0U;
    }

    memcpy(&data_crc8, &data[len - 1U], 1U);
    return (CRC8_Calc(data, len - 1U) == data_crc8) ? 1U : 0U;
}

void CRC8_Append(uint8_t *data, uint32_t len) {
    uint8_t crc8;

    if ((data == 0) || (len <= 2U)) {
        return;
    }

    crc8 = CRC8_Calc(data, len - 1U);
    memcpy(&data[len - 1U], &crc8, 1U);
}

uint16_t CRC16_Calc(uint8_t *data, uint32_t len) {
    uint16_t crc16;
    uint8_t bit;

    crc16 = CRC16_BASE;
    while (len-- != 0U) {
        crc16 ^= *data++;
        for (bit = 0; bit < 8U; bit++) {
            if ((crc16 & 0x0001U) != 0U) {
                crc16 = (uint16_t)((crc16 >> 1) ^ CRC16_POLY);
            } else {
                crc16 >>= 1;
            }
        }
    }

    return crc16;
}

uint8_t CRC16_Verify(uint8_t *data, uint32_t len) {
    uint16_t data_crc16;

    if ((data == 0) || (len <= 2U)) {
        return 0U;
    }

    memcpy(&data_crc16, &data[len - 2U], 2U);
    return (CRC16_Calc(data, len - 2U) == data_crc16) ? 1U : 0U;
}

void CRC16_Append(uint8_t *data, uint32_t len) {
    uint16_t crc16;

    if ((data == 0) || (len <= 2U)) {
        return;
    }

    crc16 = CRC16_Calc(data, len - 2U);
    memcpy(&data[len - 2U], &crc16, 2U);
}
