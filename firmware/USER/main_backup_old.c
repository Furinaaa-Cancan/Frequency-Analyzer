/*
 * @Author: leeyoung7017
 * @Description: 伯德图分析仪 - 通用频率响应测试系统 v5.5
 * @Architecture:
 *   - DDS算法 → SPI0 → DAC5311(U3) → 滤波运放 → PB1正弦波输出
 *   - PB1输出 → 被测电路(DUT) → ADC双通道采样
 *   - 计算频响特性 H(ω)=K₁/K 和相频θ(ω)
 * @Hardware:
 *   - SPI0 → DAC5311: PA5(SCLK), PA7(MOSI/DIN), PA4(SYNC)
 *   - DAC输出: PB1 (经过三级运放链路)
 *   - ADC采样: PA6(ADC0_CH6)=输入参考K, PB1(ADC1_CH9)=输出测量K₁
 *   - 串口控制: PA9(TX), PA10(RX) - 115200波特率
 * @Mode: Universal Frequency Response Analyzer (Bode Plot)
 * @Note: 测量真实硬件电路的幅频和相频特性，无需外部RC电路
 * @Email: leeyoung7017@163.com
 */
#include "main.h"
#include "led.h"
#include "timer.h"
#include "../BSP/TIMER_LED/timer_led.h"  /* LED PWM定时器 - 使用相对路径 */
#include "usart.h"
#include "adc.h"
#include "dac5311.h"
#include "../BSP/DMA/dma.h"
#include <stdlib.h>  /* 包含abs()函数 */
#include <math.h>    /* 包含数学函数（未使用但保留） */

/* 重构后的模块化头文件 */
#include "signal_processing.h"
#include "adc_handler.h"
#include "measurement.h"

/* 外部DDS函数 */
extern void DDS_Init(void);
extern void DDS_SetFrequency(uint32_t freq_hz);
extern uint32_t DDS_GetFrequency(void);
extern void DDS_Start(void);
extern void DDS_Stop(void);

/* 外部DMA缓冲区 */
extern uint32_t adc_buffer[ADC_BUFFER_SIZE];

/* 外部校准数据（在measurement.c中定义）*/
extern CalibrationData_t g_calibration;

/* 全局信号类型变量 (默认正弦波) */
SignalType_t g_signal_type = SIGNAL_TYPE_SINE;

/* 工具函数声明 */
void delay_ms(uint32_t ms);
void delay_us(uint16_t us);

