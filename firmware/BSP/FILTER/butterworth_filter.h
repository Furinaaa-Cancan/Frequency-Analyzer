/*!
 * \file     butterworth_filter.h
 * \brief    巴特沃斯低通滤波器模块
 * \details  用于DAC输出信号的平滑处理，提升信号纯度
 * \version  1.0.0
 * \date     2025-10-12
 */

#ifndef BUTTERWORTH_FILTER_H
#define BUTTERWORTH_FILTER_H

#include <stdint.h>

/* ==================== 滤波器配置 ==================== */
#define BUTTERWORTH_ORDER           4       /* 4阶滤波器 */
#define FILTER_SECTIONS             2       /* 二阶节数量（4阶=2个二阶节） */
#define ADAPTIVE_FILTER_ENABLED     1       /* 自适应截止频率 */

/* ==================== 滤波器结构体 ==================== */
typedef struct {
    /* 二阶节系数（直接型II转置） */
    float b0, b1, b2;  /* 分子系数 */
    float a1, a2;      /* 分母系数 */
    
    /* 状态变量 */
    float w1, w2;      /* 延迟状态 */
} biquad_section_t;

typedef struct {
    biquad_section_t sections[FILTER_SECTIONS];  /* 级联的二阶节 */
    uint32_t sample_rate;                         /* 采样率 */
    uint32_t cutoff_freq;                         /* 截止频率 */
    uint8_t enabled;                              /* 使能标志 */
} butterworth_filter_t;

/* ==================== 滤波器API ==================== */

/*!
 * \brief   初始化巴特沃斯滤波器
 * \param   filter 滤波器结构体指针
 * \param   sample_rate 采样率（Hz）
 * \param   cutoff_freq 截止频率（Hz）
 */
void butterworth_init(butterworth_filter_t *filter, uint32_t sample_rate, uint32_t cutoff_freq);

/*!
 * \brief   处理单个样本
 * \param   filter 滤波器结构体指针
 * \param   input 输入样本（0-255）
 * \return  滤波后的样本（0-255）
 */
uint8_t butterworth_process(butterworth_filter_t *filter, uint8_t input);

/*!
 * \brief   复位滤波器状态
 * \param   filter 滤波器结构体指针
 */
void butterworth_reset(butterworth_filter_t *filter);

/*!
 * \brief   动态更新截止频率（自适应滤波）
 * \param   filter 滤波器结构体指针
 * \param   signal_freq 信号频率（Hz）
 * \details 自动设置截止频率为信号频率的2-3倍，既滤除高频噪声又保留波形
 */
void butterworth_adaptive_cutoff(butterworth_filter_t *filter, uint32_t signal_freq);

/*!
 * \brief   使能/禁用滤波器
 * \param   filter 滤波器结构体指针
 * \param   enable 1=使能，0=禁用（直通模式）
 */
void butterworth_enable(butterworth_filter_t *filter, uint8_t enable);

#endif /* BUTTERWORTH_FILTER_H */


