//
// Created by kikuchi on 2024/03/27.
//
#include <string.h>
#include "mod20.h"

static uint8_t sTx[80], sRx[80];
volatile int step;

void mod20_Init(I2C_HandleTypeDef *hi2c1) {
    static char cmdVar[] = "V\n";
    static char cmdInit[] = "I M:\n";
    volatile HAL_StatusTypeDef s;
    static int i, xp, nlf;

    //リセット信号
    HAL_GPIO_WritePin(SD_RST_GPIO_Port, SD_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(SD_RST_GPIO_Port, SD_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(20); //リセット後20msec待機必須

    //リセット後バナー取得,0x0Aは!00の後を入れて5回
    memset(sRx, 0xFF, 80);
    for (nlf = 0, i = 0; i < 80; i++) {
        HAL_Delay(1);
        xp = i % 80;
        s = HAL_I2C_Master_Receive(hi2c1, MOD20_I2C_ADDR, &sRx[xp], 1, 10);
        if (sRx[xp] == 0x0A) nlf++;
        if (nlf == 5) break;
    };

    //バージョン確認,0x0Aは!00の後を入れて2回
    s = HAL_I2C_Master_Transmit(hi2c1, MOD20_I2C_ADDR, cmdVar, strlen(cmdVar), 10);
    memset(sRx, 0xFF, 80);
    for (nlf = 0, i = 0; i < 80; i++) {
        HAL_Delay(1);
        xp = i % 80;
        s = HAL_I2C_Master_Receive(hi2c1, MOD20_I2C_ADDR, &sRx[xp], 1, 10);
        if (sRx[xp] == 0x0A) nlf++;
        if (nlf == 2) break;
    };

    //初期化,0x0Aは!00の後を入れて1回
    s = HAL_I2C_Master_Transmit(hi2c1, MOD20_I2C_ADDR, cmdInit, strlen(cmdInit), 10);
    memset(sRx, 0xFF, 80);
    for (nlf = 0, i = 0; i < 1000; i++) {
        HAL_Delay(100);
        xp = i % 80;
        s = HAL_I2C_Master_Receive(hi2c1, MOD20_I2C_ADDR, &sRx[xp], 1, 10);
        if (sRx[xp] == 0x0A) nlf++;
        if (nlf == 1) break;
    };

#if 0
    //オープン
    mod20_open(hi2c1, 10000);

    //データ書き込指示
    for(i=0;i<80;i++) sTx[i] = i;
    //mod20_write16byte(hi2c1, sTx);
    mod20_write80byte(hi2c1, sTx);

    //クローズ
    mod20_close(hi2c1);
#endif
}

void mod20_open(I2C_HandleTypeDef *hi2c1, uint16_t nf) {
    char hex[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    int pos, odr, digit;
    volatile HAL_StatusTypeDef s;
    static char cmdOpen[] = "O 0W>M:\\F9ABCD.TXT\n";
    static int i, xp, nlf;
    HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, 1);

    //ファイルオープンコマンド生成
    for (digit = nf, odr = 0; odr <= 4; odr++) {
        pos = 13 - odr;
        cmdOpen[pos] = hex[digit % 10];
        digit = digit / 10;
    }

    step = 0;
    //オープン実行,LFは1
    s = HAL_I2C_Master_Transmit(hi2c1, MOD20_I2C_ADDR, cmdOpen, strlen(cmdOpen), 10);
    memset(sRx, 0xFF, 80);
    for (nlf = 0, i = 0; i < 20000; i++) {
        HAL_Delay(1);
        xp = i % 80;
        s = HAL_I2C_Master_Receive(hi2c1, MOD20_I2C_ADDR, &sRx[xp], 1, 10);
        if (sRx[xp] == 0x0A) nlf++;
        if (nlf == 1) break;
    };
    HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, 0);
}

void mod20_write16byte(I2C_HandleTypeDef *hi2c1, uint8_t *buf) {
    unsigned char cmdW16[] = "W 0>10\n"; //一回で16byteは書けるが32byteは書けない
    volatile HAL_StatusTypeDef s;
    static int i, xp, nlf;
    HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, 1);
    //書き込み数コマンド送信
    s = HAL_I2C_Master_Transmit(hi2c1, MOD20_I2C_ADDR, cmdW16, strlen(cmdW16), 10);

    //アンサー確認,LFは1
    memset(sRx, 0xFF, 80);
    for (nlf = 0, i = 0; i < 1000; i++) {
        HAL_Delay(1);
        xp = i % 80;
        s = HAL_I2C_Master_Receive(hi2c1, MOD20_I2C_ADDR, &sRx[xp], 1, 10);
        if (sRx[xp] == 0x0A) nlf++;
        if (nlf == 1) break;
    };

    //データ書込実行,16byte~30byteくらいならかける。32byteだとかけない。
    s = HAL_I2C_Master_Transmit(hi2c1, MOD20_I2C_ADDR, buf, 16, 10);
    //s = HAL_I2C_Master_Transmit(hi2c1, MOD20_I2C_ADDR, msgHello, strlen(msgHello), 10);
    memset(sRx, 0xFF, 80);
    for (nlf = 0, i = 0; i < 100000; i++) {
        HAL_Delay(1);
        xp = i % 80;
        s = HAL_I2C_Master_Receive(hi2c1, MOD20_I2C_ADDR, &sRx[xp], 1, 10);
        if (sRx[xp] == 0x0A) nlf++;
        if (nlf == 2) break;
    };
    HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, 0);
}

void mod20_write80byte(I2C_HandleTypeDef *hi2c1, uint8_t *buf) {
    volatile HAL_StatusTypeDef s;
    static char cmdFlush[] = "F 0\n";
    static int i, xp, nlf;

    mod20_write16byte(hi2c1, &buf[0x00]);
    mod20_write16byte(hi2c1, &buf[0x10]);
    mod20_write16byte(hi2c1, &buf[0x20]);
    mod20_write16byte(hi2c1, &buf[0x30]);
    mod20_write16byte(hi2c1, &buf[0x40]);

    //フラッシュ
    memset(sRx, 0xFF, 80);
    s = HAL_I2C_Master_Transmit(hi2c1, MOD20_I2C_ADDR, cmdFlush, strlen(cmdFlush), 10);
    memset(sRx, 0xFF, 80);
    for (nlf = 0, i = 0; i < 20000; i++) {
        HAL_Delay(1);
        xp = i % 80;
        s = HAL_I2C_Master_Receive(hi2c1, MOD20_I2C_ADDR, &sRx[xp], 1, 10);
        if (sRx[xp] == 0x0A) nlf++;
        if (nlf == 1) break;
    };
}

void mod20_close(I2C_HandleTypeDef *hi2c1) {
    volatile HAL_StatusTypeDef s;
    static char cmdCls[] = "C 0\n";
    static int i, xp, nlf;
    HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, 1);
    //クローズ,LFは一回
    memset(sRx, 0xFF, 80);
    s = HAL_I2C_Master_Transmit(hi2c1, MOD20_I2C_ADDR, cmdCls, strlen(cmdCls), 10);
    memset(sRx, 0xFF, 80);
    for (nlf = 0, i = 0; i < 20000; i++) {
        HAL_Delay(1);
        xp = i % 80;
        s = HAL_I2C_Master_Receive(hi2c1, MOD20_I2C_ADDR, &sRx[xp], 1, 10);
        if (sRx[xp] == 0x0A) nlf++;
        if (nlf == 1) break;
    };
    HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, 0);
}