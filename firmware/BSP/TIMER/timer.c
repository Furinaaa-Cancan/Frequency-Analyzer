#include "timer.h"
#include "../../USER/main.h"

/*!
 * \brief   初始化TIMER2为50kHz采样率（用于DDS波形生成）
 * \details 系统时钟72MHz，APB1=36MHz，定时器时钟=72MHz
 *          72MHz / 50kHz = 1440
 *          prescaler = 0, period = 1439（计数1440次）
 *          50kHz对1000Hz信号仍有50倍采样（远超奈奎斯特定理）
 */
void TIMER2_DDS_Init(void)
{
    timer_parameter_struct timer_struct;
    
    /* 使能TIMER2时钟 */
    rcu_periph_clock_enable(RCU_TIMER2);
    
    /* 配置定时器参数 */
    timer_struct.clockdivision = TIMER_CKDIV_DIV1;
    timer_struct.counterdirection = TIMER_COUNTER_UP;
    timer_struct.period = 1439;       /* 72MHz / 1440 = 50kHz */
    timer_struct.prescaler = 0;       /* 不分频 */
    timer_init(TIMER2, &timer_struct);
    
    /* 配置NVIC - 优先级高于UART（0,0），确保DDS波形生成不被阻塞 */
    nvic_irq_enable(TIMER2_IRQn, 0, 0);
    
    /* 使能定时器更新中断 */
    timer_interrupt_enable(TIMER2, TIMER_INT_UP);
    
    /* 启动定时器 */
    timer_enable(TIMER2);
}

/* 全局计数器用于调试TIMER2中断 */
static volatile uint32_t timer2_interrupt_count = 0;

/* MIT-BIH Arrhythmia Database - Record 100 MLII导联 */
/* 真实数据来源: PhysioNet (physionet.org/content/mitdb/1.0.0) */
/* 患者: 69岁男性, 正常窦性心律, 服用Aldomet和Inderal */
/* 采样率: 360Hz, 原始分辨率: 11位ADC */
/* 数据: 第一个心跳周期 (样本0-359), R峰在样本77 */
const uint8_t ecg_wave_table[360] = {
     78, 78, 78, 78, 78, 78, 78, 78, 82, 80, 78, 77, 76, 77, 76, 73,
     73, 72, 74, 77, 73, 73, 71, 73, 77, 80, 77, 71, 69, 64, 65, 62,
     61, 60, 57, 57, 57, 59, 61, 59, 57, 55, 55, 55, 55, 56, 54, 53,
     56, 57, 57, 57, 56, 53, 55, 53, 57, 55, 53, 51, 50, 48, 44, 40,
     40, 37, 29, 24, 24, 34, 49, 66, 90,120,161,200,225,235,223,184,
    128, 75, 43, 32, 34, 42, 49, 49, 48, 46, 49, 49, 51, 52, 50, 48,
     47, 49, 46, 48, 49, 48, 49, 49, 49, 49, 46, 45, 48, 49, 53, 50,
     50, 49, 47, 48, 47, 46, 45, 45, 47, 49, 49, 49, 47, 45, 49, 49,
     49, 49, 48, 48, 47, 49, 47, 45, 45, 45, 46, 49, 50, 48, 49, 46,
     49, 48, 48, 46, 46, 45, 47, 47, 48, 49, 45, 45, 46, 48, 48, 48,
     45, 45, 45, 46, 46, 48, 45, 44, 44, 44, 43, 44, 42, 40, 42, 44,
     45, 44, 42, 42, 43, 43, 44, 44, 43, 42, 45, 49, 49, 49, 48, 47,
     51, 53, 53, 55, 54, 55, 56, 57, 61, 61, 61, 59, 61, 62, 65, 62,
     62, 61, 61, 63, 61, 61, 62, 61, 61, 61, 59, 60, 60, 59, 58, 59,
     60, 57, 57, 55, 57, 58, 60, 57, 57, 56, 57, 57, 59, 58, 56, 55,
     57, 57, 56, 57, 53, 53, 53, 54, 53, 52, 52, 53, 54, 56, 56, 55,
     54, 52, 55, 54, 53, 53, 52, 49, 52, 53, 55, 52, 49, 49, 51, 53,
     53, 52, 50, 49, 49, 51, 52, 53, 53, 52, 53, 53, 55, 53, 53, 53,
     53, 55, 57, 54, 53, 51, 53, 54, 55, 57, 58, 57, 57, 58, 61, 61,
     61, 65, 66, 69, 69, 69, 67, 65, 66, 65, 65, 65, 65, 64, 63, 64,
     66, 68, 69, 62, 56, 56, 53, 52, 49, 49, 49, 51, 50, 51, 49, 47,
     46, 47, 47, 45, 46, 45, 49, 48, 49, 50, 46, 45, 47, 49, 49, 49,
     45, 43, 44, 41, 34, 30, 26, 20
};
static uint16_t ecg_index = 0;
static uint16_t ecg_tick_counter = 0;

