#include <sys/cdefs.h>
#include <stdlib.h>
//
// Created by kikuchi on 2023/05/20.
//
#include "main.h"
#include "string.h"
#include "time.h"
#include "drill_mon.h"
#include "bme280.h"
#include "mcp3424.h"
#include "bno055.h"
#include "bm1422.h"
#include <stdio.h>
#include <math.h>

/**
 * ペリフェラル用ハンドル
 */
extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;
extern struct bme280_dev bme_dev;
extern struct bme280_data comp_data;

/**
 * 後ろにあるやつ
 */
void make_HK(DRILL_STATUS *dst, uint8_t *fName);

/**
 * Drill用メインループ
 */
_Noreturn void drill_loop(DRILL_STATUS *dst) {
    //ファイル操作用
    uint8_t fName[16];
    uint8_t txBuf[N_FLAME];
    uint8_t cc;

    //メインループ
    for (dst->isFirst = 1;;) {
        //1PPSトリガ待機とフラグリセット
        while (dst->F_PPS != 1);
        dst->F_PPS = 0;
        //HAL_GPIO_TogglePin(CPU_MON_GPIO_Port, CPU_MON_Pin);

        //HKデータ編集
        make_HK(dst, fName);

        //UARTにデータ転送
        memcpy(txBuf,dst->flm.buf,N_FLAME);
        HAL_UART_Transmit_DMA(&huart2, txBuf, N_FLAME);

        //初回動作または時刻の切れ目でファイルオープン
#if 0
        if ((dst->TI % FILE_RENEW_SEC) == 0 || dst->isFirst == 1) {
            //初回でなければクローズ
            if (dst->isFirst != 1) f_close(&fil);
            //オープン
            FRESULT result = f_open(&fil, (const char *) fName, FA_CREATE_ALWAYS | FA_WRITE);
            dst->fOpen = (result == FR_OK) ? 1 : 0;
        }

        //データをSDカードに出力する。
        UINT bw;
        f_write(&fil, dst->flm.buf, N_FLAME, &bw);
        f_sync(&fil);
#endif
        //初回フラグクリア
        dst->isFirst = 0;
    }
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
        //HAL_GPIO_TogglePin(CPU_MON_GPIO_Port, CPU_MON_Pin);
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
    static uint16_t data;
    bno055_vector_t v;
    int16_t mag_xyz[3];

    //******計測スタート
    HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_SET);

    //ファイル名
    //sprintf(fname,"%08d.bin",dst->TI/FILE_RENEW_SEC);
    itoa(dst->TI / FILE_RENEW_SEC + 10000000, (char *) fname, 10);
    strcat((char *) fname, ".bin");

    //バッファクリア
    memset(dst->flm.buf, 0, sizeof(dst->flm.buf));

    //フレームシンクワード
    dst->flm.elm.FS = __builtin_bswap32(0xEB9038C7);

    //フレームカウンタ
    dst->flm.elm.TI = __builtin_bswap32(dst->TI);

    //ファイル状態
    dst->flm.elm.STAT = ((dst->fOpen & 0x01) << 1) | ((dst->fMount) & 0x01);

    //位置指定
    dst->flm.elm.PDU_V = 3;
    dst->flm.elm.DMY1 = 4;
    dst->flm.elm.BAT_V = 5;
    dst->flm.elm.BAT_T = 6;
    dst->flm.elm.SYS_T = 7;
    dst->flm.elm.SYS_H = 8;
    dst->flm.elm.SYS_P = 9;
    dst->flm.elm.GND_P = 10;
    dst->flm.elm.MOT_V = 11;
    dst->flm.elm.MOT_I = 12;
    dst->flm.elm.MOT_T = 13;
    dst->flm.elm.GEA_T = 14;
    dst->flm.elm.MOT_R = 15;
    dst->flm.elm.LIQ1_T = 16;
    dst->flm.elm.LIQ1_P = 17;
    dst->flm.elm.LIQ2_T = 18;
    dst->flm.elm.BOA_D = 19;
    dst->flm.elm.GRA_X = 20;
    dst->flm.elm.GRA_Y = 21;
    dst->flm.elm.GRA_Z = 22;
    dst->flm.elm.ACC_X = 23;
    dst->flm.elm.ACC_Y = 24;
    dst->flm.elm.ACC_Z = 25;
    dst->flm.elm.ROT_X = 26;
    dst->flm.elm.ROT_Y = 27;
    dst->flm.elm.ROT_Z = 28;
    dst->flm.elm.MAG_X = 29;
    dst->flm.elm.MAG_Y = 30;
    dst->flm.elm.MAG_Z = 31;

