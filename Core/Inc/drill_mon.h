#include <sys/select.h>
#include <sys/cdefs.h>
//
// Created by kikuchi on 2023/05/15.
//
#ifndef DRILL03_DRILL_MON_H
#define DRILL03_DRILL_MON_H

#include "main.h"
/**
 * 各種定数
 */
#define FILE_RENEW_SEC 60   //ファイル更新間隔(秒)
/**
 * リトルエンディアンなので、普通に格納するとローバイトが先頭にくる。
 * 送り出す前にビッグエンディアンに直す必要がある。
*/
#define N_FLAME 80     //転送バッファ長
union tlm_flame {
    uint8_t buf[N_FLAME];
    struct {
        uint32_t FS;
        uint32_t TI;
        uint8_t STAT;
        uint8_t PDU_V;
        uint8_t DMY1;
        uint8_t BAT_V;
        uint8_t BAT_T;
        uint8_t SYS_T;
        uint8_t SYS_H;
        uint8_t SYS_P;
        uint16_t GND_P;
        uint8_t MOT_V;
        uint8_t MOT_I;
        uint8_t MOT_T;
        uint8_t GEA_T;
        uint8_t MOT_R;
        uint16_t LIQ1_T;
        uint16_t LIQ1_P;
        uint16_t LIQ2_T;
        uint16_t BOA_D;
        uint16_t GRA_X;
        uint16_t GRA_Y;
        uint16_t GRA_Z;
        uint16_t ACC_X;
        uint16_t ACC_Y;
        uint16_t ACC_Z;
        uint16_t ROT_X;
        uint16_t ROT_Y;
        uint16_t ROT_Z;
        uint16_t MAG_X;
        uint16_t MAG_Y;
        uint16_t MAG_Z;
        uint16_t PAD;
        uint16_t SUM;
    } elm;
};

/**
 * Drillのステータス
 */
typedef struct {
    uint32_t TI;            //フレームカウンタ
    uint8_t isFirst;        //初回動作か
    uint8_t F_PPS;          //1PPSの通知
    union tlm_flame flm;   //転送用バッファ
    uint8_t fMount;         //FileMount OK=1, NG=0;
    uint8_t fOpen;         //File open OK=1, NG=0;
} DRILL_STATUS;

/**
 * mainで使う割込関数の実体
 */
void FS_Init(DRILL_STATUS *dst);

void PPS_Tick(DRILL_STATUS *dst);

//lib_mainにあるダンプ関数
enum {
    DTP_MAG,
    DTP_GAY,
    DTP_GRA,
    DTP_HUM,
};

void Lib_dump_3f(int type, float x, float y, float z);

void Lib_dump_ad(int8_t ch, int8_t rtc, uint16_t dt);

/**
 * mainで読み込むタスクの実体
 * @param argument
 */
_Noreturn void drill_loop(DRILL_STATUS *dst);

//各種デバイス定数,chは0base
#define MCP3424_HV_ADDR 0x68 //ad0=0,ad1=0
#define MOT_V_CH 0
#define MOT_I_CH 1
#define MOT_R_CH 2
#define PDU_V_CH 3
#define MCP3424_PT100_ADDR 0x6C //d0=1,ad1=0
#define BAT_T_CH 0
#define LIQ2_T_CH 1
#define MOT_T_CH 2
#define GEA_T_CH 3
#define MCP3424_LVDT_ADDR 0x6A  //d0=0,ad1=1
#define GND_P_CH 0
#define BAT_V_CH 1
#define LIQ1_P_CH 2
#define LIQ1_T_CH 3

#endif //DRILL03_DRILL_MON_H
