#include "led.h"

// LED状态变量
static led_mode_t current_mode = LED_MODE_OFF;
static uint16_t current_interval = 1000;  // 当前间隔时间(ms)，默认1秒
static uint8_t current_brightness = 100;  // 当前亮度(0-100%)，默认100%
static uint8_t specific_led_mask = 0x15;  // 特定LED掩码，默认0x15=LED3+LED4+LED5

// PWM变量（极高频PWM以完全消除闪烁感）
static uint16_t pwm_counter = 0;         // PWM计数器（0-99，10000Hz PWM）
static uint8_t pwm_duty = 100;           // PWM占空比(0-100)
#define PWM_RESOLUTION 100               // PWM分辨率（100 = 10000Hz，周期100us）

// 运行时变量
static uint32_t led_counter = 0;
static uint8_t led_index = 0;
static uint8_t led_state = 0;
static uint8_t breath_direction = 0;     // 呼吸灯方向：0=变亮，1=变暗
static uint8_t breath_brightness = 0;    // 呼吸灯当前亮度

// 时间基准分频器（TIMER1是20KHz，需要分频到1KHz）
static uint8_t time_divider = 0;
#define TIME_DIV_FACTOR 20  // 20KHz -> 1KHz (每20次中断 = 1ms)
// PWM时间基准（TIMER1是20KHz，每2次中断 = 1个PWM周期）
static uint8_t pwm_divider = 0;
#define PWM_DIV_FACTOR 2    // 20KHz -> 10KHz (每2次中断 = 100us = 1个PWM周期)

void LED_Init(void)
{
    // 使能GPIO时钟
    led_clock_enable;
    
    // 初始化所有LED引脚为输出
    LED7_Init;
    LED3_Init;
    LED4_Init;
    LED6_Init;
    LED5_Init;
    
    // 初始状态全部关闭
    LED7_L;
    LED3_L;
    LED4_L;
    LED6_L;
    LED5_L;
}

void LED_All_Off(void)
{
    LED7_L;
    LED3_L;
    LED4_L;
    LED6_L;
    LED5_L;
}

void LED_All_On(void)
{
    LED7_H;
    LED3_H;
    LED4_H;
    LED6_H;
    LED5_H;
}

void LED_Set_Mode(led_mode_t mode)
{
    current_mode = mode;
    led_counter = 0;
    led_index = 0;
    breath_brightness = 0;
    breath_direction = 0;
    
    // 如果当前亮度为0，自动设置为100%（避免LED不亮）
    if(current_brightness == 0) {
        current_brightness = 100;
    }
    pwm_duty = current_brightness;
    
    // 根据模式初始化LED状态和led_state
    LED_All_Off();
    
    // 模式初始化
    if(mode == LED_MODE_BREATH)
    {
        breath_brightness = 0;  // 从0开始渐亮
        breath_direction = 0;
        led_state = 0;  // 呼吸灯不需要led_state
    }
    else if(mode == LED_MODE_BLINK || mode == LED_MODE_SPECIFIC)
    {
        led_state = 1;  // 闪烁模式和特定LED模式，初始状态为亮
    }
    else
    {
        led_state = 0;  // 其他模式不使用led_state
    }
    
    printf("[Mode] Mode=%d, State=%d, Brightness=%d\r\n", 
           current_mode, led_state, current_brightness);
}

void LED_Set_Interval(uint16_t interval_ms)
{
    // 设置间隔时间（支持1-30000ms）
    if(interval_ms >= 1 && interval_ms <= 30000)
    {
        current_interval = interval_ms;
        led_counter = 0;
    }
}

void LED_Set_Brightness(uint8_t brightness)
{
    // 设置亮度（0-100%）
    // 使用1000Hz极高频PWM，完全消除闪烁（理论极限）
    if(brightness <= 100)
    {
        current_brightness = brightness;
        pwm_duty = brightness;
    }
}

void LED_Set_Specific_LED(uint8_t led_mask)
{
    // 设置特定LED掩码（bit0-4对应5个LED）
    specific_led_mask = led_mask & 0x1F;
}

uint8_t LED_Get_Current_Mode(void)
{
    return (uint8_t)current_mode;
}