/*!
 * \brief   获取TIMER2中断计数（调试用）
 */
uint32_t TIMER2_GetInterruptCount(void)
{
    return timer2_interrupt_count;
}

/*!
 * \brief   TIMER2中断处理函数 - 50kHz采样率
 * \details 在此中断中生成DDS波形并输出到DAC5311
 *          自适应降采样：根据当前频率动态调整UART数据流密度
 */
void TIMER2_IRQHandler(void)
{
    static uint16_t stream_counter = 0;
    static uint16_t stream_divisor = 50;  /* 默认降采样比例 */
    
    if(timer_interrupt_flag_get(TIMER2, TIMER_INT_FLAG_UP) != RESET)
    {
        /* 清除中断标志 */
        timer_interrupt_flag_clear(TIMER2, TIMER_INT_FLAG_UP);
        
        /* 调试计数器 */
        timer2_interrupt_count++;
        
        /* 获取DDS样本 */
        extern uint8_t DDS_GetSample(void);
        extern uint32_t DDS_GetFrequency(void);
        
        uint8_t sample;
        
        if(g_signal_type == SIGNAL_TYPE_ECG)
        {
            /* ECG模式：使用和正弦波一样的DDS逻辑
             * 360点波形表，使用相位累加器控制频率
             * ECG频率设为100Hz（和正弦波默认频率一样）
             * phase_increment = freq * 85899 (和DDS一样的计算)
             * 但ECG表只有360点，需要映射：index = (phase >> 24) * 360 / 256
             */
            static uint32_t ecg_phase_accumulator = 0;
            uint32_t ecg_phase_increment = 100 * 85899UL;  /* 100Hz */
            
            /* 从相位累加器计算ECG表索引 */
            uint32_t phase_index = (ecg_phase_accumulator >> 24) & 0xFF;  /* 0-255 */
            ecg_index = (phase_index * 360) >> 8;  /* 映射到0-359 */
            
            sample = ecg_wave_table[ecg_index];
            
            /* 相位累加 */
            ecg_phase_accumulator += ecg_phase_increment;
        }
        else
        {
            /* 正弦波模式：输出DDS生成的波形 */
            sample = DDS_GetSample();
        }
        
        /* 输出到DAC5311 */
        extern void DAC5311_Write(uint8_t data);
        DAC5311_Write(sample);
        
        /* 自适应降采样：保持每周期约15个采样点 */
        stream_counter++;
        if(stream_counter >= stream_divisor)
        {
            stream_counter = 0;
            
            /* 动态计算降采样比例：每周期15个点
             * divisor = 50000 / (freq * 15)
             * 限制：最小10（5kHz数据流），最大500（100Hz数据流）
             */
            uint32_t freq = DDS_GetFrequency();
            
            if(g_signal_type == SIGNAL_TYPE_ECG || freq == 0)
            {
                /* ECG模式或0Hz：高密度数据流
                 * 50kHz / 50 = 1000Hz 数据流率 (每1ms一个点)
                 * 医疗级采样密度，确保波形平滑
                 */
                stream_divisor = 50;
            }
            else
            {
                stream_divisor = 50000 / (freq * 15);
                if(stream_divisor < 10) stream_divisor = 10;   /* 最大5kHz数据流 */
                if(stream_divisor > 500) stream_divisor = 500; /* 最小100Hz数据流 */
            }
            
            /* 读取最新的ADC数据（双通道）*/
            /* DMA格式：低16位ADC0(PA6)，高16位ADC1(PB1) */
            extern uint32_t adc_buffer[];
            uint32_t adc_val = adc_buffer[0];
            uint16_t adc0 = adc_val & 0xFFFF;
            uint16_t adc1 = (adc_val >> 16) & 0xFFFF;
            
            extern void UART_SendStreamData(uint8_t sample, uint16_t adc0, uint16_t adc1);
            UART_SendStreamData(sample, adc0, adc1);
        }
    }
}

