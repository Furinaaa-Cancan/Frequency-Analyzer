#ifndef _DAC5311_H_
#define _DAC5311_H_

#include "gd32f10x.h"

/*!
 * \brief   DAC5311驱动（通过SPI接口）
 * \details 外部8位DAC芯片，通过SPI1控制
 *          SPI1接口：PB13(SCK), PB15(MOSI), PB12(CS)
 *          注意：PA7保留给ADC1_CH7使用
 */

/* 初始化DAC5311（SPI接口）*/
void DAC5311_Init(void);

/* 写入8位DAC数据（0-255）*/
void DAC5311_Write(uint8_t data);

#endif
