#include "usart.h"
#include "../../USER/main.h"

/* 重定向printf函数 */
int fputc(int ch, FILE *f)
{
    usart_data_transmit(USART0, (uint8_t)ch);
    while(RESET == usart_flag_get(USART0, USART_FLAG_TBE));
    return ch;
}

void USART0_Init(uint32_t baudrate)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_USART0);
    
    gpio_init(GPIOA,GPIO_MODE_AF_PP,GPIO_OSPEED_50MHZ,GPIO_PIN_9);//TX
    gpio_init(GPIOA,GPIO_MODE_IN_FLOATING,GPIO_OSPEED_50MHZ,GPIO_PIN_10);//RX  
    //96N81
    usart_baudrate_set( USART0,  baudrate);
    usart_parity_config( USART0,  USART_PM_NONE);
    usart_word_length_set( USART0,  USART_WL_8BIT);
    usart_stop_bit_set( USART0,  USART_STB_1BIT);
    usart_hardware_flow_rts_config(USART0, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(USART0, USART_CTS_DISABLE);
    
    usart_transmit_config( USART0,  USART_TRANSMIT_ENABLE);
    usart_receive_config( USART0,  USART_RECEIVE_ENABLE);
    usart_interrupt_enable( USART0, USART_INT_RBNE);
    
    /* UART优先级低于TIMER2 (1,1) < (0,0)，确保DDS波形生成不被阻塞 */
    nvic_irq_enable(USART0_IRQn,1, 1);
    usart_enable(USART0);
}


/* UART接收缓冲区 */
#define UART_RX_BUFFER_SIZE 64
static char uart_rx_buffer[UART_RX_BUFFER_SIZE];
static uint8_t uart_rx_index = 0;

/* 数据流控制 */
static uint8_t uart_stream_enable = 0;  /* 0=禁用, 1=启用数据流 */
static uint32_t uart_timestamp = 0;     /* 时间戳计数器 */

/* 字符串比较函数 */
static int str_compare(const char *str1, const char *str2, uint32_t len)
{
    for(uint32_t i = 0; i < len; i++)
    {
        if(str1[i] != str2[i]) return 1;
        if(str1[i] == '\0') return 0;
    }
    return 0;
}

/* 字符串转整数 */
static uint32_t str_to_uint(const char *str)
{
    uint32_t result = 0;
    while(*str >= '0' && *str <= '9')
    {
        result = result * 10 + (*str - '0');
        str++;
    }
    return result;
}

