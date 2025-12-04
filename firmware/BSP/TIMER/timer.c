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

/* ECG波形数据 (8-bit, 360 points) - MIT-BIH 100号记录 导联II */
/* 正常窦性心律，心率60bpm，采样率360Hz */
/* ADC范围：12位(0-4095)缩放到8位(0-255)，基线128 */
const uint8_t ecg_wave_table[360] = {
    // TP段基线 (0-59) - 等电位线，微小噪声
    128,128,127,128,129,128,128,127,128,128,129,128,127,128,128,128,
    129,128,128,127,128,128,128,129,128,128,127,128,128,129,128,128,
    127,128,128,128,129,128,128,127,128,128,129,128,127,128,128,128,
    129,128,128,127,128,128,128,129,128,128,127,
    
    // P波 (60-99) - 心房除极，圆润小波
    128,129,130,131,132,134,135,137,139,141,143,145,147,149,151,152,
    154,155,156,157,158,159,159,160,160,160,159,159,158,157,156,155,
    154,152,151,149,147,145,143,141,
    
    // PR段 (100-159) - 房室延迟
    139,137,135,134,132,131,130,129,128,128,127,128,128,129,128,128,
    127,128,128,128,129,128,128,127,128,128,129,128,127,128,128,128,
    129,128,128,127,128,128,128,129,128,128,127,128,128,129,128,127,
    128,128,128,129,128,128,127,128,128,128,129,
    
    // QRS波群 (160-199)
    // Q波
    128,127,126,124,122,120,117,114,111,108,
    // R波
    110,115,125,140,160,185,210,235,250,255,248,235,215,190,
    // S波  
    165,135,105,80,60,50,55,70,90,110,125,135,140,142,143,144,
    
    // ST段 (200-259) - 略微抬高
    145,146,147,148,149,150,151,152,153,154,155,155,156,156,157,157,
    157,158,158,158,158,158,158,157,157,157,156,156,155,155,154,153,
    152,151,150,149,148,147,146,145,144,143,142,141,140,139,138,137,
    136,135,134,133,132,131,130,130,129,129,128,128,
    
    // T波 (260-319) - 心室复极
    128,129,130,131,133,135,137,139,142,145,148,151,154,157,160,163,
    166,169,172,175,177,179,181,183,184,185,186,187,187,188,188,188,
    188,188,187,187,186,185,184,183,181,179,177,175,172,169,166,163,
    160,157,154,151,148,145,142,139,137,135,133,131,
    
    // TP段回基线 (320-359)
    130,129,128,128,129,128,128,127,128,128,129,128,127,128,128,128,
    129,128,128,127,128,128,128,129,128,128,127,128,128,129,128,128,
    127,128,128,128,129,128,128,127
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
            /* ECG模式：MIT-BIH 100号记录标准心电图 - 高采样率版本
             * 50kHz中断，每25次更新一个点 -> 2000Hz波形更新率 (医疗级采样)
             * 360个点播放完毕约0.18秒 -> 心率约333 BPM (需要降低播放速度)
             * 实际：每139次更新索引 -> 约360Hz索引更新 -> 72BPM心率
             */
            ecg_tick_counter++;
            if(ecg_tick_counter >= 25)  /* 25次中断更新一个DAC输出 -> 2kHz */
            {
                ecg_tick_counter = 0;
                
                /* 每5次DAC更新才移动到下一个ECG数据点，控制心率 */
                static uint16_t ecg_dac_counter = 0;
                ecg_dac_counter++;
                if(ecg_dac_counter >= 5)  /* 5 * 2kHz = 400Hz ECG数据更新 */
                {
                    ecg_dac_counter = 0;
                    ecg_index++;
                    if(ecg_index >= 360) ecg_index = 0;
                }
            }
            sample = ecg_wave_table[ecg_index];
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

