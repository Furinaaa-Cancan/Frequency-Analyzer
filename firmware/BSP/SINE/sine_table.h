/*!
 * \file     sine_table.h
 * \brief    正弦波查找表头文件
 * \details  256点正弦波表，范围0-255，用于DAC5311输出
 */

#ifndef _SINE_TABLE_H_
#define _SINE_TABLE_H_

#include <stdint.h>

#define SINE_TABLE_SIZE 256

/* 正弦波查找表：0-255范围 */
extern const uint8_t sine_table[SINE_TABLE_SIZE];

#endif /* _SINE_TABLE_H_ */


