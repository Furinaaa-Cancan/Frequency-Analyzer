#include "timer_led.h"
#include "led.h"
#include "usart.h"

// 定时发送相关变量
static uint16_t send_counter = 0;           // 发送计数器
static uint16_t send_interval = 1000;       // 发送间隔（默认1秒）
static uint8_t auto_send_enabled = 0;       // 是否启用自动发送（默认禁用，避免干扰调试）

/**
 * @brief: 设置定时发送间隔
 * @param {uint16_t} interval_ms - 间隔时间（毫秒）
 */
void Timer_Set_Send_Interval(uint16_t interval_ms)
{
    if(interval_ms >= 100 && interval_ms <= 10000)  // 限制范围100ms-10s
    {
        send_interval = interval_ms;
        send_counter = 0;  // 重置计数器
    }
}

/**
 * @brief: 启用/禁用自动发送
 * @param {uint8_t} enable - 1启用，0禁用
 */
void Timer_Enable_Auto_Send(uint8_t enable)
{
    auto_send_enabled = enable;
    send_counter = 0;
}

/**
 * @brief: 获取当前发送间隔
 * @return {uint16_t} 发送间隔（毫秒）
 */
uint16_t Timer_Get_Send_Interval(void)
{
    return send_interval;
}

void TIM1_Init_LED(uint16_t psc,uint16_t per)
{
    timer_parameter_struct timer_struct;
    timer_oc_parameter_struct timer_oc_struct;
    
    rcu_periph_clock_enable(RCU_TIMER1);  // 定时器时钟72MHz
    
    // 定时器基础配置
    timer_struct.clockdivision = TIMER_CKDIV_DIV1;
    timer_struct.counterdirection = TIMER_COUNTER_UP;
    timer_struct.period = per;
    timer_struct.prescaler = psc;
    timer_init(TIMER1, &timer_struct);
    
    // 配置CH1输出参数
    timer_oc_struct.outputstate = TIMER_CCX_ENABLE;
    timer_oc_struct.outputnstate = TIMER_CCXN_DISABLE;
    timer_oc_struct.ocpolarity = TIMER_OC_POLARITY_HIGH;
    timer_oc_struct.ocnpolarity = TIMER_OCN_POLARITY_HIGH;
    timer_oc_struct.ocidlestate = TIMER_OC_IDLE_STATE_LOW;
    timer_oc_struct.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;
    timer_channel_output_config(TIMER1, TIMER_CH_1, &timer_oc_struct);
    
    // 设置CH1比较值（占空比50%）
    timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_1, per / 2);
    
    // 设置CH1输出模式为PWM0
    timer_channel_output_mode_config(TIMER1, TIMER_CH_1, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER1, TIMER_CH_1, TIMER_OC_SHADOW_DISABLE);
    
    // 使能中断
    nvic_irq_enable(TIMER1_IRQn, 1, 1);
    timer_interrupt_enable(TIMER1, TIMER_INT_UP);
    
    // 使能定时器
    timer_enable(TIMER1);
    
    printf(">>> TIMER1 Initialized <<<\r\n");
    printf("Frequency: %d Hz (triggers ADC)\r\n", 72000000 / (psc + 1) / (per + 1));
}

// 定时器1中断处理函数 - 每1ms调用一次
void TIMER1_IRQHandler(void)
{
    timer_flag_clear(TIMER1,TIMER_FLAG_UP);
    timer_interrupt_flag_clear(TIMER1,TIMER_INT_FLAG_UP);
    
    // 调用LED处理函数
    LED_Process();
    
    // ==================== 定时发送功能（任务三第3点） ====================
    // 注意：Project 2中禁用此功能，避免与现有USART冲突
    /*
    if(auto_send_enabled)
    {
        send_counter++;
        if(send_counter >= send_interval)
        {
            send_counter = 0;
            
            // 发送系统状态数据（48字节格式）
            USART_Send_Status_Data();
            
            // 或者也可以打印ASCII格式（兼容性）
            // printf("STATUS:%d,%d,%d\r\n", 
            //        LED_Get_Current_Mode(),
            //        LED_Get_Current_Interval(),
            //        LED_Get_Current_Brightness());
        }
    }
    */
}