uint16_t LED_Get_Current_Interval(void)
{
    return current_interval;
}

uint8_t LED_Get_Current_Brightness(void)
{
    return current_brightness;
}

uint8_t LED_Get_Specific_Mask(void)
{
    return specific_led_mask;
}

// 软件PWM实现（10KHz极高频PWM完全消除闪烁）
static void LED_PWM_Update(void)
{
    // PWM分频：每2次中断才更新一次PWM计数器（20KHz -> 10KHz）
    pwm_divider++;
    if(pwm_divider < PWM_DIV_FACTOR) return;
    pwm_divider = 0;
    
    // PWM计数器：0-99循环，产生10000Hz的PWM（周期100us）
    pwm_counter++;
    if(pwm_counter >= PWM_RESOLUTION) pwm_counter = 0;
    
    // 将0-100的亮度直接映射到0-100的PWM周期（1:1映射）
    uint16_t pwm_threshold = pwm_duty;
    
    // 判断当前PWM周期是高电平还是低电平
    uint8_t led_on = (pwm_counter < pwm_threshold) ? 1 : 0;
    
    // 先关闭所有LED，然后根据模式和PWM状态决定哪些LED点亮
    LED_All_Off();
    
    // 如果PWM处于低电平期，不点亮任何LED
    if(!led_on) return;
    
    // PWM处于高电平期，根据模式决定哪些LED需要点亮
    switch(current_mode)
    {
        case LED_MODE_FLOW:
            // 流水灯：当前LED和前一个LED同时亮（拖尾效果）
            switch(led_index)
            {
                case 0: LED3_H; LED5_H; break;  // LED3亮，LED5作为拖尾
                case 1: LED7_H; LED3_H; break;  // LED7亮，LED3作为拖尾
                case 2: LED4_H; LED7_H; break;  // LED4亮，LED7作为拖尾
                case 3: LED6_H; LED4_H; break;  // LED6亮，LED4作为拖尾
                case 4: LED5_H; LED6_H; break;  // LED5亮，LED6作为拖尾
            }
            break;
            
        case LED_MODE_BREATH:
            // 呼吸灯：所有LED同时亮（亮度由pwm_duty控制）
            LED_All_On();
            break;
            
        case LED_MODE_BLINK:
            // 闪烁灯：根据led_state决定是否点亮所有LED
            if(led_state)
            {
                LED_All_On();
            }
            break;
            
        case LED_MODE_SPECIFIC:
            // 特定LED：根据掩码和led_state决定
            if(led_state)
            {
                if(specific_led_mask & 0x01) LED3_H;
                if(specific_led_mask & 0x02) LED7_H;
                if(specific_led_mask & 0x04) LED4_H;
                if(specific_led_mask & 0x08) LED6_H;
                if(specific_led_mask & 0x10) LED5_H;
            }
            break;
            
        case LED_MODE_ROTATE:
            // 单个轮流：只亮当前索引的LED
            switch(led_index)
            {
                case 0: LED3_H; break;
                case 1: LED7_H; break;
                case 2: LED4_H; break;
                case 3: LED6_H; break;
                case 4: LED5_H; break;
            }
            break;
            
        default:
            break;
    }
}

