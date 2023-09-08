//
// Created by kikuchi on 2023/06/23.
//
#include "main.h"
#include "drill_mon.h"
#include "mcp3424.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;

int MCP3424_Read(uint8_t addr, uint8_t ch, uint16_t *data) {
    HAL_StatusTypeDef s;
    uint8_t rcv[3];
    uint8_t cmd[4] = {0x84, 0xA4, 0xC4, 0xE4};
    s = HAL_I2C_Master_Transmit(&hi2c1, addr << 1, &cmd[ch], 1, 100);
    if (s != HAL_OK) return -1;
    HAL_Delay(1);
    s = HAL_I2C_Master_Receive(&hi2c1, addr << 1, rcv, 3, 100);
    if (s != HAL_OK) return -2;
    *data = rcv[0] << 8 | rcv[1];
    return 0;
}

void MCP3424_dump(uint8_t adr, int8_t ch) {
    int rtc;
    uint16_t data;
    while (1) {
        rtc = MCP3424_Read(adr, ch, &data);
        Lib_dump_ad(ch, rtc, data);
        HAL_Delay(1000);
    }
}