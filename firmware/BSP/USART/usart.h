#ifndef _USART_H_
#define _USART_H_

#include "main.h"

void USART0_Init(uint32_t baudrate);
void UART_SendStreamData(uint8_t sample, uint16_t adc0, uint16_t adc1);

#endif
