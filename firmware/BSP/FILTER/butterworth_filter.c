/*!
 * \file     butterworth_filter.c
 * \brief    巴特沃斯低通滤波器实现
 * \details  4阶Butterworth低通滤波器，级联2个二阶节
 * \version  1.0.0
 * \date     2025-10-12
 */

#include "butterworth_filter.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ==================== 内部函数声明 ==================== */
static void calculate_biquad_coefficients(biquad_section_t *section, 
                                          float omega_c, 
                                          float q_factor);

/* ==================== 滤波器初始化 ==================== */
void butterworth_init(butterworth_filter_t *filter, uint32_t sample_rate, uint32_t cutoff_freq)
{
    filter->sample_rate = sample_rate;
    filter->cutoff_freq = cutoff_freq;
    filter->enabled = 1;
    
    /* 计算归一化截止频率 */
    float omega_c = 2.0f * M_PI * cutoff_freq / sample_rate;
    
    /* 4阶Butterworth的Q值（2个二阶节）*/
    float q_factors[FILTER_SECTIONS] = {0.541f, 1.306f};  /* Butterworth标准值 */
    
    /* 为每个二阶节计算系数 */
    for(int i = 0; i < FILTER_SECTIONS; i++)
    {
        calculate_biquad_coefficients(&filter->sections[i], omega_c, q_factors[i]);
        filter->sections[i].w1 = 0.0f;
        filter->sections[i].w2 = 0.0f;
    }
}

/* ==================== 计算二阶节系数（双线性变换法）==================== */
static void calculate_biquad_coefficients(biquad_section_t *section, 
                                          float omega_c, 
                                          float q_factor)
{
    /* 预变换频率 */
    float omega = tan(omega_c / 2.0f);
    float omega2 = omega * omega;
    
    /* 归一化因子 */
    float norm = 1.0f / (1.0f + omega / q_factor + omega2);
    
    /* 计算系数 */
    section->b0 = omega2 * norm;
    section->b1 = 2.0f * section->b0;
    section->b2 = section->b0;
    
    section->a1 = 2.0f * (omega2 - 1.0f) * norm;
    section->a2 = (1.0f - omega / q_factor + omega2) * norm;
}

/* ==================== 处理单个样本 ==================== */
uint8_t butterworth_process(butterworth_filter_t *filter, uint8_t input)
{
    if(!filter->enabled)
    {
        return input;  /* 旁路模式 */
    }
    
    /* 转换为浮点数（0-255 → 0.0-1.0） */
    float sample = (float)input / 255.0f;
    
    /* 级联处理所有二阶节 */
    for(int i = 0; i < FILTER_SECTIONS; i++)
    {
        biquad_section_t *s = &filter->sections[i];
        
        /* 直接型II转置结构 */
        float output = s->b0 * sample + s->w1;
        s->w1 = s->b1 * sample - s->a1 * output + s->w2;
        s->w2 = s->b2 * sample - s->a2 * output;
        
        sample = output;  /* 输出作为下一节的输入 */
    }
    
    /* 转换回整数（0.0-1.0 → 0-255），带限幅 */
    int32_t result = (int32_t)(sample * 255.0f + 0.5f);
    if(result < 0) result = 0;
    if(result > 255) result = 255;
    
    return (uint8_t)result;
}

/* ==================== 复位滤波器 ==================== */
void butterworth_reset(butterworth_filter_t *filter)
{
    for(int i = 0; i < FILTER_SECTIONS; i++)
    {
        filter->sections[i].w1 = 0.0f;
        filter->sections[i].w2 = 0.0f;
    }
}

/* ==================== 自适应截止频率 ==================== */
void butterworth_adaptive_cutoff(butterworth_filter_t *filter, uint32_t signal_freq)
{
    /* 设置截止频率为信号频率的2.5倍
     * 这样可以：
     * 1. 保留信号基频和少量谐波（改善波形）
     * 2. 滤除高频噪声和量化误差
     * 3. 避免过度平滑导致的相位延迟
     */
    uint32_t new_cutoff = signal_freq * 25 / 10;  /* 2.5倍 */
    
    /* 限制截止频率不超过采样率的1/5（奈奎斯特定理的安全边界） */
    uint32_t max_cutoff = filter->sample_rate / 5;
    if(new_cutoff > max_cutoff)
    {
        new_cutoff = max_cutoff;
    }
    
    /* 只在频率变化较大时重新计算（节省CPU） */
    if(new_cutoff != filter->cutoff_freq)
    {
        butterworth_init(filter, filter->sample_rate, new_cutoff);
    }
}

/* ==================== 使能控制 ==================== */
void butterworth_enable(butterworth_filter_t *filter, uint8_t enable)
{
    filter->enabled = enable;
    
    /* 切换模式时复位状态 */
    if(enable)
    {
        butterworth_reset(filter);
    }
}


