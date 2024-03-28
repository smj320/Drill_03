//
// Created by kikuchi on 2024/03/27.
//

#ifndef DRILL03_MOD20_H
#define DRILL03_MOD20_H

#include "main.h"

#define MOD20_I2C_ADDR 0xA4

int mod20_Init(I2C_HandleTypeDef *hi2c1);
int mod20_open(I2C_HandleTypeDef *hi2c1, uint16_t nf);
int mod20_close(I2C_HandleTypeDef *hi2c1);
int mod20_write16byte(I2C_HandleTypeDef *hi2c1, uint8_t *buf);
int mod20_write80byte(I2C_HandleTypeDef *hi2c1, uint8_t *buf);

#endif //DRILL03_MOD20_H
