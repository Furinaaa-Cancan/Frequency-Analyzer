/*!
 * \file    measurement.h
 * \brief   测量功能模块 - 扫频和校准
 * \author  GD32 Bode Analyzer
 * \version v1.0
 */

#ifndef __MEASUREMENT_H
#define __MEASUREMENT_H

#include "gd32f10x.h"

/* 校准系统配置 */
#define CALIBRATION_POINTS  100  /* 校准点数量：10Hz-1000Hz，步进10Hz */

/* 校准数据结构 */
typedef struct {
    uint8_t valid;                              /* 校准数据有效标志 */
    uint16_t gain_correction[CALIBRATION_POINTS]; /* 增益校正系数×10000 */
    int16_t phase_correction[CALIBRATION_POINTS]; /* 相位校正（度×100）*/
} CalibrationData_t;

/* 外部校准数据声明 */
extern CalibrationData_t g_calibration;

/* 函数声明 */

/*!
 * \brief   自动扫频测量（10Hz ~ 1kHz）
 * \details 每隔10Hz测量一次，输出完整的频率响应曲线
 */
void AutoSweep(void);

/*!
 * \brief   自动校准系统（测量直连环路响应）
 * \details 测量100个频率点的传输特性，保存校准系数
 */
void AutoCalibration(void);

#endif /* __MEASUREMENT_H */
