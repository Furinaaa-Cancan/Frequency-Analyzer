/*!
 * \file    adc_handler.c
 * \brief   ADC数据处理模块实现
 * \author  GD32 Bode Analyzer
 * \version v1.0
 */

#include "adc_handler.h"
#include "signal_processing.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* 外部DMA缓冲区声明 */
extern uint32_t adc_buffer[ADC_BUFFER_SIZE];

/* 外部DDS函数 */
extern uint32_t DDS_GetFrequency(void);

/*!
 * \brief   从DMA缓冲区提取双通道ADC数据
 * \param   adc0_data - 输出：ADC0数据数组（输入参考）
 * \param   adc1_data - 输出：ADC1数据数组（输出信号）
 * \param   count - 要提取的样本数量
 */
void ExtractADCData(uint16_t *adc0_data, uint16_t *adc1_data, uint32_t count)
{
    for(uint32_t i = 0; i < count && i < ADC_BUFFER_SIZE; i++)
    {
        /* ⚠️ 实际硬件数据格式（实测验证）：
         * 在GD32F103双ADC并行模式下，DMA数据格式为：
         * ADC_RDATA[15:0]  = ADC0的转换结果（PA6，输入参考信号）
         * ADC_RDATA[31:16] = ADC1的转换结果（PB1，输出测量信号）
         * 
         * 注意：PA6是第一级运放输出（标准正弦波，有直流偏置）
         *       PB1经过C3/C4耦合电容，直流偏置被滤除，负半周被ADC削波到0
         * 
         * ⚠️ 硬件问题：PB1信号没有直流偏置，ADC读到的是半波整流信号！
         *    理想情况应在PB1添加1.65V偏置电路
         */
        adc0_data[i] = (uint16_t)(adc_buffer[i] & 0xFFFF);           /* ADC0(PA6): 低16位 */
        adc1_data[i] = (uint16_t)((adc_buffer[i] >> 16) & 0xFFFF);   /* ADC1(PB1): 高16位（有削波）*/
    }
}

/*!
 * \brief   处理ADC数据并计算幅频/相频特性
 * \details 测量真实电路的频率响应（外部反馈）
 */