#if 1
    //気温・湿度・圧力
    bme280_set_sensor_mode(BME280_FORCED_MODE, &bme_dev);
    HAL_Delay(40);
    if (bme280_get_sensor_data(BME280_ALL, &comp_data, &bme_dev) == 0) {
        dst->flm.elm.SYS_T = comp_data.temperature / 100.0;
        dst->flm.elm.SYS_H = comp_data.humidity / 1024.0;
        double atm = ((double)comp_data.pressure / 10000.0)/1024.0;
        dst->flm.elm.SYS_P = (u_int8_t)(round(atm));
    }
#endif

#if 0
    //加速度
    v = bno055_getVectorGravity();  //Unit m/s^2
    dst->flm.elm.GRA_X = v.x*100;
    dst->flm.elm.GRA_Y = v.y*100;
    dst->flm.elm.GRA_Z = v.z*100;
    v = bno055_getVectorGyroscope(); //Unit deg/sec
    dst->flm.elm.ROT_X = v.x*100;
    dst->flm.elm.ROT_Y = v.y*100;
    dst->flm.elm.ROT_Z = v.z*100;
    v = bno055_getVectorLinearAccel(); //Unit m/s^2
    dst->flm.elm.ACC_X = v.x*100;
    dst->flm.elm.ACC_Y = v.y*100;
    dst->flm.elm.ACC_Z = v.z*100;
#endif

#if 1
    //磁場 *0.042で uT
    BM1422_read_mag(mag_xyz);
    dst->flm.elm.MAG_X = mag_xyz[0];
    dst->flm.elm.MAG_Y = mag_xyz[1];
    dst->flm.elm.MAG_Z = mag_xyz[2];

    //ここまで80ms
    //HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_RESET);
#endif

#if 1
    //MCP3424 MCP3424_HV_ADDR, ad0=0,ad1=0
    //MCP3424 MCP3424_PT100_ADDR, ad0=1,ad1=0
    //MCP3424 MCP3424_PT100_ADDR, d0=0,ad1=1
    //変換時間に66msかかるので、ICごとに指令をずらして出す
    MCP3424_Ask(MCP3424_HV_ADDR, MOT_V_CH);
    MCP3424_Ask(MCP3424_PT100_ADDR, BAT_T_CH);
    MCP3424_Ask(MCP3424_LVDT_ADDR, GND_P_CH);
    HAL_Delay(70);
    if (MCP3424_Ans(MCP3424_HV_ADDR, &data) == 0) dst->flm.elm.MOT_V = data >> 6;
    if (MCP3424_Ans(MCP3424_PT100_ADDR, &data) == 0) dst->flm.elm.BAT_T = data >> 6;
    if (MCP3424_Ans(MCP3424_LVDT_ADDR, &data) == 0) dst->flm.elm.GND_P = data >> 6;
    //
    MCP3424_Ask(MCP3424_HV_ADDR, MOT_I_CH);
    MCP3424_Ask(MCP3424_PT100_ADDR, LIQ2_T_CH);
    MCP3424_Ask(MCP3424_LVDT_ADDR, BAT_V_CH);
    HAL_Delay(70);
    if (MCP3424_Ans(MCP3424_HV_ADDR, &data) == 0) dst->flm.elm.MOT_I = data >> 6;
    if (MCP3424_Ans(MCP3424_PT100_ADDR, &data) == 0) dst->flm.elm.LIQ2_T = data;
    if (MCP3424_Ans(MCP3424_LVDT_ADDR,  &data) == 0) dst->flm.elm.BAT_V = data >> 6;
    //
    MCP3424_Ask(MCP3424_HV_ADDR, MOT_R_CH);
    MCP3424_Ask(MCP3424_PT100_ADDR, MOT_T_CH);
    MCP3424_Ask(MCP3424_LVDT_ADDR, LIQ1_P_CH);
    HAL_Delay(70);
    if (MCP3424_Ans(MCP3424_HV_ADDR, &data) == 0) dst->flm.elm.MOT_R = data >> 6;
    if (MCP3424_Ans(MCP3424_PT100_ADDR, &data) == 0) dst->flm.elm.MOT_T = data >> 6;
    if (MCP3424_Ans(MCP3424_LVDT_ADDR,  &data) == 0) dst->flm.elm.LIQ1_P = data;
    //
    MCP3424_Ask(MCP3424_HV_ADDR, PDU_V_CH);
    MCP3424_Ask(MCP3424_PT100_ADDR, GEA_T_CH);
    MCP3424_Ask(MCP3424_LVDT_ADDR, LIQ1_T_CH);
    HAL_Delay(70);
    if (MCP3424_Ans(MCP3424_HV_ADDR, &data) == 0) dst->flm.elm.PDU_V = data >> 6;
    if (MCP3424_Ans(MCP3424_PT100_ADDR, &data) == 0) dst->flm.elm.GEA_T = data >> 6;
    if (MCP3424_Ans(MCP3424_LVDT_ADDR, &data) == 0) dst->flm.elm.LIQ1_T = data;