void LED_Process(void)
{
    // ==================== 软件PWM更新（每次中断都执行，20KHz） ====================
    // PWM在内部分频到10KHz，才能实现10000Hz的PWM频率（完全无闪烁）
    LED_PWM_Update();
    
    // ==================== 时间基准分频（20KHz -> 1KHz = 1ms） ====================
    // TIMER1中断频率是20KHz (每50us一次)
    // 需要每20次中断才算1ms，用于LED闪烁计时
    time_divider++;
    if(time_divider < TIME_DIV_FACTOR)
    {
        // 未达到1ms，不处理LED状态切换
        return;
    }
    time_divider = 0;  // 重置分频器，已经过了1ms
    
    // 调试信息（已关闭）
    // static uint16_t debug_counter = 0;
    // debug_counter++;
    // if(debug_counter >= 1000) {
    //     printf("[LED_Process] Mode=%d, Brightness=%d, PWM_Duty=%d, Index=%d\r\n", 
    //            current_mode, current_brightness, pwm_duty, led_index);
    //     debug_counter = 0;
    // }
    
    // ==================== 模式逻辑处理（每1ms执行一次） ====================
    switch(current_mode)
    {
        case LED_MODE_OFF:
            LED_All_Off();
            break;
            
        case LED_MODE_FLOW:
            // 流水灯：按间隔切换LED
            pwm_duty = current_brightness;  // 使用设定的亮度
            if(led_counter++ >= current_interval)
            {
                led_counter = 0;
                LED_All_Off();  // 切换前先关闭所有LED
                led_index = (led_index + 1) % 5;
            }
            break;
            
        case LED_MODE_BREATH:
            // 呼吸灯：渐变亮度
            // 每个步长时间 = interval/200 (0-100-0需要200步)
            if(led_counter++ >= (current_interval / 200))
            {
                led_counter = 0;
                
                if(breath_direction == 0)  // 变亮：0→100
                {
                    breath_brightness += 2;
                    if(breath_brightness >= 100)
                    {
                        breath_brightness = 100;
                        breath_direction = 1;
                    }
                }
                else  // 变暗：100→0
                {
                    if(breath_brightness >= 2)
                        breath_brightness -= 2;
                    else
                        breath_brightness = 0;
                    
                    if(breath_brightness == 0)
                    {
                        breath_direction = 0;
                    }
                }
                
                // PWM占空比 = 用户设定亮度 × 呼吸亮度比例
                pwm_duty = (current_brightness * breath_brightness) / 100;
            }
            break;
            
        case LED_MODE_BLINK:
            // 闪烁灯：按间隔切换亮灭（interval = 单次持续时间）
            pwm_duty = current_brightness;  // PWM固定使用用户亮度
            if(led_counter++ >= current_interval)
            {
                led_counter = 0;
                led_state = !led_state;  // 切换亮灭状态
            }
            break;
            
        case LED_MODE_SPECIFIC:
            // 特定灯闪：按间隔切换亮灭（与Blink相同）
            pwm_duty = current_brightness;  // PWM固定使用用户亮度
            if(led_counter++ >= current_interval)
            {
                led_counter = 0;
                led_state = !led_state;  // 切换亮灭状态
            }
            break;
            
        case LED_MODE_ROTATE:
            // 单个轮流：一次点亮一个
            pwm_duty = current_brightness;  // 使用设定的亮度
            if(led_counter++ >= current_interval)
            {
                led_counter = 0;
                LED_All_Off();  // 切换前先关闭所有LED
                led_index = (led_index + 1) % 5;
            }
            break;
            
        case LED_MODE_ALARM:
            // 报警模式：1-2Hz快速闪烁，最亮
            pwm_duty = 100;  // 亮度固定为最亮
            // 使用500ms间隔 (2Hz频率)
            if(led_counter++ >= 500)
            {
                led_counter = 0;
                led_state = !led_state;  // 切换亮灭状态
            }
            break;
    }
}

// ==================== 外部中断功能实现（任务一） ====================

// 全局变量定义
volatile uint8_t button_count = 0;      // 按键计数（0-5循环）
volatile uint8_t button_pressed = 0;    // 按键按下标志

/**
 * @brief: 外部中断配置
 * @description: 配置PA0为外部中断输入，下降沿触发
 * @return: 无
 */
void EXTI_Config(void)
{
    // 1. 使能GPIOA和AFIO时钟
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_AF);
    
    // 2. 配置PA0为输入模式（上拉输入）
    gpio_init(BUTTON_PORT, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, BUTTON_PIN);
    
    // 3. 连接EXTI0线到PA0
    gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOA, GPIO_PIN_SOURCE_0);
    
    // 4. 配置EXTI0线
    exti_init(BUTTON_EXTI, EXTI_INTERRUPT, EXTI_TRIG_FALLING);  // 下降沿触发（按键按下）
    exti_interrupt_flag_clear(BUTTON_EXTI);
    
    // 5. 配置NVIC
    nvic_irq_enable(BUTTON_IRQn, 2, 0);  // 优先级2（低于Timer1的优先级1）
    
    printf("\r\n>>> EXTI Initialized <<<\r\n");
    printf("Button on PA0, falling edge trigger\r\n");
    printf("Press button to switch LED mode (0-5)\r\n");
}

