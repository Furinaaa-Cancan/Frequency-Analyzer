/*!
 * \file    adc_handler.h
 * \brief   ADC数据处理模块
 * \author  GD32 Bode Analyzer
 * \version v1.0
 */

#ifndef __ADC_HANDLER_H
#define __ADC_HANDLER_H

#include "gd32f10x.h"
#include "../BSP/DMA/dma.h"  /* 使用dma.h中的ADC_BUFFER_SIZE定义 */

/* 函数声明 */

/*!
 * \brief   从DMA缓冲区提取双通道ADC数据
 * \param   adc0_data - 输出：ADC0数据数组（输入参考）
 * \param   adc1_data - 输出：ADC1数据数组（输出信号）
 * \param   count - 要提取的样本数量
 */
void ExtractADCData(uint16_t *adc0_data, uint16_t *adc1_data, uint32_t count);

/*!
 * \brief   处理ADC数据并计算幅频/相频特性
 * \details 测量真实电路的频率响应（外部反馈）
 */
void ProcessADCData(void);

/*!
 * \brief   欠采样波形采集（独立功能）
 * \param   signal_freq - 信号频率(Hz)
 * \param   sample_rate - 采样率(Hz)，可以低于信号频率（欠采样）
 * \details 用于演示欠采样效果和波形可视化
 */
void CaptureWaveform(uint32_t signal_freq, uint32_t sample_rate);

#endif /* __ADC_HANDLER_H */
