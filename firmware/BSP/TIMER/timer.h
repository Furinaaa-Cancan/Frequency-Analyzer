#ifndef _TIMER_H_
#define _TIMER_H_

#include "main.h"

void TIM1_Init(uint16_t psc,uint16_t per);

/* 初始化TIMER2为50kHz采样率（用于DDS波形生成） */
void TIMER2_DDS_Init(void);

/* 初始化TIMER3为ADC触发源（默认10kHz采样率） */
void TIMER3_ADC_Init(uint32_t sample_rate_hz);

/* 设置TIMER3采样率（动态调整） */
void TIMER3_SetSampleRate(uint32_t sample_rate_hz);

/* 获取TIMER2中断计数（调试用） */
uint32_t TIMER2_GetInterruptCount(void);

#endif
