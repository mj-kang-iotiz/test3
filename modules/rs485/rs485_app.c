#include "softuart.h"
#include "rs485_app.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "gps_app.h"
#include "gsm_app.h"
#include "lora_app.h"
#include "board_config.h"
#include <stdio.h>
#include <string.h>

char* INIT_Notify = "+READY\r\n";
char* GPS_Notify = "+GPS,\r\n";
char* F_Version_Response = "+V0.0.1\r\n";
char* AT_Response = "+OK\r\n";
char* ATZ_Response = "+RESET\r\n";
char* ATnF_Response = "+CONFIGINIT\r\n";
char* GPSMANUF_Response = "+Unicore\r\n"; 					//"+Ublox\r\n"
char* CONFIG_Response = "+CONFIG=Hello1234567890abcdefghijklmnopqrstuvwyzABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n"; 						//need make CONFIG Variable
char* SETBASELINE_Response = "+SETBASELINE=\r\n"; 	//need make setbaseline variable
char* CASTER_Response = "+CASTER=\r\n"; 						//need make caster variable 
char* ID_Response = "+ID=\r\n";											//need make ID variable
char* MOUNTPOINT_Response = "+MOUNTPOINT=\r\n";			//need make MOUNTPOINT variable
char* PASSWORD_Response = "+PASSWORD=\r\n";					//need make PASSWORD variable
char* START_Response = "+GUGUSTART\r\n";
char* STOP_Response = "+GUGUSTOP\r\n";

char* ERROR_Response = "+ERROR\r\n";   	//ETC
char* ERROR1_Response = "+E01\r\n";			//DO NOT KNOW ERROR
char* ERROR2_Response = "+E02\r\n";			//Parameter ERROR
char* ERROR3_Response = "+E03\r\n";			//NO ready device ERROR

void RS485_SetTransmitMode(void)
{
    HAL_GPIO_WritePin(RS485_DE_PORT, RS485_DE_PIN, GPIO_PIN_SET);  // DE=1, RE=1 (송신)
		HAL_GPIO_WritePin(RS485_RE_PORT, RS485_RE_PIN, GPIO_PIN_SET);  // DE=1, RE=1 (송신)
}

void RS485_SetReceiveMode(void)
{
    HAL_GPIO_WritePin(RS485_DE_PORT, RS485_DE_PIN, GPIO_PIN_RESET); // DE=0, RE=0 (수신)
		HAL_GPIO_WritePin(RS485_RE_PORT, RS485_RE_PIN, GPIO_PIN_RESET); // DE=0, RE=0 (수신)
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance==TIM1)
	{
		SoftUartHandler();
	}
}

int get_line(uint8_t SoftUartNumber, char *buffer, int maxlen)
{
    int index = 0;
    uint8_t ch;

    while (index < maxlen - 1)
    {
        while (SoftUartRxAlavailable(SoftUartNumber) == 0);  // 수신 대기
        SoftUartReadRxBuffer(SoftUartNumber, &ch, 1);

        buffer[index++] = ch;

        if (ch == '\r')  // 종료 문자
            break;
    }

    buffer[index] = '\0';  // 문자열 종료
    return index;
}


