#include "softuart.h"
#include "rs485_app.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>
#include "flash_params.h"
#include "board_type.h"
#include "board_config.h"
#include "gps_app.h"

#ifndef TAG
#define TAG "RS485_APP"
#endif

#include "log.h"

char* INIT_Notify = "+READY\r";
char* GPS_Notify = "+GPS,\r";
char* F_Version_Response = "+V0.0.1\r";
char* AT_Response = "+OK\r";
char* ATZ_Response = "+RESET\r";
char* ATnF_Response = "+CONFIGINIT\r";
char* GPSMANUF_UM982_Response = "+Unicore\r";
char* GPSMANUF_F9P_Response = "+Ublox\r";
char* CONFIG_Response = "+CONFIG=Hello1234567890abcdefghijklmnopqrstuvwyzABCDEFGHIJKLMNOPQRSTUVWXYZ\r"; 						//need make CONFIG Variable
char* SETBASELINE_Response = "+SETBASELINE=\r"; 	//need make setbaseline variable
char* CASTER_Response = "+CASTER=\r"; 						//need make caster variable
char* ID_Response = "+ID=\r";											//need make ID variable
char* MOUNTPOINT_Response = "+MOUNTPOINT=\r";			//need make MOUNTPOINT variable
char* PASSWORD_Response = "+PASSWORD=\r";					//need make PASSWORD variable
char* START_Response = "+GUGUSTART\r";
char* STOP_Response = "+GUGUSTOP\r";

char* ERROR_Response = "+ERROR\r";   	//ETC
char* ERROR1_Response = "+E01\r";			//DO NOT KNOW ERROR
char* ERROR2_Response = "+E02\r";			//Parameter ERROR
char* ERROR3_Response = "+E03\r";			//NO ready device ERROR

static SemaphoreHandle_t rs485_tx_mutex;

volatile bool is_gugu_started = false;

void delay_170ns(void);

// 약 800ns 지연
void delay_170ns(void) {
    __asm volatile (
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
    );
}

void RS485_SetTransmitMode(void)
{
    HAL_GPIO_WritePin(RS485_DE_PORT, RS485_DE_PIN, GPIO_PIN_SET);  // DE=1, RE=1 (송신)
	HAL_GPIO_WritePin(RS485_RE_PORT, RS485_RE_PIN, GPIO_PIN_SET);  // DE=1, RE=1 (송신)
	delay_170ns();
  delay_170ns();
  delay_170ns();
  delay_170ns();
  delay_170ns();
}

void RS485_SetReceiveMode(void)
{
  delay_170ns();
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
        while (SoftUartRxAlavailable(SoftUartNumber) == 0)
        {
            vTaskDelay(pdMS_TO_TICKS(3));
        }
        SoftUartReadRxBuffer(SoftUartNumber, &ch, 1);

        buffer[index++] = ch;

        if (ch == '\r')  // 종료 문자
            break;
    }

    buffer[index] = '\0';  // 문자열 종료
    return index;
}

void RS485_Send(uint8_t *data, uint8_t len)
{
    xSemaphoreTake(rs485_tx_mutex, portMAX_DELAY);

    RS485_SetTransmitMode();
    SoftUartPuts(0, data, len);
    SoftUartWaitUntilTxComplate(0);
    RS485_SetReceiveMode();

    xSemaphoreGive(rs485_tx_mutex);
}


volatile bool base_init_finished = false;
static void base_config_complete(bool success, void *user_data) {
  gps_id_t id = (gps_id_t)(uintptr_t)user_data;
  LOG_INFO("GPS[%d] Fix mode init %s", id, success ? "succeeded" : "failed");

  base_init_finished = true;
}

static void send_gps_task(void* pvParameters)
{
  char buf[120];
  TickType_t xLastWakeTime = xTaskGetTickCount();

  while(1)
  {
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(2000));
    if(is_gugu_started)
    {
      gps_format_position_data(buf);
      RS485_Send((uint8_t*)buf, strlen(buf));
    }
  }
}

