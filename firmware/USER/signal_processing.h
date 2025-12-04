/*!
 * \file    signal_processing.h
 * \brief   信号处理算法模块 - DFT、相位、失真度计算
 * \author  GD32 Bode Analyzer
 * \version v1.0
 */

#ifndef __SIGNAL_PROCESSING_H
#define __SIGNAL_PROCESSING_H

#include "gd32f10x.h"

/* 函数声明 */

/*!
 * \brief   计算信号峰峰值
 * \param   data - 信号数据数组
 * \param   count - 数据点数量
 * \return  峰峰值（ADC原始值）
 */
uint16_t CalculatePeakToPeak(uint16_t *data, uint32_t count);

/*!
 * \brief   计算信号直流偏移
 * \param   data - 信号数据数组
 * \param   count - 数据点数量
 * \return  平均值（ADC原始值）
 */
uint16_t CalculateDCOffset(uint16_t *data, uint32_t count);

/*!
 * \brief   使用RMS方法计算信号幅度（能量法）
 * \param   signal - 信号数据数组
 * \param   count - 数据点数量
 * \param   sample_rate - 采样率（Hz）（保留参数以保持接口兼容）
 * \param   signal_freq - 信号频率（Hz）（保留参数以保持接口兼容）
 * \return  信号幅度（峰值，单位与ADC值相同）
 */
float CalculateAmplitude_DFT(uint16_t *signal, uint32_t count, uint32_t sample_rate, uint32_t signal_freq);

/*!
 * \brief   估算两路信号的相位差（整数版本，单位：度×100）
 * \param   signal1 - 参考信号数据
 * \param   signal2 - 待测信号数据
 * \param   count - 数据点数量
 * \param   sample_rate - 采样率（Hz）
 * \param   signal_freq - 信号频率（Hz）
 * \return  相位差（度×100，例如：90度 = 9000）
 */
int32_t EstimatePhaseShift_Int(uint16_t *signal1, uint16_t *signal2, uint32_t count, 
                                uint32_t sample_rate, uint32_t signal_freq);

/*!
 * \brief   计算信号失真度（THD - Total Harmonic Distortion）
 * \param   data - 信号数据数组
 * \param   count - 数据点数量
 * \param   freq - 信号频率（Hz）
 * \param   sample_rate - 采样率（Hz）
 * \return  失真度百分比（0-100）
 */
float CalculateDistortion(uint16_t *data, uint32_t count, uint32_t freq, uint32_t sample_rate);

#endif /* __SIGNAL_PROCESSING_H */