/**
 * @brief: 获取按键计数
 * @return: 当前按键计数值
 */
uint8_t EXTI_Get_Button_Count(void)
{
    return button_count;
}

/**
 * @brief: 重置按键计数
 * @return: 无
 */
void EXTI_Reset_Button_Count(void)
{
    button_count = 0;
}

/**
 * @brief: EXTI0中断服务程序
 * @description: 按键按下时触发，更新计数和LED状态
 * @return: 无
 */
void EXTI0_IRQHandler(void)
{
    // 检查EXTI0中断标志
    if(RESET != exti_interrupt_flag_get(BUTTON_EXTI))
    {
        // 简单软件消抖（延时20ms）
        delay_ms(20);
        
        // 再次检查按键状态，确认是真实按下
        if(gpio_input_bit_get(BUTTON_PORT, BUTTON_PIN) == RESET)
        {
            // 按键计数递增
            button_count++;
            if(button_count >= BUTTON_COUNT_MAX)
            {
                button_count = 0;  // 循环计数
            }
            
            // 设置按键按下标志
            button_pressed = 1;
            
            // 直接在中断中切换LED模式（满足任务一要求）
            LED_Set_Mode((led_mode_t)button_count);
            
            // 打印调试信息
            printf("\r\n>>> Button Pressed <<<\r\n");
            printf("Button Count: %d\r\n", button_count);
            
            // 根据计数显示对应的LED模式
            switch(button_count)
            {
                case 0: 
                    printf("Mode: OFF\r\n"); 
                    break;
                case 1: 
                    printf("Mode: Flow Light\r\n");
                    break;
                case 2:
                    printf("Mode: Breath Light\r\n");
                    break;
                case 3:
                    printf("Mode: Blink All\r\n");
                    break;
                case 4:
                    printf("Mode: Specific LED\r\n");
                    break;
                case 5:
                    printf("Mode: Rotate Single\r\n");
                    break;
            }
            printf("========================================\r\n");
        }
        
        // 清除中断标志
        exti_interrupt_flag_clear(BUTTON_EXTI);
    }
}

// ==================== 报警和错误指示功能（任务四） ====================

/**
 * @brief: 设置报警模式
 * @description: 1-2Hz快速闪烁，最亮（满足任务要求）
 * @return: 无
 */
void LED_Set_Alarm_Mode(void)
{
    // 设置最亮亮度
    LED_Set_Brightness(ALARM_BRIGHTNESS);
    
    // 设置报警频率（500ms = 2Hz）
    LED_Set_Interval(ALARM_FREQ_MS);
    
    // 设置为闪烁模式
    LED_Set_Mode(LED_MODE_BLINK);
    
    printf("\r\n========== ALARM MODE ==========\r\n");
    printf("Frequency: %d ms (2Hz)\r\n", ALARM_FREQ_MS);
    printf("Brightness: %d%% (MAX)\r\n", ALARM_BRIGHTNESS);
    printf("All LEDs blinking!\r\n");
    printf("=================================\r\n");
}

/**
 * @brief: 报警复位
 * @description: 关闭所有LED，清除报警状态
 * @return: 无
 */
void LED_Alarm_Reset(void)
{
    // 关闭所有LED
    LED_Set_Mode(LED_MODE_OFF);
    
    printf("\r\n>>> ALARM RESET <<<\r\n");
    printf("All LEDs turned OFF\r\n");
    printf("Alarm cleared!\r\n");
    printf("========================================\r\n");
}

/**
 * @brief: 校验错误指示
 * @description: LED7快速闪烁3次，提示校验错误
 * @return: 无
 */
void LED_Error_Indicator(void)
{
    // 保存当前LED7状态
    uint8_t saved_mode = LED_Get_Current_Mode();
    
    // LED7快速闪烁3次（每次100ms亮，100ms暗）
    for(uint8_t i = 0; i < 3; i++)
    {
        LED7_H;
        delay_ms(100);
        LED7_L;
        delay_ms(100);
    }
    
    // 恢复之前的模式（如果需要）
    // LED_Set_Mode((led_mode_t)saved_mode);
}