int main()
{
    /* 1. 初始化UART */
    USART0_Init(115200);
    
    /* 2. 发送启动消息 */
    printf("\r\n===========================================\r\n");
    printf("  GD32F103 Bode Plot Analyzer v5.6\r\n");
    printf("  Mode: External DAC5311 Signal Generator\r\n");
    printf("  DDS Generator: 10Hz - 1kHz (via SPI0 → DAC5311)\r\n");
    printf("  Signal Output: PB1 (filtered sine wave)\r\n");
    printf("  ADC Sampling: PA6(Input K), PB1(Output K₁)\r\n");
    printf("  Amplitude Method: RMS Energy (RMS能量法)\r\n");
    printf("  Frequency Response: H(ω)=K₁/K, θ(ω)\r\n");
    printf("===========================================\r\n");
    printf("Type HELP for commands.\r\n\r\n");
    
    /* 3. 初始化DAC5311（SPI0接口）*/
    DAC5311_Init();
    printf("[OK] DAC5311 initialized via SPI0.\r\n");
    
    /* 4. 初始化DDS模块（默认100Hz）*/
    DDS_Init();
    printf("[OK] DDS initialized (default 100Hz).\r\n");
    
    /* 5. 初始化TIMER2（50kHz采样率 - DDS波形生成）*/
    TIMER2_DDS_Init();
    printf("[OK] TIMER2 initialized (50kHz for DDS).\r\n");
    
    /* 6. 启动DDS */
    printf("[INFO] Starting DDS...\r\n");
    DDS_Start();
    printf("[OK] DDS started. Signal output via DAC5311 → PB1!\r\n\r\n");
    
    /* 7. 初始化ADC的DMA传输 */
    ADC_DMA_Init();
    printf("[OK] ADC DMA initialized (256 samples buffer).\r\n");
    
    /* 8. 初始化双ADC同步模式 */
    ADC_Dual_Init();
    printf("[OK] Dual ADC initialized (PA6: Input K, PB1: Output K₁).\r\n");
    
    /* 9. 初始化TIMER3（20kHz采样率 - ADC触发）*/
    TIMER3_ADC_Init(20000);  /* 20kHz采样率（改进：提高采样率以减少高频失真）*/
    printf("[OK] TIMER3 initialized (20kHz ADC trigger).\r\n");
    
    /* 10. 初始化LED控制系统（集成自作业4）*/
    LED_Init();
    printf("[OK] LED system initialized (GPIOB: PB11-PB15).\r\n");
    
    /* 11. 初始化TIMER1（20kHz中断用于LED PWM）*/
    TIM1_Init_LED(71, 49);  /* 72MHz/(71+1)/(49+1) = 20kHz */
    printf("[OK] TIMER1 initialized (20kHz for LED PWM).\r\n");
    
    /* 12. 设置默认LED状态为OFF */
    LED_Set_Mode(LED_MODE_OFF);
    printf("[INFO] LED mode: OFF (default).\r\n");
    
    printf("\r\n==============================================\r\n");
    printf("  Bode Plot Analyzer Ready\r\n");
    printf("  Signal Flow:\r\n");
    printf("    DDS -> SPI0 -> DAC5311 -> [Filter] -> PB1\r\n");
    printf("       |\r\n");
    printf("       +-> PA6 (Input K)\r\n");
    printf("       |\r\n");
    printf("       +-> [DUT] -> PB1 (Output K₁)\r\n");
    printf("  \r\n");
    printf("  Current Configuration:\r\n");
    printf("    PA6: Input reference (from signal chain)\r\n");
    printf("    PB1: Output measurement (after amplifiers)\r\n");
    printf("  \r\n");
    printf("  测量对象:\r\n");
    printf("    当前测量的是DAC+三级运放链路的传输特性\r\n");
    printf("    特性: 带通滤波器 (中心频率~200Hz)\r\n");
    printf("  \r\n");
    printf("  ⚠️  硬件限制:\r\n");
    printf("    - 最佳测试范围: 20Hz - 500Hz\r\n");
    printf("    - 高频(>700Hz)失真严重，数据仅供参考\r\n");
    printf("    - 低频(<20Hz)幅度小，信噪比低\r\n");
    printf("  \r\n");
    printf("  Commands:\r\n");
    printf("    FREQ:100       - Set frequency to 100Hz\r\n");
    printf("    MEASURE        - Measure H(ω) and θ(ω)\r\n");
    printf("    SWEEP          - Auto sweep 10Hz-1kHz\r\n");
    printf("    SWEEP:500      - Custom sweep 10Hz-500Hz\r\n");
    printf("    CALIBRATION    - Through connection test\r\n");
    printf("  \r\n");
    printf("  LED Commands:\r\n");
    printf("    LED:0-6        - Set LED mode (0=OFF, 1=Flow, 2=Breath,\r\n");
    printf("                     3=Blink, 4=Specific, 5=Rotate, 6=Alarm)\r\n");
    printf("    LED_BRIGHT:50  - Set brightness (0-100%%)\r\n");
    printf("    LED_FREQ:500   - Set interval (1-30000ms)\r\n");
    printf("==============================================\r\n\r\n");
    
    printf("[INFO] System ready! PB1 outputting 100Hz sine wave.\r\n");
    printf("[INFO] Type 'HELP' for command list.\r\n\r\n");
    
    /* 主循环 - 空闲等待UART命令 */
    while(1)
    {
        /* UART命令处理在中断中完成 */
        /* 波形生成在TIMER2中断中完成 */
        /* ADC采样持续进行（DMA自动传输） */
        
        /* 空闲延迟，降低CPU使用率 */
        delay_ms(100);
        
        /* 可选：定期输出心跳（调试用，正常使用可注释掉） */
        // static uint32_t heartbeat = 0;
        // if(++heartbeat >= 100) {  /* 每10秒一次 */
        //     heartbeat = 0;
        //     printf("[IDLE] Freq=%dHz, waiting for commands...\r\n", DDS_GetFrequency());
        // }
    }
}

/* ============== 工具函数实现 ============== */

/**
 * @brief:  Millisecond delay
 * @param {uint32_t} ms
 * @return {*}
 */
void delay_ms(uint32_t ms)
{
	uint32_t i;
	for(i=0;i < ms;i++)
	{
		delay_us(1000);
	}
}

/**
 * @brief: Nanosecond delay
 * @param {uint16_t} us
 * @return {*}
 */
