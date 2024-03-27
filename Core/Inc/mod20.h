//
// Created by kikuchi on 2024/03/27.
//

#ifndef DRILL03_MOD20_H
#define DRILL03_MOD20_H

#include "main.h"

#define MOD20_I2C_ADDR 0xA4

void mod20_Init(I2C_HandleTypeDef *hi2c1);
void mod20_open(I2C_HandleTypeDef *hi2c1, uint16_t nf);
void mod20_close(I2C_HandleTypeDef *hi2c1);
void mod20_write16byte(I2C_HandleTypeDef *hi2c1, uint8_t *buf);
void mod20_write80byte(I2C_HandleTypeDef *hi2c1, uint8_t *buf);

#endif //DRILL03_MOD20_H