#endif

    //エンコード
    dst->flm.elm.GND_P = __builtin_bswap16(dst->flm.elm.GND_P);
    dst->flm.elm.LIQ1_T = __builtin_bswap16(dst->flm.elm.LIQ1_T);
    dst->flm.elm.LIQ1_P = __builtin_bswap16(dst->flm.elm.LIQ1_P);
    dst->flm.elm.LIQ2_T = __builtin_bswap16(dst->flm.elm.LIQ2_T);
    dst->flm.elm.BOA_D = __builtin_bswap16(dst->flm.elm.BOA_D);
    dst->flm.elm.GRA_X = __builtin_bswap16(dst->flm.elm.GRA_X);
    dst->flm.elm.GRA_Y = __builtin_bswap16(dst->flm.elm.GRA_Y);
    dst->flm.elm.GRA_Z = __builtin_bswap16(dst->flm.elm.GRA_Z);
    dst->flm.elm.ACC_X = __builtin_bswap16(dst->flm.elm.ACC_X);
    dst->flm.elm.ACC_Y = __builtin_bswap16(dst->flm.elm.ACC_Y);
    dst->flm.elm.ACC_Z = __builtin_bswap16(dst->flm.elm.ACC_Z);
    dst->flm.elm.ROT_X = __builtin_bswap16(dst->flm.elm.ROT_X);
    dst->flm.elm.ROT_Y = __builtin_bswap16(dst->flm.elm.ROT_Y);
    dst->flm.elm.ROT_Z = __builtin_bswap16(dst->flm.elm.ROT_Z);
    dst->flm.elm.MAG_X = __builtin_bswap16(dst->flm.elm.MAG_X);
    dst->flm.elm.MAG_Y = __builtin_bswap16(dst->flm.elm.MAG_Y);
    dst->flm.elm.MAG_Z = __builtin_bswap16(dst->flm.elm.MAG_Z);

    //チェックサム生成
    for (i = 1, sum = dst->flm.buf[0]; i <= N_FLAME - 2; i++) {
        sum += dst->flm.buf[i];
    };
    dst->flm.buf[N_FLAME - 1] = sum;

    //******計測終了
    HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_RESET);
}

void Lib_dump_3f(int type, float x, float y, float z) {
    char msg[64];
    char fmt[32];
    switch (type) {
        case DTP_MAG:
            strcpy(fmt, "mag x:%f y:%f z:%f\r\n");
            break;
        case DTP_GRA:
            strcpy(fmt, "grav x:%f y:%f z:%f\r\n");
            break;
        case DTP_GAY:
            strcpy(fmt, "gyro x:%f y:%f z:%f\r\n");
            break;
        case DTP_ACC:
            strcpy(fmt, "acc x:%f y:%f z:%f\r\n");
            break;
        case DTP_HUM:
            strcpy(fmt, "tmp  t:%f hum:%f atm:%f\r\n");
            break;
        default:
            strcpy(fmt, "error\r\n");
    }
    sprintf(msg, fmt, x, y, z);
    HAL_UART_Transmit_DMA(&huart2, msg, strlen(msg));
}

void Lib_dump_ad(int8_t rtc[], uint16_t data[]) {
    char msg[64];
    sprintf(msg, "ad %d:%04X %d:%04X %d:%04X %d:%04X\r\n",
            rtc[0], data[0], rtc[1], data[1], rtc[2], data[2], rtc[3], data[3]);
    HAL_UART_Transmit_DMA(&huart2, msg, strlen(msg));
}