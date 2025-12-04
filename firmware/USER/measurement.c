/*!
 * \file    measurement.c
 * \brief   测量功能模块实现 - 扫频和校准
 * \author  GD32 Bode Analyzer
 * \version v1.0
 */

#include "measurement.h"
#include "signal_processing.h"
#include "adc_handler.h"
#include <stdio.h>
#include <stdlib.h>

/* 全局校准数据定义 */
CalibrationData_t g_calibration = {0};

/* 外部DDS函数声明 */
extern void DDS_SetFrequency(uint32_t freq);
extern uint32_t DDS_GetFrequency(void);

/* 外部TIMER函数声明 */
extern void TIMER3_SetSampleRate(uint32_t sample_rate_hz);

/* 外部延时函数声明 */
extern void delay_ms(uint32_t ms);

/* 外部系统滴答计数（用于测量时间）*/
extern volatile uint32_t systick_ms;

/*!
 * \brief   自动扫频测量（10Hz ~ 1kHz）
 * \details 每隔10Hz测量一次，输出完整的频率响应曲线
 */
void AutoSweep(void)
{
    printf("\r\n");
    printf("================================================\r\n");
    printf("  AUTO FREQUENCY SWEEP: 10Hz - 1000Hz\r\n");
    printf("  Step: 10Hz, Total: 100 points\r\n");
    printf("  Mode: External Feedback with Adaptive Sampling\r\n");
    printf("  Amplitude Method: RMS Energy (RMS能量法)\r\n");
    printf("  Phase Algorithm: Float DFT + atan2f\r\n");
    printf("  Phase Unwrapping: Enabled\r\n");
    printf("  ⭐ NEW: Adaptive Sampling Rate\r\n");
    printf("    采样率 = 信号频率 × 10 (满足老师要求)\r\n");
    printf("    10Hz  → 100Hz采样\r\n");
    printf("    100Hz → 1kHz采样\r\n");
    printf("    1kHz  → 10kHz采样\r\n");
    printf("================================================\r\n");
    printf("OK:SWEEP_START\r\n");
    printf("================================================\r\n\r\n");
    
    /* 相位unwrapping变量 */
    int32_t phase_offset = 0;
    int32_t last_phase_raw = 0;
    uint8_t is_first_point = 1;
    
    /* 失真统计 */
    uint16_t distortion_count = 0;
    uint16_t total_points = 0;
    
    /* 测量时间统计 */
    uint32_t sweep_start_time = systick_ms;
    uint32_t total_measurement_time = 0;
    
    for(uint32_t freq = 10; freq <= 1000; freq += 10)
    {
        /* 记录本频率点测量开始时间 */
        uint32_t freq_start_time = systick_ms;
        
        /* 设置频率 */
        DDS_SetFrequency(freq);
        
        /* ⭐ 自适应采样率：采样率 = 信号频率 × 10 */
        uint32_t adaptive_sample_rate = freq * 10;
        TIMER3_SetSampleRate(adaptive_sample_rate);
        
        printf("[INFO] %dHz: 采样率设置为 %dHz (10倍频率)\r\n", freq, adaptive_sample_rate);
        
        /* 调试输出 */
        if(freq >= 750) {
            printf("[DEBUG] Starting measurement at %d Hz\r\n", freq);
        }
        
        /* 自适应稳定时间 */
        uint32_t settle_time_ms;
        if(freq <= 20) {
            settle_time_ms = (15000 / freq) + 200;
        } else if(freq <= 50) {
            settle_time_ms = (10000 / freq) + 100;
        } else if(freq <= 200) {
            settle_time_ms = (5000 / freq) + 50;
        } else {
            settle_time_ms = (3000 / freq) + 50;
        }
        if(settle_time_ms < 100) settle_time_ms = 100;
        
        delay_ms(settle_time_ms);
        
        /* 自适应多次测量平均 */
        uint8_t measurement_count;
        if(freq <= 20) {
            measurement_count = 3;
        } else if(freq <= 50) {
            measurement_count = 2;
        } else {
            measurement_count = 1;
        }
        
        uint32_t sum_pp_ch1 = 0;
        uint32_t sum_pp_ch2 = 0;
        int64_t sum_phase = 0;
        
        /* ADC数据缓冲区 */
        static uint16_t adc0_data[512];
        static uint16_t adc1_data[512];
        
        for(uint8_t m = 0; m < measurement_count; m++)
        {
            /* 采集双通道反馈数据 */
            ExtractADCData(adc0_data, adc1_data, 512);
            
            /* ⭐ 使用自适应采样率进行计算（重要！） */
            float amp_ch1_single = CalculateAmplitude_DFT(adc0_data, 512, adaptive_sample_rate, freq);
            float amp_ch2_single = CalculateAmplitude_DFT(adc1_data, 512, adaptive_sample_rate, freq);
            
            uint16_t pp_ch1_single = (uint16_t)amp_ch1_single;
            uint16_t pp_ch2_single = (uint16_t)amp_ch2_single;
            
            /* 调试输出 */
            if(freq >= 750 && m == 0) {
                printf("[DEBUG] %dHz ADC: CH1=%d, CH2=%d, sample[0]=%d,%d\r\n", 
                       freq, pp_ch1_single, pp_ch2_single, adc0_data[0], adc1_data[0]);
            }
            
            /* ⭐ 计算相位差（使用自适应采样率） */
            int32_t phase_single = EstimatePhaseShift_Int(adc0_data, adc1_data, 512, adaptive_sample_rate, freq);
            
            /* 计算失真度（使用自适应采样率） */
            if(m == 0) {
                float distortion_input = CalculateDistortion(adc0_data, 512, freq, adaptive_sample_rate);
                float distortion_output = CalculateDistortion(adc1_data, 512, freq, adaptive_sample_rate);
                
                total_points++;
                if(distortion_output > 15.0f) {
                    distortion_count++;
                }
                
                if(distortion_output > 15.0f) {
                    printf("[WARN] %dHz: 输出信号失真严重! THD=%.1f%% (输入THD=%.1f%%)\r\n", 
                           freq, distortion_output, distortion_input);
                    printf("       建议：降低测试频率上限或改进运放电路\r\n");
                }
            }
            
            sum_pp_ch1 += pp_ch1_single;
            sum_pp_ch2 += pp_ch2_single;
            sum_phase += phase_single;
            
            /* 每次测量后发送波形数据（包含真实采样率） */
            {
                uint32_t skip = 1;
                
                printf("WAVEFORM:%d,%d,", freq, adaptive_sample_rate);
                
                for(uint32_t i = 0; i < 512; i += skip)
                {
                    printf("%d", adc0_data[i]);
                    if(i + skip < 512) printf(",");
                }
                
                printf("|");
                
                for(uint32_t i = 0; i < 512; i += skip)
                {
                    printf("%d", adc1_data[i]);
                    if(i + skip < 512) printf(",");
                }
                
                printf("\r\n");
            }
            
            /* 多次测量之间等待 */
            if(m < measurement_count - 1) {
                uint32_t wait_time = (1000 / freq) + 10;
                if(wait_time < 20) wait_time = 20;
                if(wait_time > 100) wait_time = 100;
                delay_ms(wait_time);
            }
        }
        
        /* 计算平均值 */
        uint16_t pp_ch1 = sum_pp_ch1 / measurement_count;
        uint16_t pp_ch2 = sum_pp_ch2 / measurement_count;
        int32_t phase_raw = (int32_t)(sum_phase / measurement_count);
        
        /* 检查信号有效性 */
        if(pp_ch1 < 5 || pp_ch2 < 5)
        {
            printf("[WARN] Weak signal at %dHz: CH1=%d, CH2=%d\r\n", freq, pp_ch1, pp_ch2);
        }
        
        /* 转换为电压 */
        float voltage_ch1 = ((float)pp_ch1 * 3.3f) / 4096.0f;
        float voltage_ch2 = ((float)pp_ch2 * 3.3f) / 4096.0f;
        
        /* 计算幅频特性 */
        float H = 0.0f;
        if(voltage_ch1 > 0.001f)
        {
            H = voltage_ch2 / voltage_ch1;
        }
        
        /* 应用校准修正 */
        float H_corrected = H;
        int32_t phase_corrected = phase_raw;
        
        if(g_calibration.valid && freq >= 10 && freq <= 1000 && (freq % 10) == 0)
        {
            uint32_t freq_idx = (freq / 10) - 1;
            
            float correction_factor = (float)g_calibration.gain_correction[freq_idx] / 10000.0f;
            H_corrected = H * correction_factor;
            
            phase_corrected = phase_raw + g_calibration.phase_correction[freq_idx];
        }
        
        /* 相位Unwrapping */
        if(!is_first_point)
        {
            int32_t phase_diff = phase_corrected - last_phase_raw;
            
            if(phase_diff > 18000)
            {
                phase_offset -= 36000;
            }
            else if(phase_diff < -18000)
            {
                phase_offset += 36000;
            }
        }
        
        int32_t phase_unwrapped = phase_corrected + phase_offset;
        
        /* 更新历史记录 */
        last_phase_raw = phase_corrected;
        is_first_point = 0;
        
        /* 输出频率响应数据 */
        if(g_calibration.valid)
        {
            printf("FREQ_RESP:%d,%.4f,%.4f,%.6f,%s%.2f,%.6f,%s%.2f\r\n", 
                   freq,
                   voltage_ch1, voltage_ch2,
                   H, (phase_raw<0)?"-":"", (float)abs(phase_raw)/100.0f,
                   H_corrected, (phase_unwrapped<0)?"-":"", (float)abs(phase_unwrapped)/100.0f);
        }
        else
        {
            printf("FREQ_RESP:%d,%.4f,%.4f,%.6f,%s%.2f\r\n", 
                   freq,
                   voltage_ch1, voltage_ch2,
                   H, (phase_unwrapped<0)?"-":"", (float)abs(phase_unwrapped)/100.0f);
        }
        
        /* 计算本频率点测量耗时 */
        uint32_t freq_elapsed_time = systick_ms - freq_start_time;
        total_measurement_time += freq_elapsed_time;
        
        /* 进度显示（包含测量时间） */
        if(freq % 50 == 0)
        {
            printf("# Progress: %d/1000 Hz (CH1=%d, CH2=%d, Time=%dms)\r\n", 
                   freq, pp_ch1, pp_ch2, freq_elapsed_time);
        }
        
        if(freq >= 800) {
            printf("[DEBUG] Completed %d Hz measurement (耗时%dms)\r\n", freq, freq_elapsed_time);
        }
    }
    
    /* 计算总耗时 */
    uint32_t total_elapsed = systick_ms - sweep_start_time;
    
    printf("\r\n");
    printf("[DEBUG] Loop完成！准备输出结束信息...\r\n");
    printf("================================================\r\n");
    printf("OK:SWEEP_COMPLETE\r\n");
    printf("  Total Points: 100\r\n");
    printf("  Frequency Range: 10-1000 Hz\r\n");
    printf("  Algorithm: Adaptive DFT Phase Detection\r\n");
    printf("  ⏱️  Total Measurement Time: %.2f seconds\r\n", total_elapsed / 1000.0f);
    printf("  ⏱️  Average Time per Point: %d ms\r\n", total_measurement_time / 100);
    printf("  \r\n");
    printf("  📊 信号质量统计:\r\n");
    printf("    高失真点 (THD>15%%): %d / %d (%.1f%%)\r\n", 
           distortion_count, total_points, (float)distortion_count * 100.0f / total_points);
    if(distortion_count > 30) {
        printf("    ⚠️  失真点过多，建议优化硬件电路或降低测试频率\r\n");
    } else if(distortion_count > 0) {
        printf("    ✅ 大部分频率点数据可靠\r\n");
    } else {
        printf("    ✅ 所有频率点信号质量良好\r\n");
    }
    printf("================================================\r\n\r\n");
    
    /* 恢复到默认频率 */
    DDS_SetFrequency(100);
}

