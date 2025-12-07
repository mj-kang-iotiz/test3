#ifndef NTRIP_APP_H
#define NTRIP_APP_H

#include "gsm.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief NTRIP TCP 수신 태스크 생성
 *
 * @param gsm GSM 핸들
 */
void ntrip_task_create(gsm_t *gsm);

/**
 * @brief NTRIP 리소스 정리
 *
 * GGA 송신 큐 등 NTRIP 관련 리소스 해제
 */
void ntrip_cleanup(void);

int ntrip_send_gga_data(const char *data, uint8_t len);
bool ntrip_gga_send_queue_initialized(void);

#endif // NTRIP_TASK_H