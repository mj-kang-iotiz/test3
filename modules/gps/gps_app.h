#ifndef GPS_APP_H
#define GPS_APP_H

#include "FreeRTOS.h"
#include "board_config.h"
#include "gps.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

typedef void (*gps_command_callback_t)(bool success, void *user_data);

typedef struct {
  char cmd[128];                  // 전송할 명령어
  uint32_t timeout_ms;            // 타임아웃 (ms)
  bool is_async;                  // true: 비동기, false: 동기

  SemaphoreHandle_t response_sem; // 응답 대기용 세마포어
  bool *result;                   // 응답 결과 (true: OK, false: ERROR/TIMEOUT)

  gps_command_callback_t callback; // 완료 콜백
  void *user_data;                 // 사용자 데이터
  bool async_result;               // 비동기 결과 저장용
} gps_cmd_request_t;

bool gps_send_command_sync(gps_id_t id, const char *cmd, uint32_t timeout_ms);
bool gps_send_command_async(gps_id_t id, const char *cmd, uint32_t timeout_ms,
                             gps_command_callback_t callback, void *user_data);

bool gps_send_raw_data(gps_id_t id, const uint8_t *data, size_t len);

typedef void (*gps_init_callback_t)(bool success, void *user_data);

bool gps_init_um982_base_async(gps_id_t id, gps_init_callback_t callback);
bool gps_init_um982_rover_async(gps_id_t id, gps_init_callback_t callback);

/**
 * @brief GPS 초기화 (board_config 기반)
 *
 * board_config.h의 설정을 읽어서 자동으로 GPS 인스턴스 생성
 */
void gps_init_all(void);

/**
 * @brief GPS 태스크 생성 (레거시 호환용)
 *
 * @param arg 사용하지 않음 (board_config로 자동 설정)
 */
void gps_task_create(void *arg);

/**
 * @brief GPS 핸들 가져오기 (레거시 호환성)
 *
 * @return GPS 핸들 포인터 (첫 번째 GPS)
 */
gps_t *gps_get_handle(void);

/**
 * @brief GPS 인스턴스 핸들 가져오기
 *
 * @param id GPS ID
 * @return GPS 핸들 포인터
 */
gps_t *gps_get_instance_handle(gps_id_t id);

/**
 * @brief GGA 평균 데이터 읽기 가능 여부
 *
 * @param id GPS ID
 * @return true: 읽기 가능, false: 읽기 불가능
 */
bool gps_gga_avg_can_read(gps_id_t id);

/**
 * @brief GGA 평균 데이터 가져오기
 *
 * @param id GPS ID
 * @param lat 위도 출력 (NULL 가능)
 * @param lon 경도 출력 (NULL 가능)
 * @param alt 고도 출력 (NULL 가능)
 * @return true: 성공, false: 실패
 */
bool gps_get_gga_avg(gps_id_t id, double *lat, double *lon, double *alt);
bool gps_factory_reset_async(gps_id_t id, gps_init_callback_t callback, void *user_data);

/**
 * @brief GPS heading length 설정 (비동기)
 *
 * user_params에서 읽은 baseline_len 값으로 heading length를 설정합니다.
 * 다음 명령어를 순차적으로 전송:
 *   1. config heading length [baseline_len] [slave_distance]
 *   2. CONFIG HEADING FIXLENGTH
 *
 * @param id GPS ID
 * @param baseline_len Baseline 길이 (cm)
 * @param slave_distance Master-slave 거리 (cm)
 * @param callback 완료 콜백 (NULL 가능)
 * @param user_data 콜백에 전달할 사용자 데이터
 * @return true: 명령 전송 시작 성공, false: 실패
 */
bool gps_config_heading_length_async(gps_id_t id, double baseline_len, double slave_distance,
                                     gps_command_callback_t callback, void *user_data);

/**
 * @brief GPS 위치 데이터 포맷팅
 *
 * 포맷: +GPS,lat,N/S,lon,E/W,msl_alt,ellipsoid_alt,heading,fix\r\n
 * 예시: +GPS,37.123456,N,127.123456,E,100.0,148.0,45.50,4\r\n
 *
 * @param id GPS ID
 * @param buffer 출력 버퍼
 * @param buf_size 버퍼 크기
 * @return true: 성공, false: 실패 (데이터 없음 또는 버퍼 부족)
 */
bool gps_format_position_data(gps_id_t id, char *buffer, size_t buf_size);

#endif