/*!
 * \brief   自动校准系统（直通测试）
 * \details 要求：将PA6直接短接到PB1
 */
void AutoCalibration(void)
{
    printf("\r\n");
    printf("================================================\r\n");
    printf("  CALIBRATION MODE - Through Connection Test\r\n");
    printf("  将PA6直接短接到PB1！\r\n");
    printf("  要求：移除被测电路（DUT），用短线连接PA6->PB1\r\n");
    printf("  理论结果：H(ω)≈1.0, θ(ω)≈0°\r\n");
    printf("  测试范围：10Hz - 1000Hz (100 points)\r\n");
    printf("================================================\r\n");
    printf("按任意键开始校准，或输入CANCEL取消...\r\n");
    
    delay_ms(3000);
    
    printf("\r\n[INFO] 开始校准测量...\r\n");
    printf("OK:CALIBRATION_START\r\n");
    
    /* 清空校准数据 */
    g_calibration.valid = 0;
    for(uint8_t i = 0; i < CALIBRATION_POINTS; i++)
    {
        g_calibration.gain_correction[i] = 10000;
        g_calibration.phase_correction[i] = 0;
    }
    
    /* 扫频校准 */
    for(uint32_t freq_idx = 0; freq_idx < CALIBRATION_POINTS; freq_idx++)
    {
        uint32_t freq = (freq_idx + 1) * 10;
        
        DDS_SetFrequency(freq);
        
        /* ⭐ 自适应采样率（校准时也使用10倍频率） */
        uint32_t adaptive_sample_rate = freq * 10;
        TIMER3_SetSampleRate(adaptive_sample_rate);
        
        /* 等待信号稳定 */
        uint32_t settle_time_ms = (freq <= 50) ? (10000 / freq + 100) : (5000 / freq + 50);
        if(settle_time_ms < 100) settle_time_ms = 100;
        delay_ms(settle_time_ms);
        
        /* 采集数据 */
        static uint16_t adc0_data[512];
        static uint16_t adc1_data[512];
        ExtractADCData(adc0_data, adc1_data, 512);
        
        /* 计算幅度和相位（使用自适应采样率） */
        uint16_t pp_ch1 = CalculatePeakToPeak(adc0_data, 512);
        uint16_t pp_ch2 = CalculatePeakToPeak(adc1_data, 512);
        int32_t phase_raw = EstimatePhaseShift_Int(adc0_data, adc1_data, 512, adaptive_sample_rate, freq);
        
        /* 检查有效性 */
        if(pp_ch1 < 10 || pp_ch2 < 10)
        {
            printf("[ERROR] CALIB_FAIL:%d,signal_weak\r\n", freq);
            printf("[提示] 请检查PA6和PB1是否正确连接！\r\n");
            return;
        }
        
        /* 计算校准系数 */
        uint32_t H_measured = ((uint32_t)pp_ch2 * 10000) / pp_ch1;
        
        uint32_t correction = 100000000UL / H_measured;
        if(correction > 65535) correction = 65535;
        g_calibration.gain_correction[freq_idx] = (uint16_t)correction;
        
        g_calibration.phase_correction[freq_idx] = (int16_t)(-phase_raw);
        
        /* 输出校准数据 */
        printf("CALIB_DATA:%d,%d.%04d,%s%d.%02d\r\n", 
               freq,
               H_measured/10000, H_measured%10000,
               (phase_raw<0)?"-":"", abs(phase_raw/100), abs(phase_raw%100));
        
        if(freq % 100 == 0)
        {
            printf("# Calibration Progress: %d/1000 Hz\r\n", freq);
        }
    }
    
    /* 标记校准数据有效 */
    g_calibration.valid = 1;
    
    printf("\r\n");
    printf("================================================\r\n");
    printf("OK:CALIBRATION_COMPLETE\r\n");
    printf("  校准完成！共%d个频率点\r\n", CALIBRATION_POINTS);
    printf("  校准数据已保存到内存\r\n");
    printf("  后续测量将自动应用校准修正\r\n");
    printf("  \r\n");
    printf("  下一步：\r\n");
    printf("    1. 移除PA6-PB1短接线\r\n");
    printf("    2. 连接被测电路（DUT）\r\n");
    printf("    3. 运行SWEEP命令\r\n");
    printf("================================================\r\n\r\n");
    
    DDS_SetFrequency(100);
}
