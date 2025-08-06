#include <sys/cdefs.h>
#include <stdlib.h>
//
// Created by kikuchi on 2023/05/20.
//
#include "main.h"
#include "string.h"
#include "drill_mon.h"
#include "bme280.h"
#include "mcp3424.h"
#include "bno055.h"
#include "bm1422.h"
#include "mod20.h"
#include "cputemp.h"
#include <stdio.h>
#include <math.h>

/**
 * ペリフェラル用ハンドルと外部変数
 */
extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;
extern struct bme280_dev bme_dev;
extern struct bme280_data comp_data;
extern uint8_t F_STAT;

/**
 * 後ろにあるやつ
 */
void make_HK(DRILL_STATUS *dst);

/**
 * Drill用メインループ
 */
_Noreturn void drill_loop(DRILL_STATUS *dst) {
    //ファイル操作用
    static int nf = 0;
    uint8_t fName[16];
    uint8_t txBuf[N_FLAME];
    uint8_t cc;

    //ファイルオープン,reopenでタイムアウトになるので基本閉じない
    //初期化が失敗しているときは処理をスキップする。
    if (F_STAT & ST_SD_INIT) {
        F_STAT |= ST_SD_OPEN;
    } else {
        F_STAT = mod20_open(&hi2c1, nf) ? F_STAT | ST_SD_OPEN : F_STAT & ~ST_SD_OPEN;
    }

    //メインループ
    while (1) {
        //1PPSトリガ待機とフラグリセット
        while (dst->F_PPS != 1);
        dst->F_PPS = 0;
        HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, 1);

        //HKデータ編集
        make_HK(dst);

        //UARTにデータ転送
        memcpy(txBuf, dst->flm.buf, N_FLAME);
        HAL_UART_Transmit_DMA(&huart2, txBuf, N_FLAME);

        //ST_SD_OPENが0のときデータをSDカードに出力する。
        if (!(F_STAT & ST_SD_OPEN)) {
            F_STAT = mod20_write80byte(&hi2c1, txBuf) ? F_STAT | ST_SD_WRITE : F_STAT & ~ST_SD_WRITE;
        }

        //後始末
        dst->TI++;
        nf = (nf == FILE_ROLLUP - 1) ? 0 : nf + 1;
        HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, 0);
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
    //1PPSカウントアップ
    if (ti++ == 1000) {
        dst->F_PPS = 1;
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
 */
static int16_t cc = -127 * 64;

void make_HK(DRILL_STATUS *dst) {

    uint8_t sum, i;
    static int16_t data;
    bno055_vector_t v;
    int16_t mag_xyz[3];
    static double absG, grav0[3] = {0.0, 0.0, 0.0};

    //******計測スタート
    //HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_SET);

    //バッファクリア
    memset(dst->flm.buf, 0, sizeof(dst->flm.buf));

    //フレームシンクワード
    dst->flm.elm.FS = __builtin_bswap32(0xEB9038C7);

    //フレームカウンタ
    dst->flm.elm.TI = dst->TI;

    //システム状態
    int8_t is_butyl = HAL_GPIO_ReadPin(BUTYL_GPIO_Port, BUTYL_Pin);
    dst->flm.elm.STAT = F_STAT+(((~is_butyl)&0x01) << 2);

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
    dst->flm.elm.PAD1 = 0x55;
    dst->flm.elm.LIQ1_T = 17;
    dst->flm.elm.LIQ1_P = 18;
    dst->flm.elm.LIQ2_T = 19;
    dst->flm.elm.BOA1_D = 20;
    dst->flm.elm.GRA_X = 21;
    dst->flm.elm.GRA_Y = 22;
    dst->flm.elm.GRA_Z = 23;
    dst->flm.elm.ACC_X = 24;
    dst->flm.elm.ACC_Y = 25;
    dst->flm.elm.ACC_Z = 26;
    dst->flm.elm.ROT_X = 27;
    dst->flm.elm.ROT_Y = 28;
    dst->flm.elm.ROT_Z = 29;
    dst->flm.elm.MAG_X = 30;
    dst->flm.elm.MAG_Y = 31;
    dst->flm.elm.MAG_Z = 32;
    dst->flm.elm.BOA2_D = 30;
    dst->flm.elm.LIQ3_T = 31;
    dst->flm.elm.LIQ4_T = 32;
    dst->flm.elm.SYS_T2 = 33;
    dst->flm.elm.SYS_P2 = 34;

#if 1
    //気温・湿度・圧力
    bme280_set_sensor_mode(BME280_FORCED_MODE, &bme_dev);
    HAL_Delay(40);
    if (bme280_get_sensor_data(BME280_ALL, &comp_data, &bme_dev) == 0) {
        dst->flm.elm.SYS_T = comp_data.temperature / 100.0;
        dst->flm.elm.SYS_H = comp_data.humidity / 1024.0;
        double atm = ((double) comp_data.pressure / 10000.0) / 1024.0;
        dst->flm.elm.SYS_P = (uint8_t) (round(atm));
    }
#endif

#if 1
    //重力加速度
    //重力加速度の絶対値が8.0~12.0の範囲外なら前の値を使う。
    //ドリル座標系はX=y, Y=-x, Z=-z
    v = bno055_getVectorGravity();  //Unit m/s^2
    absG = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (8.0 < absG && absG < 12.0) {
        dst->flm.elm.GRA_X = (int16_t) (v.y * 100);
        dst->flm.elm.GRA_Y = (int16_t) (-v.x * 100);
        dst->flm.elm.GRA_Z = (int16_t) (-v.z * 100);
        grav0[0] = v.y;
        grav0[1] = -v.x;
        grav0[2] = -v.z;
    } else {
        dst->flm.elm.GRA_X = (int16_t) (grav0[0] * 100);
        dst->flm.elm.GRA_Y = (int16_t) (grav0[1] * 100);
        dst->flm.elm.GRA_Z = (int16_t) (grav0[2] * 100);
    }

    HAL_Delay(10);
    //角速度
    v = bno055_getVectorGyroscope(); //Unit deg/sec
    dst->flm.elm.ROT_X = (int16_t) (v.y * 100);
    dst->flm.elm.ROT_Y = (int16_t) (-v.x * 100);
    dst->flm.elm.ROT_Z = (int16_t) (-v.z * 100);
    HAL_Delay(10);
    //並進加速度
    v = bno055_getVectorLinearAccel(); //Unit m/s^2
    dst->flm.elm.ACC_X = (int16_t) (v.y * 100);
    dst->flm.elm.ACC_Y = (int16_t) (-v.x * 100);
    dst->flm.elm.ACC_Z = (int16_t) (-v.z * 100);
    HAL_Delay(10);
#endif

#if 1
    //磁場, 24で割るとuTになる
    //ドリル座標系はX=y, Y=-x, Z=z
    BM1422_getVal(mag_xyz);
    dst->flm.elm.MAG_X = (int16_t) (mag_xyz[1]);
    dst->flm.elm.MAG_Y = (int16_t) (-mag_xyz[0]);
    dst->flm.elm.MAG_Z = (int16_t) (mag_xyz[2]);

    //ここまで80ms
    //HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_RESET);
#endif

#if 1
    //MCP3424 MCP3424_HV_ADDR, ad0=0,ad1=0 0x68
    //MCP3424 MCP3424_LVDT1_ADDR, ad0=1,ad1=0 0x6C
    //MCP3424 MCP3424_PT100_ADDR, d0=0,ad1=1 0x6A
    //MCP3424 MCP3424_LVDT2_ADDR, ad0=1,ad1=1 0x66
    //MCP3424 MCP3424_LVDT3_ADDR, ad0=0,ad1=open 0x61
    //変換時間に66msかかるので、ICごとに指令をずらして出す
    MCP3424_Ask(MCP3424_HV_ADDR, MOT_V_CH);
    MCP3424_Ask(MCP3424_PT100_ADDR, BAT_T_CH);
    MCP3424_Ask(MCP3424_LVDT1_ADDR, GND_P_CH);
    MCP3424_Ask(MCP3424_LVDT2_ADDR, BOA1_D_CH);
    MCP3424_Ask(MCP3424_LVDT3_ADDR, BOA2_D_CH);
    HAL_Delay(70);
    if (MCP3424_Ans(MCP3424_HV_ADDR, &data) == 0) dst->flm.elm.MOT_V = (int8_t) (data >> 8);
    if (MCP3424_Ans(MCP3424_PT100_ADDR, &data) == 0) dst->flm.elm.BAT_T = (int8_t) (data >> 8);
    if (MCP3424_Ans(MCP3424_LVDT1_ADDR, &data) == 0) dst->flm.elm.GND_P = data;
    if (MCP3424_Ans(MCP3424_LVDT2_ADDR, &data) == 0) dst->flm.elm.BOA1_D = data;
    if (MCP3424_Ans(MCP3424_LVDT3_ADDR, &data) == 0) dst->flm.elm.BOA2_D = data;
    //
    MCP3424_Ask(MCP3424_HV_ADDR, SYS_P2_CH);
    MCP3424_Ask(MCP3424_PT100_ADDR, LIQ2_T_CH);
    MCP3424_Ask(MCP3424_LVDT1_ADDR, BAT_V_CH);
    MCP3424_Ask(MCP3424_LVDT2_ADDR, LIQ3_T_CH);
    MCP3424_Ask(MCP3424_LVDT3_ADDR, LIQ4_T_CH);
    HAL_Delay(70);
    if (MCP3424_Ans(MCP3424_HV_ADDR, &data) == 0) {
        dst->flm.elm.MOT_I = (int8_t) (data >> 8);
        dst->flm.elm.SYS_P2 = data;
    }
    if (MCP3424_Ans(MCP3424_PT100_ADDR, &data) == 0) dst->flm.elm.LIQ2_T = data;
    if (MCP3424_Ans(MCP3424_LVDT1_ADDR, &data) == 0) dst->flm.elm.BAT_V = (int8_t) (data >> 8);
    if (MCP3424_Ans(MCP3424_LVDT2_ADDR, &data) == 0) dst->flm.elm.LIQ3_T = data;
    if (MCP3424_Ans(MCP3424_LVDT3_ADDR, &data) == 0) dst->flm.elm.LIQ4_T = data;
    //
    MCP3424_Ask(MCP3424_HV_ADDR, MOT_R_CH);
    MCP3424_Ask(MCP3424_PT100_ADDR, MOT_T_CH);
    MCP3424_Ask(MCP3424_LVDT1_ADDR, LIQ1_P_CH);
    HAL_Delay(70);
    if (MCP3424_Ans(MCP3424_HV_ADDR, &data) == 0) dst->flm.elm.MOT_R = (int8_t) (data >> 8);
    if (MCP3424_Ans(MCP3424_PT100_ADDR, &data) == 0) dst->flm.elm.MOT_T = (int8_t) (data >> 8);
    if (MCP3424_Ans(MCP3424_LVDT1_ADDR, &data) == 0) dst->flm.elm.LIQ1_P = data;
    //
    MCP3424_Ask(MCP3424_HV_ADDR, PDU_V_CH);
    MCP3424_Ask(MCP3424_PT100_ADDR, GEA_T_CH);
    MCP3424_Ask(MCP3424_LVDT1_ADDR, LIQ1_T_CH);
    HAL_Delay(70);
    if (MCP3424_Ans(MCP3424_HV_ADDR, &data) == 0) dst->flm.elm.PDU_V = (int8_t) (data >> 8);
    if (MCP3424_Ans(MCP3424_PT100_ADDR, &data) == 0) dst->flm.elm.GEA_T = (int8_t) (data >> 8);
    if (MCP3424_Ans(MCP3424_LVDT1_ADDR, &data) == 0) dst->flm.elm.LIQ1_T = data;
#endif
    /*
    data = cc;
    dst->flm.elm.MOT_R = (int8_t)(data/64);
    cc = cc + 64;
    if(128*64 < cc) cc=-127*64;
     */

#if 1
    // CPU Temp / SYS_P2
    float current_temp = GetInternalTemperature();
    dst->flm.elm.SYS_T2 = (int16_t)(current_temp*100);
#endif

    //チェックサム生成
    for (i = 1, sum = dst->flm.buf[0]; i <= N_FLAME - 2; i++) {
        sum += dst->flm.buf[i];
    };
    dst->flm.buf[N_FLAME - 1] = sum;

    //******計測終了
    //HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_RESET);
}

void Lib_dump_3f(int type, float x, float y, float z) {
    char msg[64];
    char fmt[32];
    switch (type) {
        case DTP_MAG:
            strcpy(fmt, "mag(xyz), %8.3f, %8.3f, %8.3f\r\n");
            break;
        case DTP_GRA:
            strcpy(fmt, "grv(xyz), %8.3f, %8.3f, %8.3f\r\n");
            break;
        case DTP_GAY:
            strcpy(fmt, "gyr(xyz), %8.3f, %8.3f, %8.3f\r\n");
            break;
        case DTP_ACC:
            strcpy(fmt, "acc(xyz), %8.3f, %8.3f, %8.3f\r\n");
            break;
        case DTP_HUM:
            strcpy(fmt, "tmp(TAH),  %8.3f, %8.3f, %8.3f\r\n");
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