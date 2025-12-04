#ifndef _TIMER_LED_H_
#define _TIMER_LED_H_

#include "main.h"

// 定时器初始化（用于LED PWM控制）
void TIM1_Init_LED(uint16_t psc,uint16_t per);

// 定时发送功能（任务三第3点）
void Timer_Set_Send_Interval(uint16_t interval_ms);  // 设置发送间隔
void Timer_Enable_Auto_Send(uint8_t enable);         // 启用/禁用自动发送
uint16_t Timer_Get_Send_Interval(void);              // 获取发送间隔

#endif
