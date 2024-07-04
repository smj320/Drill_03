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

/**
 * リトルエンディアンなので下位バイトが先にくる。
*/
#define N_FLAME 80     //転送バッファ長
union tlm_flame {
    uint8_t buf[N_FLAME];
    struct {
        uint32_t FS;    //0
        uint32_t TI;    //4
        uint8_t STAT;   //8
        int8_t PDU_V;   //9
        int8_t DMY1;    //10
        int8_t BAT_V;   //11
        int8_t BAT_T;   //12
        int8_t SYS_T;   //13
        int8_t SYS_H;   //14
        int8_t SYS_P;   //15
        int16_t GND_P;  //16
        int8_t MOT_V;   //18
        int8_t MOT_I;   //19
        int8_t MOT_T;   //20
        int8_t GEA_T;   //21
        int8_t MOT_R;   //22
        int8_t PAD1;    //23
        int16_t LIQ1_T; //24
        int16_t LIQ1_P; //26
        int16_t LIQ2_T; //28
        int16_t BOA1_D;  //30
        int16_t GRA_X;  //32
        int16_t GRA_Y;  //34
        int16_t GRA_Z;  //36
        int16_t ACC_X;  //38
        int16_t ACC_Y;  //40
        int16_t ACC_Z;  //42
        int16_t ROT_X;  //44
        int16_t ROT_Y;  //46
        int16_t ROT_Z;  //48
        int16_t MAG_X;  //50
        int16_t MAG_Y;  //52
        int16_t MAG_Z;  //54
        int16_t BOA2_D; //56
        int16_t LIQ3_T; //58
        int16_t LIQ4_T; //60
        uint16_t PAD;   //62
        uint16_t SUM;   //80
    } elm;
};

/**
 * Drillのステータス
 */
//ファイルステータスビット
#define ST_SD_INIT (1<<0)
#define ST_SD_OPEN (1<<1)
#define ST_SD_WRITE (1<<2)
#define ST_SD_CLOSE (1<<3)
typedef struct {
    uint32_t TI;            //フレームカウンタ
    uint8_t isFirst;        //初回動作か
    uint8_t F_PPS;          //1PPSの通知
    union tlm_flame flm;   //転送用バッファ
    uint8_t f_stat;         //ファイルステータスビット
} DRILL_STATUS;

/**
 * mainで使う割込関数の実体
 */
void PPS_Tick(DRILL_STATUS *dst);

//lib_mainにあるダンプ関数
enum {
    DTP_MAG,
    DTP_GAY,
    DTP_GRA,
    DTP_ACC,
    DTP_HUM,
};

void Lib_dump_3f(int type, float x, float y, float z);
void Lib_dump_ad(int8_t rtc[], uint16_t data[]);

/**
 * mainで読み込むタスクの実体
 * @param argument
 */
_Noreturn void drill_loop(DRILL_STATUS *dst);

//各種デバイス定数,chは0base
#define MCP3424_HV_ADDR 0x68 //ad0=0,ad1=0
#define MOT_I_CH 0
#define MOT_V_CH 1
#define MOT_R_CH 2
#define PDU_V_CH 3
#define MCP3424_LVDT1_ADDR 0x6A  //d0=0,ad1=1
#define GND_P_CH 0
#define BAT_V_CH 1
#define LIQ1_P_CH 2
#define LIQ1_T_CH 3
#define MCP3424_PT100_ADDR 0x6C //d0=1,ad1=0
#define BAT_T_CH 0
#define LIQ2_T_CH 1
#define MOT_T_CH 2
#define GEA_T_CH 3
#define MCP3424_LVDT2_ADDR 0x66  //d0=1,ad1=1
#define BOA1_D_CH 0
#define LIQ3_T_CH 3
#define MCP3424_LVDT3_ADDR 0x61  //d0=0,ad1=F
#define BOA1_2_CH 0
#define LIQ4_T_CH 1

#endif //DRILL03_DRILL_MON_H