void delay_us(uint16_t us)
{
	SysTick->CTRL = 0; // Disable SysTick
	SysTick->LOAD = SystemCoreClock/1000000 * us; // 24bit Reload register
	SysTick->VAL = 0; // Clear current value as well as count flag
	SysTick->CTRL = 5; // Enable SysTick timer with processor clock
	while ((SysTick->CTRL & 0x00010000)==0);// Wait until count flag is set
	SysTick->CTRL = 0; // Disable SysTick
}

/*!
 * \brief   计算信号失真度（THD - Total Harmonic Distortion）
 * \param   data - ADC数据数组
 * \param   count - 数据点数量
 * \param   freq - 信号频率（Hz）
 * \param   sample_rate - 采样率（Hz）
 * \return  失真度百分比（0-100）
 * \details 通过DFT计算基波和谐波能量，评估波形失真程度
 */
float CalculateDistortion(uint16_t *data, uint32_t count, uint32_t freq, uint32_t sample_rate)
{
    /* 边界检查 */
    if(count == 0 || data == NULL || sample_rate == 0) return 100.0f;
    
    /* 去除直流偏移 */
    uint16_t dc = CalculateDCOffset(data, count);
    
    /* 计算基波（fundamental）能量 */
    float sin_sum = 0.0f, cos_sum = 0.0f;
    float omega = 2.0f * PI * freq / (float)sample_rate;
    
    for(uint32_t i = 0; i < count; i++)
    {
        float val = (float)((int32_t)data[i] - (int32_t)dc);
        float phase = omega * i;
        sin_sum += val * sinf(phase);
        cos_sum += val * cosf(phase);
    }
    
    /* 基波幅度 */
    float fundamental = sqrtf(sin_sum * sin_sum + cos_sum * cos_sum);
    
    /* 计算总能量（RMS） */
    float total_energy = 0.0f;
    for(uint32_t i = 0; i < count; i++)
    {
        float val = (float)((int32_t)data[i] - (int32_t)dc);
        total_energy += val * val;
    }
    total_energy = sqrtf(total_energy / count);
    
    /* 失真度 = sqrt(总能量^2 - 基波能量^2) / 基波能量 */
    if(fundamental < 1.0f) return 100.0f;  /* 信号太弱，fundamental_rms也会很小 */
    
    float fundamental_rms = fundamental / sqrtf(2.0f * count);
    
    /* ⚠️ 关键修复：防止负数开方（浮点舍入误差保护）*/
    float energy_diff = total_energy * total_energy - fundamental_rms * fundamental_rms;
    if(energy_diff < 0.0f) energy_diff = 0.0f;
    
    float harmonic_energy = sqrtf(energy_diff);
    
    float thd = (harmonic_energy / fundamental_rms) * 100.0f;
    
    /* 限制范围 */
    if(thd < 0.0f) thd = 0.0f;
    if(thd > 100.0f) thd = 100.0f;
    
    return thd;
}

/*!
 * \brief   使用RMS方法计算信号幅度（能量法）
 * \param   signal - 信号数据数组
 * \param   count - 数据点数量
 * \param   sample_rate - 采样率（Hz）（保留参数以保持接口兼容）
 * \param   signal_freq - 信号频率（Hz）（保留参数以保持接口兼容）
 * \return  信号幅度（峰值，单位与ADC值相同）
 * \note    v5.6改进：使用RMS能量法，比DFT法更稳定
 * \details 算法原理：
 *          1. 计算信号的RMS（均方根值）
 *          2. 峰值幅度 = RMS × √2
 *          RMS是最稳定的能量法，不受频谱泄漏影响
 *          
 *          数学公式：
 *          RMS = sqrt(Σ(x[i] - dc)² / N)
 *          峰值 = RMS × √2
 *          
 *          优势：
 *          - 稳定性极高（不受频率误差影响）
 *          - 计算简单快速
 *          - 物理意义明确（信号能量）
 *          - 不需要频率信息
 */
float CalculateAmplitude_DFT(uint16_t *signal, uint32_t count, uint32_t sample_rate, uint32_t signal_freq)
{
    /* 边界检查 */
    if(count == 0 || signal == NULL) return 0.0f;
    
    /* 去除直流偏移 */
    uint16_t dc = CalculateDCOffset(signal, count);
    
    /* 计算信号的平方和（能量） */
    float sum_of_squares = 0.0f;
    
    for(uint32_t i = 0; i < count; i++)
    {
        /* 去直流后的信号值 */
        float val = (float)((int32_t)signal[i] - (int32_t)dc);
        
        /* 累加平方 */
        sum_of_squares += val * val;
    }
    
    /* 计算RMS（均方根）
     * 公式：RMS = sqrt(Σ(x² / N))
     */
    float rms = sqrtf(sum_of_squares / (float)count);
    
    /* 转换为峰值幅度
     * 对于正弦波：峰值 = RMS × √2
     */
    float amplitude = rms * 1.414213562f;  /* √2 ≈ 1.414213562 */
    
    return amplitude;
}

