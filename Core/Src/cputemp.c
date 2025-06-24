//
// Created by kikuchi on 2025/06/23.
//
#include "main.h" // HAL関連のヘッダファイル
#include "cputemp.h"

// 外部参照としてADCハンドルを宣言
extern ADC_HandleTypeDef hadc1;

// システムメモリからキャリブレーションデータを読み取るためのアドレス定義
// これらはSTM32F303T8のリファレンスマニュアルで確認が必要です
#define TEMP_CAL1_ADDR   ((uint16_t*)((uint32_t)0x1FFFF7B8)) // 例: 30°CでのADC値のアドレス
#define TEMP_CAL2_ADDR   ((uint16_t*)((uint32_t)0x1FFFF7C2)) // 例: 110°CでのADC値のアドレス
#define VREFINT_CAL_ADDR ((uint16_t*)((uint32_t)0x1FFFF7BA)) // 例: 内部基準電圧のキャリブレーション値

#define VREFINT_CAL_VREF_MV  3000 // 内部基準電圧がキャリブレーションされた時の電圧 (mV)

// ADCから温度を読み取る関数
float GetInternalTemperature(void)
{
    uint32_t adc_temp_raw;
    uint32_t adc_vref_raw;

    // 内部温度センサーチャネルを設定
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    // データシートで推奨されるサンプリング時間を設定
    sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5; // 例: F3シリーズの場合

    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        Error_Handler(); // エラー処理
    }

    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 100) != HAL_OK)
    {
        Error_Handler();
    }
    adc_temp_raw = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    // 内部基準電圧チャネルを設定 (正確な温度計算のため)
    sConfig.Channel = ADC_CHANNEL_VREFINT;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5; // VREFINTも同様に高いサンプリング時間

    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 100) != HAL_OK)
    {
        Error_Handler();
    }
    adc_vref_raw = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    // キャリブレーションデータを読み込む
    uint16_t ts_cal1 = *TEMP_CAL1_ADDR;
    uint16_t ts_cal2 = *TEMP_CAL2_ADDR;
    uint16_t vrefint_cal = *VREFINT_CAL_ADDR;

    // 現在のVCC (V_DDA) を内部基準電圧から推定
    // V_DDA = VREFINT_CAL_VREF_MV * (*VREFINT_CAL_ADDR) / adc_vref_raw;
    // 実際には、以下の計算式は、温度センサーの出力電圧がV_DDAに比例することを利用しています。
    // 温度センサーの出力電圧 = (adc_temp_raw / ADC_MAX_VALUE) * V_DDA
    // V_DDAはADCの基準電圧（通常V_DDAピンに印加される電圧）
    // V_DDAの変動を考慮するため、VREFINTチャネルを同時に測定し、補正に使用します。
    // データシートの計算式は通常、V_SENSEを直接扱う形です。
    // ここでは、ADC値の比率で補間します。

    float temperature_celsius;

    // 測定時のVrefint ADC値と工場出荷時のVrefint ADC値の比率を考慮して、
    // 測定時の温度センサーADC値を補正
    // これにより、VCCの変動による影響を相殺します。
    float current_ts_adc_compensated = (float)adc_temp_raw * (*VREFINT_CAL_ADDR) / adc_vref_raw;

    // 線形補間により温度を計算
    // T = (TS_ADC_DATA - TS_CAL1) * (110 - 30) / (TS_CAL2 - TS_CAL1) + 30
    temperature_celsius = (current_ts_adc_compensated - ts_cal1) * (110.0f - 30.0f) / (ts_cal2 - ts_cal1) + 30.0f;

    return temperature_celsius;
}
