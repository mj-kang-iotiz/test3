#ifndef GSM_PORT_H
#define GSM_PORT_H

#include "gsm_app.h"

void gsm_dma_init(void);
void gsm_uart_init(void);
void gsm_port_comm_start(void);
void gsm_port_gpio_start(void);
uint32_t gsm_get_rx_pos(void);
void gsm_port_init(void);
void gsm_start(void);

/**
 * @brief EC25 모듈 하드웨어 리셋 (HAL ops 콜백)
 *
 * RST 핀을 이용한 하드웨어 리셋 수행
 * 리셋 후 모듈 부팅 완료까지 대기
 *
 * @return int 0: 성공
 */
int gsm_port_reset(void);

/**
 * @brief EC25 모듈 전원 종료
 *
 * PWR_KEY 핀을 이용한 정상 종료 수행
 * EC25 데이터시트에 따르면 PWRKEY를 최소 650ms LOW 유지 시 shutdown
 *
 * @return int 0: 성공
 */
int gsm_port_poweroff(void);

#endif
