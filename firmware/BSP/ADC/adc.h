#ifndef _ADC_H_
#define _ADC_H_

#include "main.h"

/* 双ADC同步模式初始化 */
void ADC_Dual_Init(void);

/* 单通道ADC读取（保留旧接口，兼容性） */
double ADC_Read(uint32_t adc_periph);

/* 温度读取（保留旧接口） */
double GetTemp(double data);

#endif
