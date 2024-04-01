//
// Created by kikuchi on 2024/03/27.
//
#include <string.h>
#include "mod20.h"

/**
 * FAT32の場合、ルートには65517個のファイルがおける。
 * 10分おきに１つファイルを作ると1ファイル47kByte
 * 60000で0にロールアップさせると2.6G, 416日
 */

/**
 * リターンチェック
 * @param hi2c1
 * @param nWait
 * @param _nlf
 * @return
 */
int check_rtc(I2C_HandleTypeDef *hi2c1, int nWait, int _nlf)
{
    volatile HAL_StatusTypeDef s;
    uint8_t cc, rtcBuf[N_RTC_BUF];
    static int i, xp, nBusError, nlf, debug=0;
    //リセット後バナー取得,0x0Aは!00の後を入れて5回
    memset(rtcBuf, 0xFF, N_RTC_BUF);
    for (nBusError=0, nlf = 0, i = 0; i < nWait; i++) {
        HAL_Delay( 1);
        xp = i % N_RTC_BUF;
        s = HAL_I2C_Master_Receive(hi2c1, MOD20_I2C_ADDR, &cc, 1, I2C_TIMEOUT);
        if(s!=HAL_OK) nBusError++;
        if (cc == 0x00) continue;   //0x00ならスキップ
        rtcBuf[xp] = cc;            //0x00以外ならリングバッファに格納
        if (cc == 0x0A) nlf++;  //LFならカウント
        if (nlf == _nlf) break;    //LFが規定数なら終了
        if(++xp==N_RTC_BUF) xp=0; //リングバッファのロールアップ
    };
    if(i==nWait){//タイムアウト
        debug = 1;
        return -1;
    }
    if(nBusError!=0){//バスエラー
        debug = 2;
        return -2;
    }
    if(rtcBuf[xp-3]!='!' || rtcBuf[xp-2]!='0' || rtcBuf[xp-1]!='0'){ //エラーコードあり
        debug = 3;
        return -3;
    }
    return 0;
}

/**
 * ボード初期化
 * @param hi2c1
 * @return
 */
int mod20_Init(I2C_HandleTypeDef *hi2c1) {
    static char cmdInit[] = "I M:\n";
    volatile HAL_StatusTypeDef s;
    static int rtc;

    //リセット信号
    HAL_GPIO_WritePin(SD_RST_GPIO_Port, SD_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(SD_RST_GPIO_Port, SD_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(20); //リセット後20msec待機必須

    //バナー取得,0x0Aは!00の後を入れて5回
    rtc = check_rtc(hi2c1, 2000, 5);
    if(rtc!=0) return -1;

    //初期化コマンド送信
    s = HAL_I2C_Master_Transmit(hi2c1, MOD20_I2C_ADDR, cmdInit, strlen(cmdInit), I2C_TIMEOUT);

    //リターンチェック,LFは一回
    rtc = check_rtc(hi2c1, 2000, 1);
    if(rtc!=0) return -2;

#if 0
    //オープン
    mod20_open(hi2c1, 10000);

    //テストデータ書き込
    static uint8_t sTx[80];
    memset(sTx,0xFF,80);
    mod20_write80byte(hi2c1, sTx);

    //クローズ
    mod20_close(hi2c1);

    //待機
    while(1);
#endif

    //初期化正常終了
    return 0;
}

int mod20_open(I2C_HandleTypeDef *hi2c1, uint16_t nf) {
    char hex[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    int pos, odr, digit;
    volatile HAL_StatusTypeDef s;
    static char cmdOpen[] = "O 0W>M:\\F9ABCD.TXT\n";
    static int rtc;

    //ファイルオープンコマンド生成
    for (digit = nf, odr = 0; odr <= 4; odr++) {
        pos = 13 - odr;
        cmdOpen[pos] = hex[digit % 10];
        digit = digit / 10;
    }

    //オープンコマンド送信
    s = HAL_I2C_Master_Transmit(hi2c1, MOD20_I2C_ADDR, cmdOpen, strlen(cmdOpen), I2C_TIMEOUT);

    //リターンチェック,LFは一回
    rtc = check_rtc(hi2c1, 2000, 1);
    if(rtc!=0) return -1;

    //初期化正常終了
    return 0;
}

int mod20_write16byte(I2C_HandleTypeDef *hi2c1, uint8_t *buf) {
    unsigned char cmdW16[] = "W 0>10\n"; //一回で16byteは書けるが32byteは書けない
    volatile HAL_StatusTypeDef s;
    static int rtc;

    //書き込み数コマンド送信
    s = HAL_I2C_Master_Transmit(hi2c1, MOD20_I2C_ADDR, cmdW16, strlen(cmdW16), I2C_TIMEOUT);

    //リターンチェック,LFは一回
    rtc = check_rtc(hi2c1, 2000, 1);
    if(rtc!=0) return -10 + rtc;

    //データ書込コマンド送信
    s = HAL_I2C_Master_Transmit(hi2c1, MOD20_I2C_ADDR, buf, 16, I2C_TIMEOUT);

    //リターンチェック,LFは一回
    rtc = check_rtc(hi2c1, 1000, 2);
    if(rtc!=0) return -10 + rtc;

    //初期化正常終了
    return 0;
}

int mod20_write80byte(I2C_HandleTypeDef *hi2c1, uint8_t *buf) {
    volatile HAL_StatusTypeDef s;
    static char cmdFlush[] = "F 0\n";
    static int rtc;
    //HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, 1);

    //書き込み
    for(int i=0;i<5;i++) mod20_write16byte(hi2c1, &buf[0x10*i]);

    //フラッシュコマンド送信
    s = HAL_I2C_Master_Transmit(hi2c1, MOD20_I2C_ADDR, cmdFlush, strlen(cmdFlush), I2C_TIMEOUT);

    //リターンチェック,LFは一回
    rtc = check_rtc(hi2c1, 1000, 1);
    //HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, 0);
    if(rtc!=0) return -10 + rtc;
}

int mod20_close(I2C_HandleTypeDef *hi2c1) {
    volatile HAL_StatusTypeDef s;
    static char cmdCls[] = "C 0\n";
    static int rtc;

    //クローズコマンド送信
    s = HAL_I2C_Master_Transmit(hi2c1, MOD20_I2C_ADDR, cmdCls, strlen(cmdCls), I2C_TIMEOUT);

    //リターンチェック,LFは一回
    rtc = check_rtc(hi2c1, 1000, 1);
    if(rtc!=0) return rtc;

    //初期化正常終了
    return 0;
}