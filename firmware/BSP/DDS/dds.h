/*!
 * \file     dds.h
 * \brief    DDS频率控制模块头文件
 * \details  直接数字频率合成，支持10Hz-2000Hz频率输出
 */

#ifndef _DDS_H_
#define _DDS_H_

#include <stdint.h>

/* 配置参数 */
#define DDS_SAMPLE_RATE  50000UL    /* 采样率：50kHz（对2000Hz仍有25倍采样） */
#define DDS_MIN_FREQ     10         /* 最小频率：10Hz */
#define DDS_MAX_FREQ     2000       /* 最大频率：2000Hz */
#define DDS_FREQ_STEP    10         /* 频率步进：10Hz */

/* 滤波器配置 */
#define DDS_FILTER_ENABLED  1       /* 使能巴特沃斯滤波器（提升信号纯度和THD） */

/* DDS初始化 */
void DDS_Init(void);

/* 设置输出频率 */
void DDS_SetFrequency(uint32_t freq_hz);

/* 获取当前频率 */
uint32_t DDS_GetFrequency(void);

/* 获取下一个波形样本（在定时器中断中调用） */
uint8_t DDS_GetSample(void);

/* 获取当前正弦表索引 */
uint8_t DDS_GetSineIndex(void);

/* 获取当前相位累加器值 */
uint32_t DDS_GetPhaseAccumulator(void);

/* 启动/停止输出 */
void DDS_Start(void);
void DDS_Stop(void);

#endif /* _DDS_H_ */

