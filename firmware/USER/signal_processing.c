/*!
 * \file    signal_processing.c
 * \brief   信号处理算法模块实现
 * \author  GD32 Bode Analyzer
 * \version v1.0
 */

#include "signal_processing.h"
#include <math.h>
#include <stddef.h>  /* 包含NULL定义 */

/* 数学常数 */
#ifndef PI
#define PI 3.14159265358979323846f
#endif

/*!
 * \brief   计算信号峰峰值
 * \param   data - 信号数据数组
 * \param   count - 数据点数量
 * \return  峰峰值（ADC原始值）
 */
uint16_t CalculatePeakToPeak(uint16_t *data, uint32_t count)
{
    /* 边界检查 */
    if(count == 0 || data == NULL) return 0;
    
    uint16_t min = 4095;
    uint16_t max = 0;
    
    for(uint32_t i = 0; i < count; i++)
    {
        if(data[i] < min) min = data[i];
        if(data[i] > max) max = data[i];
    }
    
    return (max - min);
}

/*!
 * \brief   计算信号的直流偏移（平均值）
 * \param   data - ADC数据数组
 * \param   count - 数据点数量
 * \return  平均值（ADC原始值）
 */
uint16_t CalculateDCOffset(uint16_t *data, uint32_t count)
{
    /* 边界检查 */
    if(count == 0 || data == NULL) return 0;
    
    uint32_t sum = 0;
    
    for(uint32_t i = 0; i < count; i++)
    {
        sum += data[i];
    }
    
    return (uint16_t)(sum / count);
}

/*!
 * \brief   使用RMS方法计算信号幅度（能量法）
 * \param   signal - 信号数据数组
 * \param   count - 数据点数量
 * \param   sample_rate - 采样率（Hz）（保留参数以保持接口兼容）
 * \param   signal_freq - 信号频率（Hz）（保留参数以保持接口兼容）
 * \return  信号幅度（峰值，单位与ADC值相同）
 */
float CalculateAmplitude_DFT(uint16_t *signal, uint32_t count, uint32_t sample_rate, uint32_t signal_freq)
{
    /* 边界检查 */
    if(count == 0 || signal == NULL) return 0.0f;
    
    /* 去除直流偏移 */
    uint16_t dc = CalculateDCOffset(signal, count);
    
    /* 计算信号的平方和（能量） */
    float sum_of_squares = 0.0f;
    
    for(uint32_t i = 0; i < count; i++)
    {
        /* 去直流后的信号值 */
        float val = (float)((int32_t)signal[i] - (int32_t)dc);
        
        /* 累加平方 */
        sum_of_squares += val * val;
    }
    
    /* 计算RMS（均方根）
     * 公式：RMS = sqrt(Σ(x² / N))
     */
    float rms = sqrtf(sum_of_squares / (float)count);
    
    /* 转换为峰值幅度
     * 对于正弦波：峰值 = RMS × √2
     */
    float amplitude = rms * 1.414213562f;  /* √2 ≈ 1.414213562 */
    
    return amplitude;
}

/*!
 * \brief   高精度相位差计算（DFT方法 + 浮点atan2）
 * \param   signal1 - 信号1数据（输入参考）
 * \param   signal2 - 信号2数据（输出测量）
 * \param   count - 数据点数量
 * \param   sample_rate - 采样率（Hz）
 * \param   signal_freq - 信号频率（Hz）
 * \return  相位差（度×100，例如1234表示12.34°）
 */
