//
// Created by kikuchi on 2025/06/23.
//
#include "main.h"             // HAL関連のヘッダファイル
#include "stm32f3xx_ll_adc.h" // 温度センサ/VREFINTの校正データ定義 (ST提供)
#include "cputemp.h"

// 外部参照としてADCハンドルを宣言
extern ADC_HandleTypeDef hadc1;

// 校正データのアドレス・校正温度・校正時Vref+は stm32f3xx_ll_adc.h の
// TEMPSENSOR_CAL1_ADDR / TEMPSENSOR_CAL2_ADDR / VREFINT_CAL_ADDR /
// TEMPSENSOR_CAL1_TEMP / TEMPSENSOR_CAL2_TEMP / VREFINT_CAL_VREF /
// TEMPSENSOR_CAL_VREFANALOG をそのまま使用する（自前定義しない）。

// 内部温度センサ / VREFINT のサンプリング時間。
// データシート上の最小サンプリング時間は約2.2us。
// ADC12クロックは PLLCLK(16MHz)/1 = 16MHz → 1サイクル 62.5ns なので
// 最低35.2サイクル必要。余裕を見て181.5サイクル(約11.3us)を使う。
#define CPUTEMP_SAMPLETIME  ADC_SAMPLETIME_181CYCLES_5

// ADC変換のポーリングタイムアウト (ms)
#define CPUTEMP_ADC_TIMEOUT 10U

// 測定失敗時に返す値（物理的にあり得ない温度をエラー標識として使う）
#define CPUTEMP_ERROR_VALUE (-273.15f)

// 指定チャネルを1回だけ変換して生値を得る
static HAL_StatusTypeDef ReadAdcChannel(uint32_t channel, uint32_t *raw)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    sConfig.Channel      = channel;
    sConfig.Rank         = ADC_REGULAR_RANK_1;
    sConfig.SingleDiff   = ADC_SINGLE_ENDED;
    sConfig.SamplingTime = CPUTEMP_SAMPLETIME;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset       = 0;

    // 内部チャネル(TEMPSENSOR/VREFINT)の測定経路の有効化と
    // 立ち上がり待ち(10us)はHAL側で実施される
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (HAL_ADC_Start(&hadc1) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (HAL_ADC_PollForConversion(&hadc1, CPUTEMP_ADC_TIMEOUT) != HAL_OK)
    {
        HAL_ADC_Stop(&hadc1);
        return HAL_ERROR;
    }

    *raw = HAL_ADC_GetValue(&hadc1);

    return HAL_ADC_Stop(&hadc1);
}

// ADCから温度を読み取る関数
// 戻り値: 温度 [degC]。測定失敗時は CPUTEMP_ERROR_VALUE (-273.15f)
float GetInternalTemperature(void)
{
    uint32_t adc_temp_raw = 0;
    uint32_t adc_vref_raw = 0;

    if (ReadAdcChannel(ADC_CHANNEL_TEMPSENSOR, &adc_temp_raw) != HAL_OK)
    {
        return CPUTEMP_ERROR_VALUE;
    }

    // 正確な温度計算のため、実際のVref+をVREFINTから求める
    if (ReadAdcChannel(ADC_CHANNEL_VREFINT, &adc_vref_raw) != HAL_OK)
    {
        return CPUTEMP_ERROR_VALUE;
    }

    // 工場出荷時の校正データ（システムメモリ上）
    const int32_t ts_cal1 = (int32_t) *TEMPSENSOR_CAL1_ADDR; //  30degC でのADC生値
    const int32_t ts_cal2 = (int32_t) *TEMPSENSOR_CAL2_ADDR; // 110degC でのADC生値

    // ゼロ除算・校正データ異常（未書き込みで0xFFFFなど）のガード
    if ((adc_vref_raw == 0U) || (ts_cal2 == ts_cal1))
    {
        return CPUTEMP_ERROR_VALUE;
    }

    // 実際のVref+ [mV] を算出（__LL_ADC_CALC_VREFANALOG_VOLTAGE と同じ式をfloatで）
    //   Vref+ = VREFINT_CAL * VREFINT_CAL_VREF / VREFINT_ADC_DATA
    const float vref_mv = (float) VREFINT_CAL_VREF * (float) (*VREFINT_CAL_ADDR)
                          / (float) adc_vref_raw;

    // 温度センサのADC生値を、校正時のVref+(3300mV)基準にスケーリング
    const float ts_scaled = (float) adc_temp_raw * vref_mv
                            / (float) TEMPSENSOR_CAL_VREFANALOG;

    // 線形補間により温度を計算（__LL_ADC_CALC_TEMPERATURE と同じ式をfloatで）
    //   T = (TS_ADC_DATA - TS_CAL1) * (CAL2_TEMP - CAL1_TEMP)
    //       / (TS_CAL2 - TS_CAL1) + CAL1_TEMP
    // ST提供マクロは整数演算で分解能が1degCとなるため、
    // 0.01degC分解能で使う呼び出し側に合わせてfloatで同じ計算を行う。
    const float temperature_celsius =
        (ts_scaled - (float) ts_cal1)
        * (float) (TEMPSENSOR_CAL2_TEMP - TEMPSENSOR_CAL1_TEMP)
        / (float) (ts_cal2 - ts_cal1)
        + (float) TEMPSENSOR_CAL1_TEMP;

    return temperature_celsius;
}
