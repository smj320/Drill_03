//
// Created by kikuchi on 2024/03/27.
//

#ifndef DRILL03_MOD20_H
#define DRILL03_MOD20_H

#include "main.h"

#define FILE_RENEW_SEC 60*10   //ファイル更新間隔(秒)
#define FILE_ROLLUP 60000

#define MOD20_I2C_ADDR 0xA4
#define I2C_TIMEOUT 10
#define N_RTC_BUF 80

int mod20_Init(I2C_HandleTypeDef *hi2c1);
int mod20_open(I2C_HandleTypeDef *hi2c1, uint16_t nf);
int mod20_close(I2C_HandleTypeDef *hi2c1);
int mod20_write16byte(I2C_HandleTypeDef *hi2c1, uint8_t *buf);
int mod20_write80byte(I2C_HandleTypeDef *hi2c1, uint8_t *buf);

#endif //DRILL03_MOD20_H
