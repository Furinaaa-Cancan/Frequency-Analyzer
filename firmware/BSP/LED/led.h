#ifndef _LED_H_
#define _LED_H_

#include "main.h"

#define led_clock_enable rcu_periph_clock_enable(RCU_GPIOB)

// 注意：根据你的硬件原理图，LED可能在不同的GPIO口
// 请根据实际硬件修改这些定义
#define   LED7_Init  gpio_init(GPIOB,GPIO_MODE_OUT_PP,GPIO_OSPEED_50MHZ,GPIO_PIN_12)
#define   LED3_Init  gpio_init(GPIOB,GPIO_MODE_OUT_PP,GPIO_OSPEED_50MHZ,GPIO_PIN_11)
#define   LED4_Init  gpio_init(GPIOB,GPIO_MODE_OUT_PP,GPIO_OSPEED_50MHZ,GPIO_PIN_13)
#define   LED6_Init  gpio_init(GPIOB,GPIO_MODE_OUT_PP,GPIO_OSPEED_50MHZ,GPIO_PIN_14)
#define   LED5_Init  gpio_init(GPIOB,GPIO_MODE_OUT_PP,GPIO_OSPEED_50MHZ,GPIO_PIN_15)

#define  LED7_L  gpio_bit_reset(GPIOB,GPIO_PIN_12)
#define  LED3_L  gpio_bit_reset(GPIOB,GPIO_PIN_11)
#define  LED4_L  gpio_bit_reset(GPIOB,GPIO_PIN_13)
#define  LED6_L  gpio_bit_reset(GPIOB,GPIO_PIN_14)
#define  LED5_L  gpio_bit_reset(GPIOB,GPIO_PIN_15)

#define  LED3_H  gpio_bit_set(GPIOB,GPIO_PIN_11)
#define  LED7_H  gpio_bit_set(GPIOB,GPIO_PIN_12)
#define  LED4_H  gpio_bit_set(GPIOB,GPIO_PIN_13)
#define  LED6_H  gpio_bit_set(GPIOB,GPIO_PIN_14)
#define  LED5_H  gpio_bit_set(GPIOB,GPIO_PIN_15)

// LED控制模式枚举（重新设计）
typedef enum {
    LED_MODE_OFF = 0,           // 0 - 关闭所有LED
    LED_MODE_FLOW,              // 1 - 流水灯（依次点亮）
    LED_MODE_BREATH,            // 2 - 呼吸灯（渐亮渐暗）
    LED_MODE_BLINK,             // 3 - 闪烁灯（全部闪烁）
    LED_MODE_SPECIFIC,          // 4 - 特定灯闪（指定LED闪烁）
    LED_MODE_ROTATE,            // 5 - 单个轮流（一次一个）
    LED_MODE_ALARM              // 6 - 报警模式（1-2Hz快速闪烁，最亮）
} led_mode_t;

// 报警模式参数
#define ALARM_FREQ_MS     500    // 报警闪烁频率：500ms = 2Hz
#define ALARM_BRIGHTNESS  100    // 报警亮度：最亮

// LED控制函数
void LED_Init(void);
void LED_All_Off(void);
void LED_All_On(void);
void LED_Set_Mode(led_mode_t mode);
void LED_Set_Interval(uint16_t interval_ms);
void LED_Set_Brightness(uint8_t brightness);
void LED_Set_Specific_LED(uint8_t led_mask);
void LED_Process(void);

// LED状态查询函数
uint8_t LED_Get_Current_Mode(void);
uint16_t LED_Get_Current_Interval(void);
uint8_t LED_Get_Current_Brightness(void);
uint8_t LED_Get_Specific_Mask(void);

// 报警和错误指示函数
void LED_Set_Alarm_Mode(void);          // 设置报警模式
void LED_Alarm_Reset(void);             // 报警复位（关闭LED）
void LED_Error_Indicator(void);         // 校验错误指示（LED7快速闪烁3次）

// ==================== 外部中断功能（任务一） ====================
// 按键定义（使用PA0作为按键输入）
#define BUTTON_PORT     GPIOA
#define BUTTON_PIN      GPIO_PIN_0
#define BUTTON_EXTI     EXTI_0
#define BUTTON_IRQn     EXTI0_IRQn
#define BUTTON_COUNT_MAX  6  // 按键计数上限（对应6种LED模式）

// 外部变量声明
extern volatile uint8_t button_count;       // 按键计数变量
extern volatile uint8_t button_pressed;     // 按键按下标志

// 外部中断函数
void EXTI_Config(void);                     // 外部中断初始化
uint8_t EXTI_Get_Button_Count(void);        // 获取按键计数
void EXTI_Reset_Button_Count(void);         // 重置按键计数

#endif

