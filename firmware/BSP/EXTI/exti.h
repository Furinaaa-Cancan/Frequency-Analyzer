#ifndef _EXTI_H_
#define _EXTI_H_

#include "main.h"


#define KEY_GPIO_PORT   GPIOB
#define KEY_GPIO_PIN    GPIO_PIN_2

void KEY_Init(void);
void KEY_EXTI_Init(void);

#endif
