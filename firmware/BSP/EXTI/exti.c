#include "exti.h"


void KEY_Init(void)
{
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_AF);
    //PB2
    gpio_init(KEY_GPIO_PORT,GPIO_MODE_IPU,GPIO_OSPEED_50MHZ,KEY_GPIO_PIN);
    KEY_EXTI_Init();
}


void KEY_EXTI_Init(void)
{
    nvic_irq_enable(EXTI2_IRQn, 0, 0);
    gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOB, GPIO_PIN_SOURCE_2);
    exti_init(EXTI_2, EXTI_INTERRUPT, EXTI_TRIG_BOTH);       //中断模式 双边沿触发
    exti_interrupt_enable(EXTI_2);
}

void EXTI2_IRQHandler(void)
{
    FlagStatus flag_pin = RESET;
    delay_ms(10);
    exti_interrupt_flag_clear(EXTI_2);
    exti_flag_clear(EXTI_2);
    flag_pin = gpio_output_bit_get(GPIOB,GPIO_PIN_11);
    gpio_bit_write(GPIOB,GPIO_PIN_11,!flag_pin);
}