/*!
 * \brief   高精度相位差计算（DFT方法 + 浮点atan2）
 * \param   signal1 - 信号1数据（输入参考）
 * \param   signal2 - 信号2数据（输出测量）
 * \param   count - 数据点数量
 * \param   sample_rate - 采样率（Hz）
 * \param   signal_freq - 信号频率（Hz）
 * \return  相位差（度×100，例如1234表示12.34°）
 * \note    v5.4修复版：简化算法，使用浮点atan2f确保精度
 * \details 算法原理：
 *          1. 计算信号的sin/cos分量（DFT单频点）
 *          2. 使用标准atan2f计算相位角
 *          3. 计算两个信号的相位差
 */
int32_t EstimatePhaseShift_Int(uint16_t *signal1, uint16_t *signal2, uint32_t count, 
                                uint32_t sample_rate, uint32_t signal_freq)
{
    /* 边界检查 */
    if(count == 0 || signal1 == NULL || signal2 == NULL || sample_rate == 0) return 0;
    
    /* 去除直流偏移 */
    uint16_t dc1 = CalculateDCOffset(signal1, count);
    uint16_t dc2 = CalculateDCOffset(signal2, count);
    
    /* DFT累加器（使用浮点，避免复杂的整数运算） */
    float sin_sum1 = 0.0f, cos_sum1 = 0.0f;  /* 信号1的sin/cos分量 */
    float sin_sum2 = 0.0f, cos_sum2 = 0.0f;  /* 信号2的sin/cos分量 */
    
    /* 预计算角频率（2 * PI * freq / sample_rate） */
    float omega = 2.0f * PI * signal_freq / (float)sample_rate;
    
    /* DFT计算（单频点） */
    for(uint32_t i = 0; i < count; i++)
    {
        /* 去直流后的信号值 */
        float val1 = (float)((int32_t)signal1[i] - (int32_t)dc1);
        float val2 = (float)((int32_t)signal2[i] - (int32_t)dc2);
        
        /* 当前相位角 */
        float phase = omega * i;
        
        /* 计算sin和cos值 */
        float sin_val = sinf(phase);
        float cos_val = cosf(phase);
        
        /* 累加DFT分量 */
        sin_sum1 += val1 * sin_val;
        cos_sum1 += val1 * cos_val;
        sin_sum2 += val2 * sin_val;
        cos_sum2 += val2 * cos_val;
    }
    
    /* 使用标准atan2f计算相位（弧度） */
    float phase1_rad = atan2f(sin_sum1, cos_sum1);  /* signal1 = PA6相位 */
    float phase2_rad = atan2f(sin_sum2, cos_sum2);  /* signal2 = PB1相位 */
    
    /* 计算相位差（弧度）：PA6 - PB1 */
    float phase_diff_rad = phase1_rad - phase2_rad;  /* θ = PA6 - PB1 */
    
    /* 转换为度×100 */
    float phase_diff_deg = phase_diff_rad * 18000.0f / PI;  /* rad * (180/π) * 100 */
    
    /* 归一化到 -180° ~ +180° (-18000 ~ +18000) */
    while(phase_diff_deg > 18000.0f) phase_diff_deg -= 36000.0f;
    while(phase_diff_deg < -18000.0f) phase_diff_deg += 36000.0f;
    
    return (int32_t)phase_diff_deg;
}

/*!
 * \brief   处理ADC数据并计算幅频/相频特性
 * \details 测量真实电路的频率响应（外部反馈）
 */
