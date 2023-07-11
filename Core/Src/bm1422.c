#include "main.h"
#include <stdio.h>
#include <string.h>

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;
uint8_t BM1422_addr = 0x0e << 1;

void BM1422_write_data(uint8_t addr, uint8_t reg, uint8_t data) {

    HAL_I2C_Mem_Write(&hi2c1, addr, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, 10);
}

int16_t BM1422_read_word_data(uint8_t addr, uint8_t reg) {
    uint8_t data[2];
    volatile HAL_StatusTypeDef s;

    HAL_I2C_Mem_Read(&hi2c1, addr, reg, I2C_MEMADD_SIZE_8BIT, data, 2, 100);

    return (int16_t) ((data[1] << 8) + data[0]);
}

void BM1422_Init() {

    BM1422_write_data(BM1422_addr, 0x1b, 0x80); // CNTL1
    HAL_Delay(10);
    BM1422_write_data(BM1422_addr, 0x5c, 0x00); // CNTL4
    BM1422_write_data(BM1422_addr, 0x5d, 0x00);
    HAL_Delay(10);
    BM1422_write_data(BM1422_addr, 0x1c, 0x08); // CNTL2
    HAL_Delay(10);
    BM1422_write_data(BM1422_addr, 0x1d, 0x40); // CNTL3
}

void BM1422_read_mag(int16_t mag[]) {

    mag[0] = BM1422_read_word_data(BM1422_addr, 0x10);
    HAL_Delay(1);
    mag[1] = BM1422_read_word_data(BM1422_addr, 0x12);
    HAL_Delay(1);
    mag[2] = BM1422_read_word_data(BM1422_addr, 0x14);
}

void BM1422_dump()
{
    while (1) {
        int16_t mag_xyz[3];
        char msg[128];
        BM1422_read_mag(mag_xyz);
        sprintf(msg, "X=%d Y=%d Z=%d\r\n", mag_xyz[0], mag_xyz[1], mag_xyz[2]);
        HAL_UART_Transmit_DMA(&huart2, msg, strlen(msg));
        HAL_Delay(1000);
    }
}