static void rs485_task(void *pvParameter)
{
	char ch;
	char rx_buffer[64];
  char buf[64];

  const board_config_t *config = board_get_config();

  // 시스템 부팅 후 3초 대기 (초기화 완료 대기)
  vTaskDelay(pdMS_TO_TICKS(3000));

  // READY 메시지 전송
  RS485_Send((uint8_t*)INIT_Notify, strlen(INIT_Notify));

  while(1)
  {
		RS485_SetReceiveMode();
    int len = get_line(0, rx_buffer, sizeof(rx_buffer));  // 문자열 수신

    if (strcmp(rx_buffer, "AT\r") == 0)
    {
        RS485_Send((uint8_t*)AT_Response, strlen(AT_Response));
    }
		else if (strcmp(rx_buffer, "ATZ\r") == 0)
    {
      user_params_t* current_params = flash_params_get_current();
      // if(flash_params_save(current_params) != HAL_OK)
      // {
      //     RS485_Send((uint8_t*)ERROR_Response, strlen(ERROR_Response));
      // }
      // else
      // {
        RS485_Send((uint8_t*)ATZ_Response, strlen(ATZ_Response));
        vTaskDelay(pdMS_TO_TICKS(10));
        NVIC_SystemReset();
      // }
    }
		else if (strcmp(rx_buffer, "AT&F\r") == 0)
    {
      user_params_t* current_params = flash_params_get_current();
      flash_params_erase();
			RS485_Send((uint8_t*)ATnF_Response, strlen(ATnF_Response));
      vTaskDelay(pdMS_TO_TICKS(10));
      NVIC_SystemReset();
    }
		else if (strcmp(rx_buffer, "AT+VER?\r") == 0)
    {
      sprintf(buf, "+%s\r", BOARD_VERSION);
			RS485_Send((uint8_t*)buf, strlen(buf));
    }
		else if (strcmp(rx_buffer, "AT+GPSMANUF?\r") == 0)
    {
      if(config->board == BOARD_TYPE_BASE_UM982 || config->board == BOARD_TYPE_ROVER_UM982)
      {
        RS485_Send((uint8_t*)GPSMANUF_UM982_Response, strlen(GPSMANUF_UM982_Response));
      }
      else if(config->board == BOARD_TYPE_BASE_F9P || config->board == BOARD_TYPE_ROVER_F9P)
      {
        RS485_Send((uint8_t*)GPSMANUF_F9P_Response, strlen(GPSMANUF_F9P_Response));
      }
    }
		else if (strcmp(rx_buffer, "AT+CONFIG?\r") == 0)
    {
				RS485_Send((uint8_t*)CONFIG_Response, strlen(CONFIG_Response));
    }
		else if (strncmp(rx_buffer, "AT+SETBASELINE:", 15) == 0)
    {
      char *endptr;
      double baseline_value = strtod(rx_buffer + 15, &endptr);

      flash_params_set_baseline_len(baseline_value);

      if (endptr != rx_buffer + 15 && (*endptr == '\r'))
      {
          RS485_Send((uint8_t*)AT_Response, strlen(AT_Response));
      }
      else
      {
          RS485_Send((uint8_t*)ERROR_Response, strlen(ERROR_Response));
      }
    }
		else if (strncmp(rx_buffer, "AT+CASTER:", 10) == 0)
    {
				RS485_Send((uint8_t*)CASTER_Response, strlen(CASTER_Response));
    }
		else if (strncmp(rx_buffer, "AT+ID=", 6) == 0)
    {
				RS485_Send((uint8_t*)ID_Response, strlen(ID_Response));
    }
		else if (strncmp(rx_buffer, "AT+MOUNTPOINT=", 14) == 0)
    {
				RS485_Send((uint8_t*)MOUNTPOINT_Response, strlen(MOUNTPOINT_Response));
    }
		else if (strncmp(rx_buffer, "AT+PASSWORD=", 12) == 0)
    {
				RS485_Send((uint8_t*)PASSWORD_Response, strlen(PASSWORD_Response));
    }
		else if (strcmp(rx_buffer, "AT+GUGUSTART\r") == 0)
    {
      RS485_Send((uint8_t*)START_Response, strlen(START_Response));
      is_gugu_started = true;
    }
		else if (strcmp(rx_buffer, "AT+GUGUSTOP\r") == 0)
    {
				RS485_Send((uint8_t*)STOP_Response, strlen(STOP_Response));
        is_gugu_started = false;
    }
    else if(strncmp(rx_buffer, "AT+SAVE\r", 8) == 0)
    {
      user_params_t* params = flash_params_get_current();
      flash_params_write(params);
      RS485_Send((uint8_t*)"+SAVE\r", strlen("+SAVE\r"));
      vTaskDelay(pdMS_TO_TICKS(10));
      NVIC_SystemReset();
    }
		else{
				RS485_Send((uint8_t*)ERROR1_Response, strlen(ERROR1_Response));
		}
  }
}

void rs485_app_init(void)
{
    rs485_tx_mutex = xSemaphoreCreateMutex();
    xTaskCreate(rs485_task, "RS485_Task", 512, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(send_gps_task, "send_gps", 512, NULL, tskIDLE_PRIORITY + 1, NULL);
}