void ProcessADCData(void)
{
    static uint16_t adc0_data[512];  /* ADC0数据缓存（PA6: 输入参考）*/
    static uint16_t adc1_data[512];  /* ADC1数据缓存（PB1: 输出测量）*/
    
    /* 获取当前频率 */
    uint32_t current_freq = DDS_GetFrequency();
    
    /* ⭐ 计算自适应采样率（严格10倍频率） */
    uint32_t adaptive_sample_rate = current_freq * 10;
    
    /* 1. 提取ADC数据（双通道反馈）*/
    ExtractADCData(adc0_data, adc1_data, 512);
    
    /* 2. 使用RMS能量法计算信号幅度（使用自适应采样率） */
    float amp_ch1 = CalculateAmplitude_DFT(adc0_data, 512, adaptive_sample_rate, current_freq);
    float amp_ch2 = CalculateAmplitude_DFT(adc1_data, 512, adaptive_sample_rate, current_freq);
    
    /* 转换为整数以便后续处理（ADC单位）*/
    uint16_t pp_ch1 = (uint16_t)amp_ch1;
    uint16_t pp_ch2 = (uint16_t)amp_ch2;
    
    /* 3. 数据有效性检查 */
    if(pp_ch1 < 10 || pp_ch2 < 10)
    {
        printf("[ERROR] Signal too weak! CH1=%d, CH2=%d ADC (expected: >100)\r\n", pp_ch1, pp_ch2);
        printf("Possible causes:\r\n");
        printf("  1. DAC5311输出未连接或未工作\r\n");
        printf("  2. 运放电路未正常工作\r\n");
        printf("  3. 被测电路(DUT)未连接\r\n");
        printf("  4. ADC输入引脚未连接 (PA6/PB1)\r\n");
        printf("Run 'DEBUG' command for detailed diagnosis.\r\n\r\n");
        return;
    }
    
    /* 额外检查：信号幅度是否合理 */
    if(pp_ch1 > 4000 || pp_ch2 > 4000)
    {
        printf("[WARNING] Signal clipping! CH1=%d, CH2=%d ADC\r\n", pp_ch1, pp_ch2);
    }
    
    /* 4. 转换为电压（mV，峰值幅度，RMS能量法） */
    uint32_t voltage_ch1_mv = ((uint32_t)pp_ch1 * 3300) / 4096;
    uint32_t voltage_ch2_mv = ((uint32_t)pp_ch2 * 3300) / 4096;
    
    /* 5. 计算幅频特性 H(ω) = K₁/K (CH1=输入K, CH2=输出K₁) */
    uint32_t H_x10000 = 0;
    if(pp_ch1 > 0)  /* 防止除零 */
    {
        H_x10000 = ((uint32_t)pp_ch2 * 10000) / pp_ch1;
    }
    else
    {
        printf("[ERROR] CH1 amplitude is zero! Cannot calculate gain.\r\n");
        return;
    }
    
    /* 6. 计算相位差（使用DFT方法，使用自适应采样率） */
    int32_t phase_x100 = EstimatePhaseShift_Int(adc0_data, adc1_data, 512, adaptive_sample_rate, current_freq);
    
    /* 7. 计算直流偏移 */
    uint16_t dc_ch1 = CalculateDCOffset(adc0_data, 512);
    uint16_t dc_ch2 = CalculateDCOffset(adc1_data, 512);
    
    /* 8. 输出结果 */
    printf("========================================\r\n");
    printf("Mode: External Feedback Monitoring\r\n");
    printf("Frequency: %d Hz\r\n", DDS_GetFrequency());
    printf("CH1 (PA6): %d.%02d mV (RMS×√2), ADC=%d, DC=%d\r\n", 
           voltage_ch1_mv/100, voltage_ch1_mv%100, pp_ch1, dc_ch1);
    printf("CH2 (PB1): %d.%02d mV (RMS×√2), ADC=%d, DC=%d\r\n", 
           voltage_ch2_mv/100, voltage_ch2_mv%100, pp_ch2, dc_ch2);
    printf("Gain H(w): %d.%04d x (%.2f dB)\r\n", 
           H_x10000/10000, H_x10000%10000,
           20.0 * log10((float)H_x10000 / 10000.0));
    printf("Phase theta(w): %s%d.%02d deg\r\n", 
           (phase_x100<0)?"-":"", abs(phase_x100/100), abs(phase_x100%100));
    printf("========================================\r\n");
    
    /* 9. 输出Web界面格式 (FREQ_RESP) */
    printf("FREQ_RESP:%d,%d.%02d,%d.%02d,%d.%04d,%s%d.%02d\r\n", 
           DDS_GetFrequency(),
           voltage_ch1_mv/100, voltage_ch1_mv%100,        /* K (CH1输入，mV) */
           voltage_ch2_mv/100, voltage_ch2_mv%100,        /* K1 (CH2输出，mV) */
           H_x10000/10000, H_x10000%10000,                /* H (实测) */
           (phase_x100<0)?"-":"", abs(phase_x100/100), abs(phase_x100%100)  /* theta (实测) */
    );
    
    /* 10. 发送波形数据用于实时显示 */
    uint32_t freq = DDS_GetFrequency();
    uint32_t skip;  /* 降采样步长 */
    
    /* 计算降采样步长：20kHz采样率，确保满足奈奎斯特定理 + 波形清晰 */
    if(freq <= 100) {
        skip = 4;   /* 低频：20000/100=200点/周期，降到50点/周期 */
    } else if(freq <= 300) {
        skip = 2;   /* 中低频：20000/300=66点/周期，降到33点/周期 */
    } else {
        skip = 1;   /* 中高频及以上：全部发送，确保波形精确 */
    }
    
    /* 调试：检查PA6数据质量 */
    uint16_t zero_count_pa6 = 0;
    uint16_t zero_count_pb1 = 0;
    uint16_t repeat_count_pa6 = 0;
    uint16_t min_pa6 = 4095, max_pa6 = 0;
    uint16_t min_pb1 = 4095, max_pb1 = 0;
    
    for(uint32_t i = 0; i < 512; i++) {
        if(adc0_data[i] == 0) zero_count_pa6++;
        if(adc1_data[i] == 0) zero_count_pb1++;
        
        if(adc0_data[i] < min_pa6) min_pa6 = adc0_data[i];
        if(adc0_data[i] > max_pa6) max_pa6 = adc0_data[i];
        if(adc1_data[i] < min_pb1) min_pb1 = adc1_data[i];
        if(adc1_data[i] > max_pb1) max_pb1 = adc1_data[i];
        
        if(i > 0 && adc0_data[i] == adc0_data[i-1]) {
            repeat_count_pa6++;
        }
    }
    
    /* 如果发现异常，输出详细诊断信息 */
    if(zero_count_pa6 > 50 || repeat_count_pa6 > 300) {
        printf("[WARNING] PA6 Data Quality Issues:\r\n");
        printf("  - Zeros: %d/512\r\n", zero_count_pa6);
        printf("  - Repeats: %d/512\r\n", repeat_count_pa6);
        printf("  - Range: %d - %d (pp=%d)\r\n", min_pa6, max_pa6, max_pa6 - min_pa6);
        printf("  - PB1 Range: %d - %d (pp=%d)\r\n", min_pb1, max_pb1, max_pb1 - min_pb1);
        printf("  - First 10 samples PA6: ");
        for(uint32_t i = 0; i < 10; i++) {
            printf("%d ", adc0_data[i]);
        }
        printf("\r\n");
        printf("  - First 10 samples PB1: ");
        for(uint32_t i = 0; i < 10; i++) {
            printf("%d ", adc1_data[i]);
        }
        printf("\r\n");
    }
    
    printf("WAVEFORM:%d,%d,", freq, adaptive_sample_rate);
    
    /* 发送输入信号波形（PA6）*/
    for(uint32_t i = 0; i < 512; i += skip)
    {
        printf("%d", adc0_data[i]);
        if(i + skip < 512) printf(",");
    }
    
    printf("|");  /* 分隔符 */
    
    /* 发送输出信号波形（PB1）*/
    for(uint32_t i = 0; i < 512; i += skip)
    {
        printf("%d", adc1_data[i]);
        if(i + skip < 512) printf(",");
    }
    
    printf("\r\n");
    printf("\r\n");
}
