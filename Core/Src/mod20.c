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
    static int i;
    //バナー読み出し
    HAL_GPIO_WritePin(SD_RST_GPIO_Port, SD_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(SD_RST_GPIO_Port, SD_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(20); //リセット後20msec待機必須
    for (i = 0; i < 120; i++) {
        s = HAL_I2C_Master_Receive(hi2c1, MOD20_I2C_ADDR, &sRx[i], 1, 10);
        if (sRx[i] == 0x00) break;
    };
    //バージョン確認
    s = HAL_I2C_Master_Transmit(hi2c1, MOD20_I2C_ADDR, cmdVar, strlen(cmdVar), 10);
    HAL_Delay(1);
    for (i = 0; i < 120; i++) {
        s = HAL_I2C_Master_Receive(hi2c1, MOD20_I2C_ADDR, &sRx[i], 1, 10);
        if (sRx[i] == 0x00) break;
    };
    //初期化
    s = HAL_I2C_Master_Transmit(hi2c1, MOD20_I2C_ADDR, cmdInit, strlen(cmdInit), 10);
    int cnt = 0;
    uint8_t rcv;
    do {
        HAL_Delay(1);
        s = HAL_I2C_Master_Receive(hi2c1, MOD20_I2C_ADDR, &rcv, 1, 10);
        if (s == HAL_OK) sRx[cnt++] = rcv;
    } while (rcv != '\n');

    //オープン
    mod20_open(hi2c1, 50505);

    //データ書き込指示
    for (i = 0; i < 80; i++){
        sTx[i] = i%10;
    }
    mod20_write80byte(hi2c1, sTx);

    //クローズ
    mod20_close(hi2c1);
}

void mod20_open(I2C_HandleTypeDef *hi2c1, uint16_t nf) {
    char hex[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    int pos, odr, digit;
    volatile HAL_StatusTypeDef s;
    static char cmdOpen[] = "O 1W>M:\\F9ABCD.TXT\n";

    //ファイルオープンコマンド生成
    for (digit = nf, odr = 0; odr <= 4; odr++) {
        pos = 13 - odr;
        cmdOpen[pos] = hex[digit % 10];
        digit = digit / 10;
    }

    //オープン実行
    s = HAL_I2C_Master_Transmit(hi2c1, MOD20_I2C_ADDR, cmdOpen, strlen(cmdOpen), 10);
    for (int i = 0; i < 80; i++) {
        HAL_Delay(1);
        s = HAL_I2C_Master_Receive(hi2c1, MOD20_I2C_ADDR, &sRx[i], 1, 10);
        if (sRx[i] == 0x00) break;
    };
}

void mod20_close(I2C_HandleTypeDef *hi2c1) {
    volatile HAL_StatusTypeDef s;
    static char cmdCls[] = "C 1\n";
    int i;
    //クローズ
    step = 0;
    s = HAL_I2C_Master_Transmit(hi2c1, MOD20_I2C_ADDR, cmdCls, strlen(cmdCls) - 1, 10);
    for (i = 0; i < 80; i++) {
        HAL_Delay(1);
        s = HAL_I2C_Master_Receive(hi2c1, MOD20_I2C_ADDR, &sRx[i], 1, 10);
        if (sRx[i] == 0x00) break;
    };
    step = 1;
}

void mod20_write80byte(I2C_HandleTypeDef *hi2c1, uint8_t *buf) {
    //unsigned char cmdW80[] = "W 1>10\n"; //80byte書き込み,数はhexで
    unsigned char cmdHello[] = "W 1>B\n";
    unsigned char msgHello[] = "Hello world";
    volatile HAL_StatusTypeDef s;
    int i;
    //書き込み数コマンド送信
    step = 0;
    memset(sRx,0xFF,80);
    //s = HAL_I2C_Master_Transmit(hi2c1, MOD20_I2C_ADDR, cmdW80, strlen(cmdW80), 10);
    s = HAL_I2C_Master_Transmit(hi2c1, MOD20_I2C_ADDR, cmdHello, strlen(cmdHello), 10);
    //アンサー確認
    for (i = 0; i < 80; i++) {
        HAL_Delay(1);
        s = HAL_I2C_Master_Receive(hi2c1, MOD20_I2C_ADDR, &sRx[i], 1, 10);
        if (sRx[i] == 0x00) break;
    };

    //データ書込実行,16byte~30byteくらいならかける。32byteだとかけない。
    step = 1;
    memset(sRx,0xFF,80);
    //s = HAL_I2C_Master_Transmit(hi2c1, MOD20_I2C_ADDR, buf, 16, 10);
    s = HAL_I2C_Master_Transmit(hi2c1, MOD20_I2C_ADDR, msgHello, strlen(msgHello), 10);
    for (i = 0; i < 800; i++) {
        HAL_Delay(1);
        s = HAL_I2C_Master_Receive(hi2c1, MOD20_I2C_ADDR, &sRx[i%80], 1, 10);
        if (sRx[i%80] == 0x00) break;
    };

    step = 2;
}
