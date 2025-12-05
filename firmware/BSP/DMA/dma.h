#ifndef _DMA_H_
#define _DMA_H_


#include "gd32f10x.h"
#include "main.h"

/* USART DMA缓冲区 */
extern uint8_t buf_recv[5];
extern uint8_t buf_send[5];

/* ADC DMA缓冲区 - 双ADC同步模式 */
#define ADC_BUFFER_SIZE  512  /* 每通道512个采样点（提高低频精度）*/
extern uint32_t adc_buffer[ADC_BUFFER_SIZE];  /* 32位：高16位=ADC1, 低16位=ADC0 */

/* USART DMA初始化 */
void USART0_DMA_Init(void);

/* ADC DMA初始化（双ADC同步模式） */
void ADC_DMA_Init(void);

/* 重启DMA采集（用于欠采样波形采集） */
void ADC_DMA_Restart(uint32_t sample_count);

#endif