static void rs485_task(void *pvParameter)
{
	char ch;
	char rx_buffer[64];
	uint8_t init_f = 0;

  while(1)
  {
    if(init_f == 0){
			init_f = 1;
			RS485_SetTransmitMode();
			vTaskDelay(pdMS_TO_TICKS(10));
			SoftUartPuts(0, (uint8_t*)INIT_Notify, strlen(INIT_Notify));  //dummy clear
			SoftUartWaitUntilTxComplate(0);
			SoftUartPuts(0, (uint8_t*)INIT_Notify, strlen(INIT_Notify));
			SoftUartWaitUntilTxComplate(0);
			vTaskDelay(pdMS_TO_TICKS(10));
		}
		
		RS485_SetReceiveMode();
    int len = get_line(0, rx_buffer, sizeof(rx_buffer));  // 문자열 수신

		RS485_SetTransmitMode();
		HAL_Delay(1);  // RS485 활성 대기
		// 문자열 비교: 대소문자 구분, 정확히 "TEST\r\n"
		
    if (strcmp(rx_buffer, "AT\n") == 0)
    {
				SoftUartPuts(0, (uint8_t*)AT_Response, strlen(AT_Response));
				SoftUartWaitUntilTxComplate(0);
    }
		else if (strcmp(rx_buffer, "ATZ\n") == 0)
    {
				SoftUartPuts(0, (uint8_t*)ATZ_Response, strlen(ATZ_Response));
				SoftUartWaitUntilTxComplate(0);
    }
		else if (strcmp(rx_buffer, "AT&F\n") == 0)
    {
				SoftUartPuts(0, (uint8_t*)ATnF_Response, strlen(ATnF_Response));
				SoftUartWaitUntilTxComplate(0);
    }
		else if (strcmp(rx_buffer, "AT+VER?\n") == 0)
    {
				SoftUartPuts(0, (uint8_t*)F_Version_Response, strlen(F_Version_Response));
				SoftUartWaitUntilTxComplate(0);
    }
		else if (strcmp(rx_buffer, "AT+GPSMANUF?\n") == 0)
    {
				SoftUartPuts(0, (uint8_t*)GPSMANUF_Response, strlen(GPSMANUF_Response));
				SoftUartWaitUntilTxComplate(0);
    }
		else if (strcmp(rx_buffer, "AT+CONFIG?\n") == 0)
    {
				SoftUartPuts(0, (uint8_t*)CONFIG_Response, strlen(CONFIG_Response));
				SoftUartWaitUntilTxComplate(0);
    }
		else if (strcmp(rx_buffer, "AT+SETBASELINE:xxx\n") == 0)
    {
				SoftUartPuts(0, (uint8_t*)SETBASELINE_Response, strlen(SETBASELINE_Response));
				SoftUartWaitUntilTxComplate(0);
    }
		else if (strcmp(rx_buffer, "AT+CASTER:xx.xx.xx.xxxx\n") == 0)
    {
				SoftUartPuts(0, (uint8_t*)CASTER_Response, strlen(CASTER_Response));
				SoftUartWaitUntilTxComplate(0);
    }
		else if (strcmp(rx_buffer, "AT+ID=xxxxx\n") == 0)
    {
				SoftUartPuts(0, (uint8_t*)ID_Response, strlen(ID_Response));
				SoftUartWaitUntilTxComplate(0);
    }
		else if (strcmp(rx_buffer, "AT+MOUNTPOINT=xxxx\n") == 0)
    {
				SoftUartPuts(0, (uint8_t*)MOUNTPOINT_Response, strlen(MOUNTPOINT_Response));
				SoftUartWaitUntilTxComplate(0);
    }
		else if (strcmp(rx_buffer, "AT+PASSWORD=xxxxx\n") == 0)
    {
				SoftUartPuts(0, (uint8_t*)PASSWORD_Response, strlen(PASSWORD_Response));
				SoftUartWaitUntilTxComplate(0);
    }
		else if (strcmp(rx_buffer, "AT+GUGUSTART\n") == 0)
    {
				// 측정 시작: GPS, LTE, LoRa 모듈 초기화
				const board_config_t *config = board_get_config();

				// RS485 GPS 전송 중지 (이미 실행 중이면)
				rs485_stop_gps_transmission();


				// 이중 초기화 방지: 기존 리소스가 있으면 먼저 정리
				// 1. LoRa 종료 (이미 초기화되어 있으면)
				lora_instance_deinit();

				// 2. GSM(LTE) 종료 (이미 초기화되어 있으면)
				gsm_task_destroy();

				// 3. GPS 종료 (이미 초기화되어 있으면)
				gps_deinit_all();

				// 이제 안전하게 새로 초기화
				// 1. GPS 초기화
				gps_init_all();

				// 2. GSM(LTE) 초기화
				if (config->use_gsm) {
					gsm_task_create(NULL);
				}

				// 3. LoRa 초기화
				lora_instance_init();

				SoftUartPuts(0, (uint8_t*)START_Response, strlen(START_Response));
				SoftUartWaitUntilTxComplate(0);
    }
		else if (strcmp(rx_buffer, "AT+GUGUSTOP\n") == 0)
    {
				// 측정 중지: GPS, LTE, LoRa 모듈 종료

				// RS485 GPS 전송 중지
				rs485_stop_gps_transmission();

				// 1. LoRa 종료
				lora_instance_deinit();

				// 2. GSM(LTE) 종료
				gsm_task_destroy();

				// 3. GPS 종료
				gps_deinit_all();

				SoftUartPuts(0, (uint8_t*)STOP_Response, strlen(STOP_Response));
				SoftUartWaitUntilTxComplate(0);
    }
		else{
				SoftUartPuts(0, (uint8_t*)ERROR1_Response, strlen(ERROR1_Response));
				SoftUartWaitUntilTxComplate(0);
		}
		
		RS485_SetReceiveMode();

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void rs485_app_init(void)
{
    xTaskCreate(rs485_task, "RS485_Task", 512, NULL, tskIDLE_PRIORITY + 1, NULL);
}

// ============================================================
// GPS 데이터 주기 전송 기능
// ============================================================

static TimerHandle_t gps_tx_timer = NULL;
static gps_id_t current_gps_id = GPS_ID_0;

/**
 * @brief GPS 데이터 전송 타이머 콜백
 */
static void gps_tx_timer_callback(TimerHandle_t xTimer)
{
  char gps_buffer[128];

  // GPS 데이터 포맷팅 (뮤텍스는 함수 내부에서 처리)
  if (gps_format_position_data(current_gps_id, gps_buffer, sizeof(gps_buffer))) {
    // 송신 모드로 전환
    RS485_SetTransmitMode();
    vTaskDelay(pdMS_TO_TICKS(2));  // RS485 안정화 대기

    // SoftUART로 전송 (Critical Section으로 보호됨)
    if (SoftUartPuts(0, (uint8_t *)gps_buffer, strlen(gps_buffer)) == SoftUart_OK) {
      SoftUartWaitUntilTxComplate(0);
    }

    vTaskDelay(pdMS_TO_TICKS(2));
    // 수신 모드로 복귀
    RS485_SetReceiveMode();
  }
}

/**
 * @brief GPS 데이터 주기 전송 시작
 */
bool rs485_start_gps_transmission(gps_id_t gps_id, uint32_t interval_ms)
{
  // 기본 전송 주기 설정
  if (interval_ms == 0) {
    interval_ms = GPS_TX_INTERVAL_MS;
  }

  // GPS ID 저장
  current_gps_id = gps_id;

  // 기존 타이머가 있으면 삭제
  if (gps_tx_timer != NULL) {
    xTimerStop(gps_tx_timer, 0);
    xTimerDelete(gps_tx_timer, 0);
  }

  // 타이머 생성 (주기적 실행)
  gps_tx_timer = xTimerCreate(
      "GPS_TX_Timer",                   // 타이머 이름
      pdMS_TO_TICKS(interval_ms),       // 주기
      pdTRUE,                           // 자동 재시작 (주기적 실행)
      NULL,                             // 타이머 ID (사용 안 함)
      gps_tx_timer_callback             // 콜백 함수
  );

  if (gps_tx_timer == NULL) {
    return false;
  }

  // 타이머 시작
  if (xTimerStart(gps_tx_timer, 0) != pdPASS) {
    xTimerDelete(gps_tx_timer, 0);
    gps_tx_timer = NULL;
    return false;
  }

  return true;
}

/**
 * @brief GPS 데이터 주기 전송 정지
 */
void rs485_stop_gps_transmission(void)
{
  if (gps_tx_timer != NULL) {
    xTimerStop(gps_tx_timer, 0);
    xTimerDelete(gps_tx_timer, 0);
    gps_tx_timer = NULL;
  }
}