/* 处理UART命令 */
static void process_uart_command(void)
{
    extern void DDS_SetFrequency(uint32_t freq_hz);
    extern uint32_t DDS_GetFrequency(void);
    extern void DDS_Start(void);
    extern void DDS_Stop(void);
    
    /* FREQ:xxx - 设置频率 */
    if(str_compare(uart_rx_buffer, "FREQ:", 5) == 0)
    {
        uint32_t freq = str_to_uint(uart_rx_buffer + 5);
        if(freq >= 0 && freq <= 1000)  /* 允许0Hz作为特殊用途 */
        {
            DDS_SetFrequency(freq);
            printf("OK:FREQ:%uHz (DAC5311 -> PB1)\r\n", (unsigned int)freq);
        }
        else
        {
            printf("ERROR:FREQ_RANGE (10-1000Hz)\r\n");
        }
    }
    /* TYPE:SINE - 切换到正弦波模式 */
    else if(str_compare(uart_rx_buffer, "TYPE:SINE", 9) == 0)
    {
        extern SignalType_t g_signal_type;
        g_signal_type = SIGNAL_TYPE_SINE;
        printf("OK:TYPE:SINE\r\n");
    }
    /* TYPE:ECG - 切换到心电ECG模式 */
    else if(str_compare(uart_rx_buffer, "TYPE:ECG", 8) == 0)
    {
        extern SignalType_t g_signal_type;
        g_signal_type = SIGNAL_TYPE_ECG;
        printf("OK:TYPE:ECG\r\n");
    }
    /* FREQ_TEST:xxx - 频率测试（Web兼容）*/
    else if(str_compare(uart_rx_buffer, "FREQ_TEST:", 10) == 0)
    {
        uint32_t freq = str_to_uint(uart_rx_buffer + 10);
        DDS_SetFrequency(freq);
        /* 开启数据流 */
        uart_stream_enable = 1;
        uart_timestamp = 0;
        printf("OK:FREQ_TEST_STARTED:%uHz\r\n", (unsigned int)freq);
    }
    /* START - 开始数据流（Web应用兼容） */
    else if(str_compare(uart_rx_buffer, "START", 5) == 0)
    {
        uart_stream_enable = 1;
        uart_timestamp = 0;
        printf("OK:STREAM_STARTED\r\n");
    }
    /* STOP - 停止数据流（Web应用兼容） */
    else if(str_compare(uart_rx_buffer, "STOP", 4) == 0)
    {
        uart_stream_enable = 0;
        printf("OK:STREAM_STOPPED\r\n");
    }
    /* STREAM:START - 开始数据流（兼容旧命令） */
    else if(str_compare(uart_rx_buffer, "STREAM:START", 12) == 0)
    {
        uart_stream_enable = 1;
        uart_timestamp = 0;
        printf("OK:STREAM_STARTED\r\n");
    }
    /* STREAM:STOP - 停止数据流（兼容旧命令） */
    else if(str_compare(uart_rx_buffer, "STREAM:STOP", 11) == 0)
    {
        uart_stream_enable = 0;
        printf("OK:STREAM_STOPPED\r\n");
    }
    /* STATUS - 查询状态 */
    else if(str_compare(uart_rx_buffer, "STATUS", 6) == 0)
    {
        extern uint32_t DDS_GetFrequency(void);
        extern uint8_t DDS_IsEnabled(void);
        uint32_t freq = DDS_GetFrequency();
        printf("\r\n========================================\r\n");
        printf("  Bode Plot Analyzer Status\r\n");
        printf("========================================\r\n");
        printf("Frequency: %u Hz\r\n", (unsigned int)freq);
        printf("Signal Path: DDS -> SPI1 -> DAC5311 -> PB1\r\n");
        printf("DDS Enabled: %s\r\n", DDS_IsEnabled() ? "YES" : "NO");
        printf("Stream: %s\r\n", uart_stream_enable ? "ON" : "OFF");
        printf("========================================\r\n\r\n");
    }
    /* DEBUG - 调试DDS状态 */
    else if(str_compare(uart_rx_buffer, "DEBUG", 5) == 0)
    {
        extern uint8_t DDS_IsEnabled(void);
        extern uint32_t TIMER2_GetInterruptCount(void);
        extern uint32_t DDS_GetFrequency(void);
        extern uint8_t DDS_GetSineIndex(void);
        extern uint32_t DDS_GetPhaseAccumulator(void);
        extern uint32_t DDS_GetPhaseIncrement(void);
        
        printf("\r\n======== BODE ANALYZER DEBUG ========\r\n");
        printf("Frequency: %u Hz\r\n", (unsigned int)DDS_GetFrequency());
        printf("DDS Enabled: %u (1=ON, 0=OFF)\r\n", (unsigned int)DDS_IsEnabled());
        printf("Sine Index: %u / 255\r\n", (unsigned int)DDS_GetSineIndex());
        printf("Phase Acc: 0x%08lX\r\n", (unsigned long)DDS_GetPhaseAccumulator());
        printf("Phase Inc: 0x%08lX\r\n", (unsigned long)DDS_GetPhaseIncrement());
        
        /* TIMER2中断计数测试（DDS更新） */
        uint32_t count1 = TIMER2_GetInterruptCount();
        delay_ms(100);  /* 等待100ms */
        uint32_t count2 = TIMER2_GetInterruptCount();
        printf("\r\nTIMER2 DDS Update Test (100ms interval):\r\n");
        printf("  Count1: %u\r\n", (unsigned int)count1);
        printf("  Count2: %u\r\n", (unsigned int)count2);
        printf("  Delta:  %u (should be ~5000 for 50kHz)\r\n", (unsigned int)(count2-count1));
        
        printf("\r\nDiagnosis:\r\n");
        
        /* 检查TIMER2中断 */
        if((count2 - count1) < 100)
        {
            printf("❌ TIMER2 interrupt NOT running! (delta=%u, expected~5000)\r\n", (unsigned int)(count2-count1));
            printf("   Check: 1) NVIC configuration\r\n");
            printf("          2) Timer enable status\r\n");
            printf("          3) Interrupt vector table\r\n");
        }
        else
        {
            printf("✓ TIMER2 DDS update is running! (delta=%u)\r\n", (unsigned int)(count2-count1));
            printf("✓ PB1 should output sine wave via DAC5311\r\n");
            printf("  - Frequency: %u Hz\r\n", (unsigned int)DDS_GetFrequency());
            printf("  - Signal path: SPI1 -> DAC5311 -> Filter -> PB1\r\n");
        }
        
        /* 检查DDS使能 */
        if(DDS_IsEnabled() == 0)
        {
            printf("❌ DDS is DISABLED! Call DDS_Start() to enable.\r\n");
        }
        else
        {
            printf("✓ DDS is enabled.\r\n");
        }
        printf("========================================\r\n\r\n");
    }
    /* MEASURE - 立即测量一次 */
    else if(str_compare(uart_rx_buffer, "MEASURE", 7) == 0)
    {
        extern void ProcessADCData(void);
        printf("OK:MEASURING...\r\n");
        ProcessADCData();
    }
    /* SWEEP - 自动扫频测量 */
    else if(str_compare(uart_rx_buffer, "SWEEP", 5) == 0)
    {
        extern void AutoSweep(void);
        printf("OK:STARTING_SWEEP\r\n");
        AutoSweep();
    }
    /* CALIBRATE - 系统校准 */
    else if(str_compare(uart_rx_buffer, "CALIBRATE", 9) == 0 || 
            str_compare(uart_rx_buffer, "CALIB", 5) == 0)
    {
        extern void AutoCalibration(void);
        printf("OK:STARTING_CALIBRATION\r\n");
        AutoCalibration();
    }
    /* LED:x - 设置LED模式 */
    else if(str_compare(uart_rx_buffer, "LED:", 4) == 0)
    {
        extern void LED_Set_Mode(uint8_t mode);
        uint32_t mode = str_to_uint(uart_rx_buffer + 4);
        if(mode <= 6)
        {
            LED_Set_Mode(mode);
            const char *mode_names[] = {"OFF", "Flow", "Breath", "Blink", "Specific", "Rotate", "Alarm"};
            printf("OK:LED_MODE:%s\r\n", mode_names[mode]);
        }
        else
        {
            printf("ERROR:LED_MODE (0-6)\r\n");
        }
    }
    /* LED_BRIGHT:x - 设置LED亮度 */
    else if(str_compare(uart_rx_buffer, "LED_BRIGHT:", 11) == 0)
    {
        extern void LED_Set_Brightness(uint8_t brightness);
        uint32_t brightness = str_to_uint(uart_rx_buffer + 11);
        if(brightness <= 100)
        {
            LED_Set_Brightness((uint8_t)brightness);
            printf("OK:LED_BRIGHTNESS:%u%%\r\n", (unsigned int)brightness);
        }
        else
        {
            printf("ERROR:LED_BRIGHTNESS (0-100)\r\n");
        }
    }
    /* LED_FREQ:x - 设置LED频率/间隔 */
    else if(str_compare(uart_rx_buffer, "LED_FREQ:", 9) == 0)
    {
        extern void LED_Set_Interval(uint16_t interval_ms);
        uint32_t freq = str_to_uint(uart_rx_buffer + 9);
        if(freq >= 1 && freq <= 30000)
        {
            LED_Set_Interval((uint16_t)freq);
            printf("OK:LED_INTERVAL:%ums\r\n", (unsigned int)freq);
        }
        else
        {
            printf("ERROR:LED_INTERVAL (1-30000ms)\r\n");
        }
    }
    /* LED_MASK:x - 设置特定LED掩码 (bit0-4对应LED3,LED7,LED4,LED6,LED5) */
    else if(str_compare(uart_rx_buffer, "LED_MASK:", 9) == 0)
    {
        extern void LED_Set_Specific_LED(uint8_t led_mask);
        uint32_t mask = str_to_uint(uart_rx_buffer + 9);
        if(mask <= 0x1F)  // 5位掩码，最大31
        {
            LED_Set_Specific_LED((uint8_t)mask);
            printf("OK:LED_MASK:0x%02X\r\n", (unsigned int)mask);
        }
        else
        {
            printf("ERROR:LED_MASK (0-31, 0x00-0x1F)\r\n");
        }
    }
    /* HELP - 帮助信息 */
    else if(str_compare(uart_rx_buffer, "HELP", 4) == 0)
    {
        printf("\r\n========================================\r\n");
        printf("  Bode Plot Analyzer Commands\r\n");
        printf("========================================\r\n");
        printf("Frequency Control:\r\n");
        printf("  FREQ:xxx      - Set frequency (10-1000Hz)\r\n");
        printf("                  Example: FREQ:100\r\n\r\n");
        printf("Output Control:\r\n");
        printf("  START         - Start DDS output\r\n");
        printf("  STOP          - Stop DDS output\r\n\r\n");
        printf("Measurement:\r\n");
        printf("  MEASURE       - Measure H(ω) and θ(ω)\r\n");
        printf("  SWEEP         - Auto sweep 10Hz-1kHz\r\n");
        printf("  CALIBRATE     - System calibration (PA6->PB1 direct)\r\n\r\n");
        printf("Status:\r\n");
        printf("  STATUS        - Query system status\r\n");
        printf("  DEBUG         - Show debug info\r\n");
        printf("  HELP          - Show this help\r\n\r\n");
        printf("LED Control:\r\n");
        printf("  LED:0-6       - Set LED mode\r\n");
        printf("                  0=OFF, 1=Flow, 2=Breath, 3=Blink\r\n");
        printf("                  4=Specific, 5=Rotate, 6=Alarm\r\n");
        printf("  LED_BRIGHT:x  - Set brightness (0-100%%)\r\n");
        printf("  LED_FREQ:x    - Set interval (1-30000ms)\r\n");
        printf("  LED_MASK:x    - Set LED mask for Specific mode (0-31)\r\n");
        printf("                  bit0=LED3, bit1=LED7, bit2=LED4\r\n");
        printf("                  bit3=LED6, bit4=LED5\r\n\r\n");
        printf("Hardware:\r\n");
        printf("  Signal Out: PB1 (DAC5311 filtered output)\r\n");
        printf("  ADC Input: PA6 (Input reference K)\r\n");
        printf("  ADC Output: PB1 (DUT output K₁)\r\n");
        printf("  LED: GPIOB (PB11-PB15)\r\n");
        printf("========================================\r\n\r\n");
    }
    else
    {
        printf("Unknown: %s (type HELP for commands)\r\n", uart_rx_buffer);
    }
}

