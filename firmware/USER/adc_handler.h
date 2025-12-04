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

#endif /* __ADC_HANDLER_H */
