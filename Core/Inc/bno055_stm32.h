#ifndef BNO055_STM32_H_
#define BNO055_STM32_H_

#ifdef __cplusplus
  extern "C" {
#endif

#include "main.h"

#ifdef FREERTOS_ENABLED
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#endif

#include "bno055.h"

I2C_HandleTypeDef *_bno055_i2c_port;

void bno055_assignI2C(I2C_HandleTypeDef *hi2c_device) {
  _bno055_i2c_port = hi2c_device;
}

void bno055_delay(int time) {
#ifdef FREERTOS_ENABLED
  osDelay(time);
#else
  HAL_Delay(time);
#endif
}

int bno055_writeData(uint8_t reg, uint8_t data) {
  uint8_t txdata[2] = {reg, data};
  volatile uint8_t status;
  status = HAL_I2C_Master_Transmit(_bno055_i2c_port, BNO055_I2C_ADDR << 1,
                                   txdata, sizeof(txdata), 100);
  if (status == HAL_OK) {
    return 0;
  }

  volatile uint32_t error = HAL_I2C_GetError(_bno055_i2c_port);
  volatile HAL_I2C_StateTypeDef state = HAL_I2C_GetState(_bno055_i2c_port);

  // while (HAL_I2C_GetState(_bno055_i2c_port) != HAL_I2C_STATE_READY) {}
  return -1;
}

int bno055_readData(uint8_t reg, uint8_t *data, uint8_t len) {
  HAL_I2C_Master_Transmit(_bno055_i2c_port, BNO055_I2C_ADDR << 1, &reg, 1,
                          100);
  HAL_Delay(10);
  HAL_I2C_Master_Receive(_bno055_i2c_port, BNO055_I2C_ADDR << 1, data, len,
                         100);
  return 0;
  // HAL_I2C_Mem_Read(_bno055_i2c_port, BNO055_I2C_ADDR_LO<<1, reg,
  // I2C_MEMADD_SIZE_8BIT, data, len, 100);
}

#ifdef __cplusplus
  }
#endif

#endif  // BNO055_STM32_H_