/*!
 * \brief   发送数据流（在TIMER2中断中调用）
 * \param   sample DDS采样值
 * \param   adc0   ADC0数据 (PA6)
 * \param   adc1   ADC1数据 (PB1)
 */
void UART_SendStreamData(uint8_t sample, uint16_t adc0, uint16_t adc1)
{
    if(uart_stream_enable == 0) return;
    
    extern uint32_t DDS_GetFrequency(void);
    uint32_t freq;
    
    /* ECG模式下使用心率对应的频率（86 BPM约= 1.43 Hz，简化为1 Hz） */
    if(g_signal_type == SIGNAL_TYPE_ECG)
    {
        freq = 1;  /* ECG心率频率标识，前端可用于判断显示模式 */
    }
    else
    {
        freq = DDS_GetFrequency();
    }
    
    /* 发送符合网页端 WAVEFORM 格式的数据: WAVEFORM:freq,sampleRate,input|output */
    /* ECG模式1000Hz，正弦波500Hz (由 timer.c 中的 stream_divisor 控制) */
    uint16_t sample_rate = (g_signal_type == SIGNAL_TYPE_ECG) ? 1000 : 500;
    printf("WAVEFORM:%u,%u,%u|%u\r\n", (unsigned int)freq, sample_rate, adc0, adc1);
}

void USART0_IRQHandler(void)
{
    uint16_t data = 0;
    if(usart_flag_get(USART0,USART_FLAG_RBNE) != RESET)
    {
        usart_flag_clear(USART0,USART_FLAG_RBNE);
        usart_interrupt_flag_clear(USART0,USART_INT_RBNE);
        data = usart_data_receive(USART0);
        
        /* 回显 */
        usart_data_transmit(USART0,data);
        
        /* 接收命令 */
        char received_char = (char)(data & 0xFF);
        if(received_char == '\r' || received_char == '\n')
        {
            if(uart_rx_index > 0)
            {
                uart_rx_buffer[uart_rx_index] = '\0';
                process_uart_command();
                uart_rx_index = 0;
            }
        }
        else if(uart_rx_index < UART_RX_BUFFER_SIZE - 1)
        {
            uart_rx_buffer[uart_rx_index++] = received_char;
        }
        else
        {
            /* 缓冲区满，输出警告并复位 */
            printf("\r\n[ERROR] Command too long! Max %d chars.\r\n", UART_RX_BUFFER_SIZE);
            uart_rx_index = 0;
        }
    }
}