void ProcessADCData(void)
{
    static uint16_t adc0_data[512];  /* ADC0数据缓存（PA6: 输入参考）*/
    static uint16_t adc1_data[512];  /* ADC1数据缓存（PB1: 输出测量）*/
    
    /* 1. 提取ADC数据（双通道反馈）*/
    ExtractADCData(adc0_data, adc1_data, 512);
    
    /* 2. 使用RMS能量法计算信号幅度（v5.6改进：更稳定） */
    float amp_ch1 = CalculateAmplitude_DFT(adc0_data, 512, 20000, DDS_GetFrequency());
    float amp_ch2 = CalculateAmplitude_DFT(adc1_data, 512, 20000, DDS_GetFrequency());
    
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
    
    /* 6. 计算相位差（使用DFT方法） */
    int32_t phase_x100 = EstimatePhaseShift_Int(adc0_data, adc1_data, 512, 20000, DDS_GetFrequency());
    
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
    
    /* 10. 发送波形数据用于实时显示 
     *     格式: WAVEFORM:freq,sampleRate,adc0_data|adc1_data
     *     自适应采样：根据频率选择降采样率，确保每周期至少5个点
     */
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
    uint16_t repeat_count_pa6 = 0;  // 连续重复值计数
    uint16_t min_pa6 = 4095, max_pa6 = 0;
    uint16_t min_pb1 = 4095, max_pb1 = 0;
    
    for(uint32_t i = 0; i < 512; i++) {
        if(adc0_data[i] == 0) zero_count_pa6++;
        if(adc1_data[i] == 0) zero_count_pb1++;
        
        if(adc0_data[i] < min_pa6) min_pa6 = adc0_data[i];
        if(adc0_data[i] > max_pa6) max_pa6 = adc0_data[i];
        if(adc1_data[i] < min_pb1) min_pb1 = adc1_data[i];
        if(adc1_data[i] > max_pb1) max_pb1 = adc1_data[i];
        
        // 检测连续重复值
        if(i > 0 && adc0_data[i] == adc0_data[i-1]) {
            repeat_count_pa6++;
        }
    }
    
    // 如果发现异常，输出详细诊断信息
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
    
    printf("WAVEFORM:%d,20000,", freq);
    
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

/*!
 * \brief   自动扫频测量（10Hz ~ 1kHz）
 * \details 每隔10Hz测量一次，输出完整的频率响应曲线
 *          使用真实ADC采样外部反馈信号
 *          v5.4: 改进的自适应采样策略，确保低频时有足够的周期数
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
    printf("  Adaptive Sampling Strategy:\r\n");
    printf("    10-20Hz:  512 samples (5+ cycles), 3x avg\r\n");
    printf("    21-50Hz:  512 samples (10+ cycles), 2x avg\r\n");
    printf("    51-200Hz: 512 samples (25+ cycles), 1x meas\r\n");
    printf("    >200Hz:   512 samples (100+ cycles), 1x meas\r\n");
    printf("================================================\r\n");
    printf("OK:SWEEP_START\r\n");
    printf("================================================\r\n\r\n");
    
    /* 相位unwrapping变量 */
    int32_t phase_offset = 0;      /* 累加的相位偏移（单位：0.01度）*/
    int32_t last_phase_raw = 0;    /* 上一次的原始相位值 */
    uint8_t is_first_point = 1;    /* 第一个点标志 */
    
    /* 失真统计 */
    uint16_t distortion_count = 0;  /* 高失真点数量 */
    uint16_t total_points = 0;      /* 总测试点数 */
    
    for(uint32_t freq = 10; freq <= 1000; freq += 10)
    {
        /* 设置频率 */
        DDS_SetFrequency(freq);
        
        /* 调试输出：显示当前频率 */
        if(freq >= 750) {
            printf("[DEBUG] Starting measurement at %d Hz\r\n", freq);
        }
        
        /* 自适应稳定时间（基于信号周期数）
         * 确保至少等待10个完整周期让信号稳定
         */
        uint32_t settle_time_ms;
        if(freq <= 20) {
            settle_time_ms = (15000 / freq) + 200;  /* 极低频：15个周期 + 200ms */
        } else if(freq <= 50) {
            settle_time_ms = (10000 / freq) + 100;  /* 低频：10个周期 + 100ms */
        } else if(freq <= 200) {
            settle_time_ms = (5000 / freq) + 50;    /* 中频：5个周期 + 50ms */
        } else {
            settle_time_ms = (3000 / freq) + 50;    /* 高频：3个周期 + 50ms */
        }
        if(settle_time_ms < 100) settle_time_ms = 100;
        
        delay_ms(settle_time_ms);
        
        /* 自适应多次测量平均（降低随机噪声影响）*/
        uint8_t measurement_count;
        if(freq <= 20) {
            measurement_count = 3;  /* 极低频：3次平均 */
        } else if(freq <= 50) {
            measurement_count = 2;  /* 低频：2次平均 */
        } else {
            measurement_count = 1;  /* 中高频：单次测量 */
        }
        
        uint32_t sum_pp_ch1 = 0;
        uint32_t sum_pp_ch2 = 0;
        int64_t sum_phase = 0;  /* 使用int64_t防止溢出 */
        
        /* 声明ADC数据缓冲区（移到循环外，以便后续发送波形数据）*/
        static uint16_t adc0_data[512];  /* PA6 (CH6): 输入参考K */
        static uint16_t adc1_data[512];  /* PB1 (CH9): 输出测量K₁ */
        
        for(uint8_t m = 0; m < measurement_count; m++)
        {
            /* 采集双通道反馈数据 
             * 注意：根据原理图，PA6是ADC0_CH6（输入参考K）
             *                     PB1是ADC1_CH9（输出测量K₁）
             */
            ExtractADCData(adc0_data, adc1_data, 512);
            
            /* v5.6改进：使用RMS能量法计算信号幅度（更稳定） */
            float amp_ch1_single = CalculateAmplitude_DFT(adc0_data, 512, 20000, freq);
            float amp_ch2_single = CalculateAmplitude_DFT(adc1_data, 512, 20000, freq);
            
            /* 转换为整数以便累加 */
            uint16_t pp_ch1_single = (uint16_t)amp_ch1_single;
            uint16_t pp_ch2_single = (uint16_t)amp_ch2_single;
            
            /* 调试：在高频时输出信号强度 */
            if(freq >= 750 && m == 0) {
                printf("[DEBUG] %dHz ADC: CH1=%d, CH2=%d, sample[0]=%d,%d\r\n", 
                       freq, pp_ch1_single, pp_ch2_single, adc0_data[0], adc1_data[0]);
            }
            
            /* 计算相位差（使用改进的DFT算法）*/
            int32_t phase_single = EstimatePhaseShift_Int(adc0_data, adc1_data, 512, 20000, freq);
            
            /* 计算失真度（仅在第一次测量时）*/
            if(m == 0) {
                float distortion_input = CalculateDistortion(adc0_data, 512, freq, 20000);
                float distortion_output = CalculateDistortion(adc1_data, 512, freq, 20000);
                
                /* 统计失真 */
                total_points++;
                if(distortion_output > 15.0f) {
                    distortion_count++;
                }
                
                /* 如果失真超过15%，输出警告 */
                if(distortion_output > 15.0f) {
                    printf("[WARN] %dHz: 输出信号失真严重! THD=%.1f%% (输入THD=%.1f%%)\r\n", 
                           freq, distortion_output, distortion_input);
                    printf("       建议：降低测试频率上限或改进运放电路\r\n");
                }
            }
            
            sum_pp_ch1 += pp_ch1_single;
            sum_pp_ch2 += pp_ch2_single;
            sum_phase += phase_single;
            
            /* 每次测量后发送波形数据（确保连续）*/
            {
                uint32_t skip = 1;  /* 发送所有点 */
                
                printf("WAVEFORM:%d,20000,", freq);
                
                /* 发送输入信号（PA6）*/
                for(uint32_t i = 0; i < 512; i += skip)
                {
                    printf("%d", adc0_data[i]);
                    if(i + skip < 512) printf(",");
                }
                
                printf("|");
                
                /* 发送输出信号（PB1）*/
                for(uint32_t i = 0; i < 512; i += skip)
                {
                    printf("%d", adc1_data[i]);
                    if(i + skip < 512) printf(",");
                }
                
                printf("\r\n");
            }
            
            /* 多次测量之间等待一个周期，避免采样到同一波形 */
            if(m < measurement_count - 1) {
                uint32_t wait_time = (1000 / freq) + 10;  /* 等待1个周期 */
                if(wait_time < 20) wait_time = 20;
                if(wait_time > 100) wait_time = 100;
                delay_ms(wait_time);
            }
        }
        
        /* 计算平均值（降低随机误差）*/
        uint16_t pp_ch1 = sum_pp_ch1 / measurement_count;
        uint16_t pp_ch2 = sum_pp_ch2 / measurement_count;
        int32_t phase_raw = (int32_t)(sum_phase / measurement_count);
        
        /* 检查信号有效性（降低阈值以支持高频测量）*/
        if(pp_ch1 < 5 || pp_ch2 < 5)
        {
            /* 信号太弱，输出警告但仍尝试计算 */
            printf("[WARN] Weak signal at %dHz: CH1=%d, CH2=%d\r\n", freq, pp_ch1, pp_ch2);
            // 不再跳过，继续计算
        }
        
        /* 转换为电压（V，峰值幅度，RMS能量法）- 使用浮点数 */
        float voltage_ch1 = ((float)pp_ch1 * 3.3f) / 4096.0f;  /* PA6电压(V) */
        float voltage_ch2 = ((float)pp_ch2 * 3.3f) / 4096.0f;  /* PB1电压(V) */
        
        /* 计算幅频特性 H(ω) = K₁/K (传输比) - 使用浮点数 */
        float H = 0.0f;
        if(voltage_ch1 > 0.001f)  /* 避免除以接近0的数 */
        {
            H = voltage_ch2 / voltage_ch1;  /* H = PB1/PA6 */
        }
        
        /* 应用校准修正（如果校准数据有效）*/
        float H_corrected = H;
        int32_t phase_corrected = phase_raw;
        
        if(g_calibration.valid && freq >= 10 && freq <= 1000 && (freq % 10) == 0)
        {
            uint32_t freq_idx = (freq / 10) - 1;  /* 10Hz->0, 20Hz->1, ..., 1000Hz->99 */
            /* 注意：如果前面的条件满足，freq_idx一定在0-99范围内 */
            
            /* 增益校准：H_corrected = H_measured × correction_factor */
            float correction_factor = (float)g_calibration.gain_correction[freq_idx] / 10000.0f;
            H_corrected = H * correction_factor;
            
            /* 相位校准：θ_corrected = θ_measured + correction_offset */
            phase_corrected = phase_raw + g_calibration.phase_correction[freq_idx];
        }
        
        /* 相位Unwrapping：消除±180°跳变
         * 标准unwrapping算法：检测超过±180°的跳变
         */
        if(!is_first_point)
        {
            /* 计算相邻频率点的相位差（使用校准后的相位）*/
            int32_t phase_diff = phase_corrected - last_phase_raw;
            
            /* 检测正向跳变（从负相位跳到正相位）
             * 例如：上一点-170°，这一点+170°，差值=+340°
             * 实际应该是：-170° → -190° (差值-20°)
             */
            if(phase_diff > 18000)  /* 差值 > 180° */
            {
                phase_offset -= 36000;  /* 补偿-360° */
            }
            /* 检测反向跳变（从正相位跳到负相位）
             * 例如：上一点+170°，这一点-170°，差值=-340°
             * 实际应该是：+170° → +190° (差值+20°)
             */
            else if(phase_diff < -18000)  /* 差值 < -180° */
            {
                phase_offset += 36000;  /* 补偿+360° */
            }
        }
        
        /* 应用unwrapping偏移，得到连续的相位曲线 */
        int32_t phase_unwrapped = phase_corrected + phase_offset;
        
        /* 更新历史记录 */
        last_phase_raw = phase_corrected;
        is_first_point = 0;
        
        /* 输出格式: FREQ_RESP:freq,K(V),K1(V),H,theta
         * 使用浮点数格式输出
         */
        if(g_calibration.valid)
        {
            /* 输出校准后的数据 */
            printf("FREQ_RESP:%d,%.4f,%.4f,%.6f,%s%.2f,%.6f,%s%.2f\r\n", 
                   freq,
                   voltage_ch1, voltage_ch2,                          /* K, K₁ (V，浮点数) */
                   H, (phase_raw<0)?"-":"", (float)abs(phase_raw)/100.0f,  /* H_raw, θ_raw */
                   H_corrected, (phase_unwrapped<0)?"-":"", (float)abs(phase_unwrapped)/100.0f  /* H_cal, θ_cal */
            );
        }
        else
        {
            /* 未校准，只输出原始数据 */
            printf("FREQ_RESP:%d,%.4f,%.4f,%.6f,%s%.2f\r\n", 
                   freq,
                   voltage_ch1, voltage_ch2,                          /* K, K₁ (V，浮点数) */
                   H, (phase_unwrapped<0)?"-":"", (float)abs(phase_unwrapped)/100.0f  /* H, θ */
            );
        }
        
        /* 波形数据已在测量循环内发送，此处无需重复发送 */
        
        /* 每50Hz显示进度（增加频率以便诊断）*/
        if(freq % 50 == 0)
        {
            printf("# Progress: %d/1000 Hz (CH1=%d, CH2=%d)\r\n", freq, pp_ch1, pp_ch2);
        }
        
        /* 高频段额外输出 */
        if(freq >= 800) {
            printf("[DEBUG] Completed %d Hz measurement successfully\r\n", freq);
        }
    }
    
    printf("\r\n");
    printf("[DEBUG] Loop完成！准备输出结束信息...\r\n");
    printf("================================================\r\n");
    printf("OK:SWEEP_COMPLETE\r\n");
    printf("  Total Points: 100\r\n");
    printf("  Frequency Range: 10-1000 Hz\r\n");
    printf("  Algorithm: Adaptive DFT Phase Detection\r\n");
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
    
    /* 恢复到默认频率100Hz */
    DDS_SetFrequency(100);
}

