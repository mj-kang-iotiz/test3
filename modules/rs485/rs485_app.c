#include "softuart.h"
#include "rs485_app.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>

// 전역 뮤텍스 (동시 전송 방지)
static SemaphoreHandle_t rs485_tx_mutex = NULL;

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

// RS485 전송 래퍼 함수 (뮤텍스로 동시 전송 방지)
void RS485_Send(uint8_t *data, uint8_t len)
{
	if (rs485_tx_mutex != NULL)
	{
		xSemaphoreTake(rs485_tx_mutex, portMAX_DELAY);

		RS485_SetTransmitMode();
		// HAL_Delay(1) 제거 - GPIO는 즉시 전환되고 인터럽트까지 20us 여유 있음

		SoftUartPuts(0, data, len);
		SoftUartWaitUntilTxComplate(0);  // 이제 vTaskDelay(1)로 효율적으로 대기

		xSemaphoreGive(rs485_tx_mutex);
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
		vTaskDelay(pdMS_TO_TICKS(10));
		RS485_Send((uint8_t*)INIT_Notify, strlen(INIT_Notify));
		vTaskDelay(pdMS_TO_TICKS(10));
	}

	RS485_SetReceiveMode();
    int len = get_line(0, rx_buffer, sizeof(rx_buffer));  // 문자열 수신

	// 응답 전송 (RS485_Send가 자동으로 송신 모드 전환)
    if (strcmp(rx_buffer, "AT\n") == 0)
    {
		RS485_Send((uint8_t*)AT_Response, strlen(AT_Response));
    }
	else if (strcmp(rx_buffer, "ATZ\n") == 0)
    {
		RS485_Send((uint8_t*)ATZ_Response, strlen(ATZ_Response));
    }
	else if (strcmp(rx_buffer, "AT&F\n") == 0)
    {
		RS485_Send((uint8_t*)ATnF_Response, strlen(ATnF_Response));
    }
	else if (strcmp(rx_buffer, "AT+VER?\n") == 0)
    {
		RS485_Send((uint8_t*)F_Version_Response, strlen(F_Version_Response));
    }
	else if (strcmp(rx_buffer, "AT+GPSMANUF?\n") == 0)
    {
		RS485_Send((uint8_t*)GPSMANUF_Response, strlen(GPSMANUF_Response));
    }
	else if (strcmp(rx_buffer, "AT+CONFIG?\n") == 0)
    {
		RS485_Send((uint8_t*)CONFIG_Response, strlen(CONFIG_Response));
    }
	else if (strcmp(rx_buffer, "AT+SETBASELINE:xxx\n") == 0)
    {
		RS485_Send((uint8_t*)SETBASELINE_Response, strlen(SETBASELINE_Response));
    }
	else if (strcmp(rx_buffer, "AT+CASTER:xx.xx.xx.xxxx\n") == 0)
    {
		RS485_Send((uint8_t*)CASTER_Response, strlen(CASTER_Response));
    }
	else if (strcmp(rx_buffer, "AT+ID=xxxxx\n") == 0)
    {
		RS485_Send((uint8_t*)ID_Response, strlen(ID_Response));
    }
	else if (strcmp(rx_buffer, "AT+MOUNTPOINT=xxxx\n") == 0)
    {
		RS485_Send((uint8_t*)MOUNTPOINT_Response, strlen(MOUNTPOINT_Response));
    }
	else if (strcmp(rx_buffer, "AT+PASSWORD=xxxxx\n") == 0)
    {
		RS485_Send((uint8_t*)PASSWORD_Response, strlen(PASSWORD_Response));
    }
	else if (strcmp(rx_buffer, "AT+GUGUSTART\n") == 0)
    {
		RS485_Send((uint8_t*)START_Response, strlen(START_Response));
    }
	else if (strcmp(rx_buffer, "AT+GUGUSTOP\n") == 0)
    {
		RS485_Send((uint8_t*)STOP_Response, strlen(STOP_Response));
    }
	else{
		RS485_Send((uint8_t*)ERROR1_Response, strlen(ERROR1_Response));
	}

	RS485_SetReceiveMode();

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void rs485_app_init(void)
{
    // 뮤텍스 생성
    rs485_tx_mutex = xSemaphoreCreateMutex();

    xTaskCreate(rs485_task, "RS485_Task", 512, NULL, tskIDLE_PRIORITY + 1, NULL);
}
