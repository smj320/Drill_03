//
// Created by kikuchi on 2023/06/23.
//
#include "main.h"
#include "mcp3424.h"
#include <string.h>
#include <stdio.h>

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;

int MCP3424_Read(uint8_t n_dev, uint8_t ch, uint16_t *data)
{
    HAL_StatusTypeDef s;
    uint8_t rcv[3];
    uint8_t dev_addr[3] = {MCP3424_PT100_ADDR,MCP3424_LVDT_ADDR,MCP3424_BAT_ADDR };
    uint8_t cmd[4]={0x98, 0xB8, 0xD8, 0xF8};
    s = HAL_I2C_Master_Transmit(&hi2c1, dev_addr[n_dev]<<1, &cmd[ch], 1, 100);
    if(s!=HAL_OK) return -1;
    HAL_Delay(1);
    s = HAL_I2C_Master_Receive(&hi2c1, dev_addr[n_dev]<<1, rcv, 3, 100);
    if(s!=HAL_OK) return -2;
    if((rcv[2]&0x80)!=0) return -3;
    *data = rcv[0]<<8 | rcv[1];
    return 0;
}

void MCP3424_dump()
{
    char msg[128];
    int rtc;
    uint8_t ti=0;
    uint16_t data;
    while (1){
        rtc = MCP3424_Read(0,0,&data);
        sprintf(msg, "%03d CH1 %04X RTC %02d\r\n",ti++,data, rtc);
        HAL_UART_Transmit_DMA(&huart2, msg, strlen(msg));
        HAL_Delay(1000);
    }
}