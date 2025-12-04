#include "pwm.h"

void PWM_Init(uint16_t psc,uint16_t per)
{
    timer_parameter_struct timer_struct;
    PWM_GPIO_Init();
    rcu_periph_clock_enable(RCU_TIMER1);
    timer_struct.clockdivision = TIMER_CKDIV_DIV1;
    timer_struct.counterdirection = TIMER_COUNTER_UP;
    timer_struct.period = per;
    timer_struct.prescaler = psc;
    timer_init(TIMER1, &timer_struct);
    
    
    timer_oc_parameter_struct   timer_ocstruct;
    timer_ocstruct.ocidlestate = TIMER_OC_IDLE_STATE_HIGH;  //初始状态为高电平
    timer_ocstruct.ocpolarity = TIMER_OC_POLARITY_HIGH;     //高电平有效
    timer_ocstruct.outputstate = TIMER_CCX_ENABLE;
    
    timer_channel_output_config(TIMER1, TIMER_CH_3, &timer_ocstruct);
    
    timer_channel_output_mode_config(TIMER1, TIMER_CH_3, TIMER_OC_MODE_PWM0);       //使用PWM0模式
    timer_channel_output_shadow_config(TIMER1,TIMER_CH_3,TIMER_OC_SHADOW_DISABLE);

    /* 自动重装载影子比较器使能 */
    timer_auto_reload_shadow_enable(TIMER1);
    
    timer_enable(TIMER1);
}

void PWM_GPIO_Init(void)
{
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_AF);
    
    gpio_init(GPIOB,GPIO_MODE_AF_PP,GPIO_OSPEED_10MHZ,GPIO_PIN_11);
    gpio_pin_remap_config(GPIO_TIMER1_FULL_REMAP, ENABLE);
    
}
