#ifndef RS485_APP_H
#define RS485_APP_H

#include "stm32f4xx.h"
#include "board_config.h"
#include <stdbool.h>
#include <stdint.h>

#define SOFT_UART_TX_PIN        GPIO_PIN_2
#define SOFT_UART_TX_PORT       GPIOD
#define SOFT_UART_RX_PIN        GPIO_PIN_12
#define SOFT_UART_RX_PORT       GPIOC

#define RS485_DE_PIN          GPIO_PIN_10
#define RS485_DE_PORT       	GPIOC
#define RS485_RE_PIN          GPIO_PIN_11
#define RS485_RE_PORT       	GPIOC

// GPS 데이터 전송 주기 (ms)
#define GPS_TX_INTERVAL_MS    1000

void rs485_app_init(void);

/**
 * @brief GPS 데이터 주기 전송 시작
 *
 * @param gps_id 전송할 GPS ID
 * @param interval_ms 전송 주기 (ms), 0이면 기본값(1000ms) 사용
 * @return true: 성공, false: 실패
 */
bool rs485_start_gps_transmission(gps_id_t gps_id, uint32_t interval_ms);

/**
 * @brief GPS 데이터 주기 전송 정지
 */
void rs485_stop_gps_transmission(void);


#endif