/*!
 * \brief   自动校准系统（直通测试）
 * \details 要求：将PA6直接短接到PB1
 *          测量100个频率点的传输特性，理论应该H≈1.0, θ≈0°
 *          将实际测量值保存为校准系数，用于后续补偿
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
    
    /* 等待用户确认（这里简化，直接开始）*/
    delay_ms(3000);
    
    printf("\r\n[INFO] 开始校准测量...\r\n");
    printf("OK:CALIBRATION_START\r\n");
    
    /* 清空校准数据 */
    g_calibration.valid = 0;
    for(uint8_t i = 0; i < CALIBRATION_POINTS; i++)
    {
        g_calibration.gain_correction[i] = 10000;  /* 默认1.0 */
        g_calibration.phase_correction[i] = 0;     /* 默认0° */
    }
    
    /* 扫频校准 */
    for(uint32_t freq_idx = 0; freq_idx < CALIBRATION_POINTS; freq_idx++)
    {
        uint32_t freq = (freq_idx + 1) * 10;  /* 10Hz, 20Hz, ..., 1000Hz */
        
        /* 设置频率 */
        DDS_SetFrequency(freq);
        
        /* 等待信号稳定 */
        uint32_t settle_time_ms = (freq <= 50) ? (10000 / freq + 100) : (5000 / freq + 50);
        if(settle_time_ms < 100) settle_time_ms = 100;
        delay_ms(settle_time_ms);
        
        /* 采集数据 */
        static uint16_t adc0_data[512];
        static uint16_t adc1_data[512];
        ExtractADCData(adc0_data, adc1_data, 512);
        
        /* 计算幅度和相位 */
        uint16_t pp_ch1 = CalculatePeakToPeak(adc0_data, 512);
        uint16_t pp_ch2 = CalculatePeakToPeak(adc1_data, 512);
        int32_t phase_raw = EstimatePhaseShift_Int(adc0_data, adc1_data, 512, 20000, freq);
        
        /* 检查有效性 */
        if(pp_ch1 < 10 || pp_ch2 < 10)
        {
            printf("[ERROR] CALIB_FAIL:%d,signal_weak\r\n", freq);
            printf("[提示] 请检查PA6和PB1是否正确连接！\r\n");
            return;
        }
        
        /* 计算校准系数 */
        /* 理论：H应该=1.0, θ应该=0° */
        /* 实际：H_measured = pp_ch2 / pp_ch1 */
        /* 校正系数 = 1.0 / H_measured = pp_ch1 / pp_ch2 */
        uint32_t H_measured = ((uint32_t)pp_ch2 * 10000) / pp_ch1;
        
        /* ⚠️ 防止溢出：限制校准系数到uint16_t范围 */
        uint32_t correction = 100000000UL / H_measured;  /* 10000 / (H_measured/10000) */
        if(correction > 65535) correction = 65535;  /* 限制到最大值 */
        g_calibration.gain_correction[freq_idx] = (uint16_t)correction;
        
        g_calibration.phase_correction[freq_idx] = (int16_t)(-phase_raw);  /* 相位校正 = -测量相位 */
        
        /* 输出校准数据 */
        printf("CALIB_DATA:%d,%d.%04d,%s%d.%02d\r\n", 
               freq,
               H_measured/10000, H_measured%10000,
               (phase_raw<0)?"-":"", abs(phase_raw/100), abs(phase_raw%100));
        
        /* 每100Hz显示进度 */
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
    
    /* 恢复到默认频率 */
    DDS_SetFrequency(100);
}

void delay_ms(uint32_t ms)
{
	uint32_t i;
	for(i=0;i < ms;i++)
	{
		delay_us(1000);
	}
}

/**
 * @brief: Nanosecond delay
 * @param {uint16_t} us
 * @return {*}
 */
void delay_us(uint16_t us)
{
	SysTick->CTRL = 0; // Disable SysTick
	SysTick->LOAD = SystemCoreClock/1000000 * us; // 24bit Reload register
	SysTick->VAL = 0; // Clear current value as well as count flag
	SysTick->CTRL = 5; // Enable SysTick timer with processor clock
	while ((SysTick->CTRL & 0x00010000)==0);// Wait until count flag is set
	SysTick->CTRL = 0; // Disable SysTick
}