int32_t EstimatePhaseShift_Int(uint16_t *signal1, uint16_t *signal2, uint32_t count, 
                                uint32_t sample_rate, uint32_t signal_freq)
{
    /* 边界检查 */
    if(count == 0 || signal1 == NULL || signal2 == NULL || sample_rate == 0) return 0;
    
    /* 去除直流偏移 */
    uint16_t dc1 = CalculateDCOffset(signal1, count);
    uint16_t dc2 = CalculateDCOffset(signal2, count);
    
    /* DFT累加器（使用浮点，避免复杂的整数运算） */
    float sin_sum1 = 0.0f, cos_sum1 = 0.0f;  /* 信号1的sin/cos分量 */
    float sin_sum2 = 0.0f, cos_sum2 = 0.0f;  /* 信号2的sin/cos分量 */
    
    /* 预计算角频率（2 * PI * freq / sample_rate） */
    float omega = 2.0f * PI * signal_freq / (float)sample_rate;
    
    /* DFT计算（单频点） */
    for(uint32_t i = 0; i < count; i++)
    {
        /* 去直流后的信号值 */
        float val1 = (float)((int32_t)signal1[i] - (int32_t)dc1);
        float val2 = (float)((int32_t)signal2[i] - (int32_t)dc2);
        
        /* 当前相位角 */
        float phase = omega * i;
        
        /* 计算sin和cos值 */
        float sin_val = sinf(phase);
        float cos_val = cosf(phase);
        
        /* 累加DFT分量 */
        sin_sum1 += val1 * sin_val;
        cos_sum1 += val1 * cos_val;
        sin_sum2 += val2 * sin_val;
        cos_sum2 += val2 * cos_val;
    }
    
    /* 使用标准atan2f计算相位（弧度） */
    float phase1_rad = atan2f(sin_sum1, cos_sum1);  /* signal1 = PA6相位 */
    float phase2_rad = atan2f(sin_sum2, cos_sum2);  /* signal2 = PB1相位 */
    
    /* 计算相位差（弧度）：PA6 - PB1 */
    float phase_diff_rad = phase1_rad - phase2_rad;  /* θ = PA6 - PB1 */
    
    /* 转换为度×100 */
    float phase_diff_deg = phase_diff_rad * 18000.0f / PI;  /* rad * (180/π) * 100 */
    
    /* 归一化到 -180° ~ +180° (-18000 ~ +18000) */
    while(phase_diff_deg > 18000.0f) phase_diff_deg -= 36000.0f;
    while(phase_diff_deg < -18000.0f) phase_diff_deg += 36000.0f;
    
    return (int32_t)phase_diff_deg;
}

/*!
 * \brief   计算信号失真度（THD - Total Harmonic Distortion）
 * \param   data - ADC数据数组
 * \param   count - 数据点数量
 * \param   freq - 信号频率（Hz）
 * \param   sample_rate - 采样率（Hz）
 * \return  失真度百分比（0-100）
 * \details 通过DFT计算基波和谐波能量，评估波形失真程度
 */
float CalculateDistortion(uint16_t *data, uint32_t count, uint32_t freq, uint32_t sample_rate)
{
    /* 边界检查 */
    if(count == 0 || data == NULL || sample_rate == 0) return 100.0f;
    
    /* 去除直流偏移 */
    uint16_t dc = CalculateDCOffset(data, count);
    
    /* 计算基波（fundamental）能量 */
    float sin_sum = 0.0f, cos_sum = 0.0f;
    float omega = 2.0f * PI * freq / (float)sample_rate;
    
    for(uint32_t i = 0; i < count; i++)
    {
        float val = (float)((int32_t)data[i] - (int32_t)dc);
        float phase = omega * i;
        sin_sum += val * sinf(phase);
        cos_sum += val * cosf(phase);
    }
    
    /* 基波幅度 */
    float fundamental = sqrtf(sin_sum * sin_sum + cos_sum * cos_sum);
    
    /* 计算总能量（RMS） */
    float total_energy = 0.0f;
    for(uint32_t i = 0; i < count; i++)
    {
        float val = (float)((int32_t)data[i] - (int32_t)dc);
        total_energy += val * val;
    }
    total_energy = sqrtf(total_energy / count);
    
    /* 失真度 = sqrt(总能量^2 - 基波能量^2) / 基波能量 */
    if(fundamental < 1.0f) return 100.0f;
    
    float fundamental_rms = fundamental / sqrtf(2.0f * count);
    
    /* 防止负数开方（浮点舍入误差保护）*/
    float energy_diff = total_energy * total_energy - fundamental_rms * fundamental_rms;
    if(energy_diff < 0.0f) energy_diff = 0.0f;
    
    float harmonic_energy = sqrtf(energy_diff);
    
    float thd = (harmonic_energy / fundamental_rms) * 100.0f;
    
    /* 限制范围 */
    if(thd < 0.0f) thd = 0.0f;
    if(thd > 100.0f) thd = 100.0f;
    
    return thd;
}
