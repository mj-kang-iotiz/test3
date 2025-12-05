#ifndef RS485_APP_H
#define RS485_APP_H

#include "stm32f4xx.h"

#define SOFT_UART_TX_PIN        GPIO_PIN_2
#define SOFT_UART_TX_PORT       GPIOD
#define SOFT_UART_RX_PIN        GPIO_PIN_12
#define SOFT_UART_RX_PORT       GPIOC

#define RS485_DE_PIN          GPIO_PIN_10
#define RS485_DE_PORT       	GPIOC
#define RS485_RE_PIN          GPIO_PIN_11
#define RS485_RE_PORT       	GPIOC

void rs485_app_init(void);


#endif
