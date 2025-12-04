/*
 * @Author: leeyoung7017
 * @Description: 伯德图分析仪 - 通用频率响应测试系统 v5.6 (重构版)
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

/* 全局系统滴答计数（用于测量时间，1ms递增） */
volatile uint32_t systick_ms = 0;

/* 工具函数声明 */
void delay_ms(uint32_t ms);
void delay_us(uint16_t us);

int main()
{
    /* 0. 初始化SysTick为1ms定时器（用于测量时间统计） */
    SysTick_Config(SystemCoreClock / 1000);  /* 72MHz / 1000 = 1ms */
    
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
    
    /* 9. 初始化TIMER3（自适应采样率 - ADC触发）*/
    TIMER3_ADC_Init(1000);  /* 默认1kHz采样率（100Hz × 10倍）*/
    printf("[OK] TIMER3 initialized (1kHz ADC trigger, adaptive mode).\r\n");
    
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
 * @brief: Microsecond delay
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

/**
 * @brief: SysTick中断处理函数（每1ms触发一次，用于时间统计）
 */
void SysTick_Handler(void)
{
	systick_ms++;
}