/*!
 * \brief   初始化TIMER3作为ADC触发源（通过CH3输出比较）
 * \param   sample_rate_hz - ADC采样率（Hz），例如10000表示10kHz
 * \details 配置TIMER3的CH3输出比较事件触发ADC0+ADC1同步采样
 *          - 系统时钟72MHz，APB1=36MHz，定时器时钟=72MHz
 *          - CH3输出比较事件触发ADC（GD32F10x的ADC0_1支持T3_CH3触发）
 *          - 推荐采样率：10kHz（对1kHz信号有10倍过采样）
 * 
 * \example TIMER3_ADC_Init(10000);  // 10kHz采样率
 */
void TIMER3_ADC_Init(uint32_t sample_rate_hz)
{
    timer_parameter_struct timer_struct;
    timer_oc_parameter_struct timer_oc_struct;
    
    /* 使能TIMER3时钟 */
    rcu_periph_clock_enable(RCU_TIMER3);
    
    /* 计算定时器周期值
     * 定时器时钟 = 72MHz（APB1=36MHz，但定时器时钟x2）
     * period = (72,000,000 / sample_rate_hz) - 1
     * 例如：10kHz采样率 -> period = 7200 - 1 = 7199
     */
    uint32_t period = (72000000 / sample_rate_hz) - 1;
    
    /* 配置定时器参数 */
    timer_struct.clockdivision = TIMER_CKDIV_DIV1;       /* 时钟不分频 */
    timer_struct.counterdirection = TIMER_COUNTER_UP;    /* 向上计数 */
    timer_struct.period = period;                        /* 周期值 */
    timer_struct.prescaler = 0;                          /* 预分频器为0（不分频） */
    timer_init(TIMER3, &timer_struct);
    
    /* 配置CH3输出比较模式 */
    timer_oc_struct.outputstate = TIMER_CCX_ENABLE;      /* 使能输出（即使不连接引脚，也需要使能以产生事件） */
    timer_oc_struct.outputnstate = TIMER_CCXN_DISABLE;
    timer_oc_struct.ocpolarity = TIMER_OC_POLARITY_HIGH;
    timer_oc_struct.ocnpolarity = TIMER_OCN_POLARITY_HIGH;
    timer_oc_struct.ocidlestate = TIMER_OC_IDLE_STATE_LOW;
    timer_oc_struct.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;
    timer_channel_output_config(TIMER3, TIMER_CH_3, &timer_oc_struct);
    
    /* 设置比较值为周期的一半（在中间触发） */
    timer_channel_output_pulse_value_config(TIMER3, TIMER_CH_3, period / 2);
    
    /* 配置CH3为PWM模式0（计数值<比较值时输出有效电平） */
    timer_channel_output_mode_config(TIMER3, TIMER_CH_3, TIMER_OC_MODE_PWM0);
    
    /* 使能CH3输出比较 */
    timer_channel_output_state_config(TIMER3, TIMER_CH_3, TIMER_CCX_ENABLE);
    
    /* 启动定时器 */
    timer_enable(TIMER3);
}

/*!
 * \brief   动态设置TIMER3的ADC采样率
 * \param   sample_rate_hz - 新的采样率（Hz）
 * \details 在运行时调整采样率，无需重新初始化
 *          - 适用于自适应采样：根据信号频率调整采样率
 *          - 例如：测量100Hz信号时用5kHz采样，测量1kHz时用20kHz
 * 
 * \example TIMER3_SetSampleRate(20000);  // 切换到20kHz采样率
 */
void TIMER3_SetSampleRate(uint32_t sample_rate_hz)
{
    /* 计算新的周期值 */
    uint32_t period = (72000000 / sample_rate_hz) - 1;
    
    /* 更新定时器周期 */
    timer_autoreload_value_config(TIMER3, period);
    
    /* 同时更新CH3比较值 */
    timer_channel_output_pulse_value_config(TIMER3, TIMER_CH_3, period / 2);
}

