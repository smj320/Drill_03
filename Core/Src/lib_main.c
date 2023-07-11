#include <sys/cdefs.h>
#include <stdlib.h>
//
// Created by kikuchi on 2023/05/20.
//
#include "main.h"
#include "string.h"
#include "time.h"
#include "FatFs.h"
#include "drill_mon.h"

/**
 * ペリフェラル用ハンドル
 */
extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;

/**
 * 後ろにあるやつ
 */
void make_HK(DRILL_STATUS *dst, uint8_t *fName);

/**
 * Drill用メインループ
 */
_Noreturn void drill_loop(DRILL_STATUS *dst) {
    //ファイル操作用
    FIL fil;
    uint8_t fName[16];
    uint8_t cc;

    //メインループ
    for (dst->isFirst = 1;;) {
        //1PPSトリガ待機とフラグリセット
        while (dst->F_PPS != 1);
        dst->F_PPS = 0;
        //HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, led++ & 0x01);

        //HKデータ編集
        make_HK(dst, fName);

        //UARTにデータ転送
        HAL_UART_Transmit_DMA(&huart2, dst->flm.buf, 80);

        //初回動作または時刻の切れ目でファイルオープン
        if ((dst->TI % FILE_RENEW_SEC) == 0 || dst->isFirst == 1) {
            //初回でなければクローズ
            if (dst->isFirst != 1) f_close(&fil);
            //オープン
            FRESULT result = f_open(&fil, (const char *) fName, FA_CREATE_ALWAYS | FA_WRITE);
            dst->fOpen = (result == FR_OK) ? 1 : 0;
        }

        //データをSDカードに出力する。
        UINT bw;
        f_write(&fil, dst->flm.buf, strlen((char *) dst->flm.buf), &bw);
        f_sync(&fil);

        //初回フラグクリア
        dst->isFirst = 0;
        //HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_RESET);

    }
}

//*********************************************
//ファイルシステム初期化関数
//*********************************************
/**
 * ファイルシステムマウント
 * @param dst
 */
void FS_Init(DRILL_STATUS *dst) {
    static FATFS Fs;
    static FRESULT rtc;
    rtc = f_mount(&Fs, "/", 1);
    dst->fMount = (rtc == FR_OK) ? 1 : 0;
}

//*********************************************
//周期割込
//*********************************************
/**
 * 1PPS用カウンタ。プリスケーラ4000-1,カウント1で1msec周期になる
 */
static int ti = 0;

void PPS_Tick(DRILL_STATUS *dst) {
    //1PPSカウント
    if (ti++ == 1000) {
        dst->F_PPS = 1;
        dst->TI++;
        ti = 0;
        HAL_GPIO_TogglePin(CPU_MON_GPIO_Port, CPU_MON_Pin);
    }
}
/**
 * ピン操作テスト
 */
#if 0

void check_GPIO() {
    static uint8_t rtc;
    HAL_GPIO_WritePin(SSPI_CS_GPIO_Port, SSPI_CS_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SSPI_CS_GPIO_Port, SSPI_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SSPI_CLK_GPIO_Port, SSPI_CLK_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SSPI_CLK_GPIO_Port, SSPI_CLK_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SSPI_MOSI_GPIO_Port, SSPI_MOSI_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SSPI_MOSI_GPIO_Port, SSPI_MOSI_Pin, GPIO_PIN_RESET);
    rtc = HAL_GPIO_ReadPin(SSPI_MISO_GPIO_Port, SSPI_MISO_Pin);
    rtc = HAL_GPIO_ReadPin(SSPI_MISO_GPIO_Port, SSPI_MISO_Pin);
}

#endif

/**
 * HKデータ作成
 * @param isFirst
 * @param hk_data
 * @param fname
 */
void make_HK(DRILL_STATUS *dst, uint8_t *fname) {

    uint8_t sum, i;

    //ファイル名
    itoa(dst->TI, (char *) fname, 16);
    strcat((char *) fname, ".bin");

    //バッファクリア
    memset(dst->flm.buf,0,sizeof(dst->flm.buf));

    //フレームシンクワード
    dst->flm.elm.FS = __builtin_bswap32(0xEB9038C7);

    //フレームカウンタ
    dst->flm.elm.TI = __builtin_bswap32(dst->TI);

    //ファイル状態
    dst->flm.elm.STAT = ((dst->fMount & 0x01) << 1) + ((dst->fOpen & 0x01) < 0);

    //位置指定
    dst->flm.elm.PDU_V = 3;
    dst->flm.elm.DMY1 = 4;
    dst->flm.elm.BAT_V = 5;
    dst->flm.elm.BAT_T = 6;
    dst->flm.elm.SYS_T = 7;
    dst->flm.elm.SYS_H = 8;
    dst->flm.elm.SYS_P = 9;
    dst->flm.elm.GND_P = __builtin_bswap16(10);
    dst->flm.elm.MOT_V = 11;
    dst->flm.elm.MOT_I = 12;
    dst->flm.elm.MOT_T = 13;
    dst->flm.elm.GEA_T = 14;
    dst->flm.elm.MOT_R = 15;
    dst->flm.elm.LIQ1_T = __builtin_bswap16(16);
    dst->flm.elm.LIQ1_P = __builtin_bswap16(17);
    dst->flm.elm.LIQ2_T = __builtin_bswap16(18);
    dst->flm.elm.BOA_D = __builtin_bswap16(19);
    dst->flm.elm.GRA_X = __builtin_bswap16(20);
    dst->flm.elm.GRA_Y = __builtin_bswap16(21);
    dst->flm.elm.GRA_Z = __builtin_bswap16(22);
    dst->flm.elm.ACC_X = __builtin_bswap16(23);
    dst->flm.elm.ACC_Y = __builtin_bswap16(24);
    dst->flm.elm.ACC_Z = __builtin_bswap16(25);
    dst->flm.elm.ROT_X = __builtin_bswap16(26);
    dst->flm.elm.ROT_Y = __builtin_bswap16(27);
    dst->flm.elm.ROT_Z = __builtin_bswap16(28);
    dst->flm.elm.MAG_X = __builtin_bswap16(29);
    dst->flm.elm.MAG_Y = __builtin_bswap16(30);
    dst->flm.elm.MAG_Z = __builtin_bswap16(31);

    //チェックサム生成
    for (i = 1, sum = dst->flm.buf[0]; i <= N_FLAME - 2; i++) {
        sum += dst->flm.buf[i];
    };
    dst->flm.buf[N_FLAME - 1] = sum;
}