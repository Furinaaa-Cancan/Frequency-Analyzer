/*!
 * \file     dds.c
 * \brief    DDS频率控制模块实现
 * \details  使用相位累加器实现频率可调的正弦波生成
 */

#include "dds.h"
#include "../SINE/sine_table.h"

/* DDS状态变量 */
static uint32_t dds_phase_accumulator = 0;  /* 相位累加器（32位） */
static uint32_t dds_phase_increment = 0;    /* 相位增量 */
static uint32_t dds_current_freq = 100;     /* 当前频率（Hz） */
static uint8_t dds_output_enable = 0;       /* 输出使能标志 */

/*!
 * \brief   DDS初始化
 */
void DDS_Init(void)
{
    dds_phase_accumulator = 0;
    dds_output_enable = 0;
    DDS_SetFrequency(100);  /* 默认100Hz */
}

/*!
 * \brief   设置输出频率
 * \param   freq_hz 频率（Hz），范围：10-2000Hz
 * \details 计算公式：phase_increment = (freq * 2^32) / sample_rate
 */
void DDS_SetFrequency(uint32_t freq_hz)
{
    /* 限制频率范围 */
    if(freq_hz < DDS_MIN_FREQ) freq_hz = DDS_MIN_FREQ;
    if(freq_hz > DDS_MAX_FREQ) freq_hz = DDS_MAX_FREQ;
    
    /* 计算相位增量
     * phase_increment = (freq * 2^32) / sample_rate
     * 为避免溢出，改写为：phase_increment = (freq * (2^32 / sample_rate))
     * 2^32 / 50000 = 85899.34592 ≈ 85899 (已验证工作正常)
     */
    dds_phase_increment = freq_hz * 85899UL;
    dds_current_freq = freq_hz;
}

/*!
 * \brief   获取当前频率
 * \return  当前频率（Hz）
 */
uint32_t DDS_GetFrequency(void)
{
    return dds_current_freq;
}

/*!
 * \brief   获取下一个波形样本
 * \return  正弦波样本值（0-255）
 * \details 在50kHz定时器中断中调用此函数
 */
uint8_t DDS_GetSample(void)
{
    uint8_t sample = 0;
    
    if(dds_output_enable)
    {
        /* 从相位累加器高8位获取查找表索引 */
        uint8_t index = (dds_phase_accumulator >> 24) & 0xFF;
        
        /* 从正弦波表获取样本（直接输出，不滤波）*/
        sample = sine_table[index];
        
        /* 相位累加 */
        dds_phase_accumulator += dds_phase_increment;
    }
    else
    {
        /* 输出禁用时，输出中点值 */
        sample = 128;
    }
    
    return sample;
}

/*!
 * \brief   启动输出
 */
void DDS_Start(void)
{
    dds_phase_accumulator = 0;  /* 复位相位 */
    dds_output_enable = 1;
}

/*!
 * \brief   停止输出
 */
void DDS_Stop(void)
{
    dds_output_enable = 0;
}

/*!
 * \brief   获取当前正弦表索引
 * \return  正弦表索引（0-255）
 */
uint8_t DDS_GetSineIndex(void)
{
    return (dds_phase_accumulator >> 24) & 0xFF;
}

/*!
 * \brief   获取当前相位累加器值
 * \return  相位累加器值（32位）
 */
uint32_t DDS_GetPhaseAccumulator(void)
{
    return dds_phase_accumulator;
}

/*!
 * \brief   获取DDS输出使能状态（调试用）
 * \return  1=使能，0=禁用
 */
uint8_t DDS_IsEnabled(void)
{
    return dds_output_enable;
}

/*!
 * \brief   获取相位增量（调试用）
 * \return  相位增量值
 */
uint32_t DDS_GetPhaseIncrement(void)
{
    return dds_phase_increment;
}

