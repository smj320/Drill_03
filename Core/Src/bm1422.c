#include "main.h"
#include "bm1422.h"
#include "drill_mon.h"

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;

int BM1422_Init() {
    volatile HAL_StatusTypeDef s;
    uint8_t wia, sta1;
    uint8_t cnt1 = BM1422AGMV_CNTL1_VAL;
    uint16_t cnt4[2] = {0,0};;

    s = HAL_I2C_Mem_Read(&hi2c1, BM1422_ADR << 1, BM1422AGMV_WIA, 1, &wia, 1, 100);
    if (s != HAL_OK) return -1;
    s = HAL_I2C_Mem_Write(&hi2c1, BM1422_ADR << 1, BM1422AGMV_CNTL1, 1, &cnt1, 1, 100);
    if (s != HAL_OK) return -2;
    s = HAL_I2C_Mem_Write(&hi2c1, BM1422_ADR << 1, BM1422AGMV_CNTL4, 1, cnt4, 2, 100);
    if (s != HAL_OK) return -3;
    s = HAL_I2C_Mem_Read(&hi2c1, BM1422_ADR << 1, BM1422AGMV_STA1, 1, &sta1, 1, 100);
     if (s != HAL_OK) return -2;
    return 0;
}

uint8_t calc_sensitivity(void)
{
    uint8_t val = BM1422AGMV_CNTL1_VAL;
    uint8_t sensitivity;

    val &= BM1422AGMV_CNTL1_OUT_BIT_MASK;

    //Out_bit  14bit:24, 12bit:6
    sensitivity = (9 * (val >> 5)) + 6;

    return sensitivity;
}


int BM1422_getVal(int16_t xyz[]) {
    volatile HAL_StatusTypeDef s;
    uint8_t raw[6], sta1;
    uint8_t cnt3 = BM1422AGMV_CNTL3_VAL;
    //
    s = HAL_I2C_Mem_Write(&hi2c1, BM1422_ADR << 1, BM1422AGMV_CNTL3, 1, &cnt3, 1, 100);
    if (s != HAL_OK) return -1;
    HAL_Delay(10);
    s = HAL_I2C_Mem_Read(&hi2c1, BM1422_ADR << 1, BM1422AGMV_STA1, 1, &sta1, 1, 100);
    if (s != HAL_OK) return -2;
    s = HAL_I2C_Mem_Read(&hi2c1, BM1422_ADR << 1, BM1422AGMV_DATAX, 1, raw, 6, 100);
    if (s != HAL_OK) return -3;
    //24で割るとuTになる
    xyz[0] = raw[1] << 8 | raw[0];
    xyz[1] = raw[3] << 8 | raw[2];
    xyz[2] = raw[5] << 8 | raw[4];
    return s;
}

void BM1422_dump() {
    volatile HAL_StatusTypeDef s;
    int16_t xyz[3];

    while (1) {
        BM1422_getVal(xyz);
        HAL_Delay(100);
        Lib_dump_3f(DTP_MAG, (float)xyz[0]/24.0, (float)xyz[1]/24.0, (float)xyz[2]/24.0);
        HAL_Delay(700);
    }
}