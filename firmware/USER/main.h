#ifndef _MAIN_H_
#define _MAIN_H_

#include "gd32f10x.h"
#include <stdio.h>

/* 信号类型定义 */
typedef enum {
    SIGNAL_TYPE_SINE = 0,
    SIGNAL_TYPE_ECG = 1
} SignalType_t;

/* 全局信号类型变量 */
extern SignalType_t g_signal_type;

void delay_ms(uint32_t ms);
void delay_us(uint16_t us);

#endif
