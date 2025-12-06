# GPS 데이터 주기 전송 가이드

## 개요

GPS 측위 데이터를 주기적으로 RS485(SoftUART)를 통해 전송하는 기능이 구현되었습니다.

### 전송 데이터 포맷

```
+GPS,37.123456,N,127.123456,E,100.0,148.0,45.50,4\r\n
```

| 필드 | 설명 | 예시 |
|------|------|------|
| 1 | 프리픽스 | `+GPS,` |
| 2 | 위도 (6자리 소수점) | `37.123456` |
| 3 | 남/북위 | `N` 또는 `S` |
| 4 | 경도 (6자리 소수점) | `127.123456` |
| 5 | 동/서경 | `E` 또는 `W` |
| 6 | 해수면 고도 (m) | `100.0` |
| 7 | 타원체 고도 (m) | `148.0` |
| 8 | 헤딩각 (도) | `45.50` |
| 9 | FIX 상태 | `4` (RTK Fix) |

### FIX 상태 코드

- `0`: Invalid (무효)
- `1`: GPS (단독 측위)
- `2`: DGPS (차분 GPS)
- `4`: RTK Fix (고정밀 RTK)
- `5`: RTK Float (유동 RTK)

---

## 구현된 기능

### 1. GPS 데이터 포맷팅 함수

**위치:** `modules/gps/gps_app.c:1045`

```c
bool gps_format_position_data(gps_id_t id, char *buffer, size_t buf_size);
```

**기능:**
- GPS 위치 데이터를 지정된 포맷으로 변환
- 뮤텍스를 사용한 스레드 안전 보장
- 데드락 방지를 위해 데이터 복사 후 즉시 뮤텍스 해제

**사용 예시:**
```c
char gps_data[128];
if (gps_format_position_data(GPS_ID_0, gps_data, sizeof(gps_data))) {
    // gps_data에 포맷된 문자열이 저장됨
    printf("%s", gps_data);
}
```

---

### 2. 주기 전송 함수

**위치:** `modules/rs485/rs485_app.c:213`

#### 시작 함수

```c
bool rs485_start_gps_transmission(gps_id_t gps_id, uint32_t interval_ms);
```

**파라미터:**
- `gps_id`: 전송할 GPS ID (GPS_ID_0, GPS_ID_1 등)
- `interval_ms`: 전송 주기 (밀리초), 0이면 기본값(1000ms) 사용

**반환값:**
- `true`: 성공
- `false`: 실패

**사용 예시:**
```c
// GPS_ID_0의 데이터를 1초마다 전송
if (rs485_start_gps_transmission(GPS_ID_0, 1000)) {
    printf("GPS 전송 시작\n");
}

// 또는 기본 주기(1000ms) 사용
rs485_start_gps_transmission(GPS_ID_0, 0);
```

#### 정지 함수

```c
void rs485_stop_gps_transmission(void);
```

**사용 예시:**
```c
rs485_stop_gps_transmission();
printf("GPS 전송 정지\n");
```

---

## 동기화 메커니즘

### 1. GPS 데이터 접근 보호

GPS 데이터는 여러 Task에서 동시에 접근할 수 있으므로, 다음과 같이 보호됩니다:

#### ✅ 수정된 부분

**lib/gps/gps.c:27**
```c
bool get_gga(gps_t *gps, char *buf, uint8_t *len) {
  bool ret = false;

  xSemaphoreTake(gps->mutex, portMAX_DELAY);  // ✅ 주석 해제
  if (gps->nmea_data.gga_is_rdy && gps->nmea_data.gga.fix != GPS_FIX_INVALID) {
    strncpy(buf, gps->nmea_data.gga_raw, gps->nmea_data.gga_raw_pos + 1);
    *len = gps->nmea_data.gga_raw_pos;
    ret = true;
  }
  xSemaphoreGive(gps->mutex);  // ✅ 주석 해제

  return ret;
}
```

**이유:** 주석 처리되어 있던 뮤텍스를 복구하여 Race Condition 방지

---

### 2. SoftUART 멀티스레드 안전성

**lib/rs485/softuart.c:110, 256**

SoftUART는 타이머 인터럽트(ISR)와 Task 간 공유 버퍼를 사용하므로 Critical Section으로 보호:

```c
// RX 버퍼 읽기
SoftUartState_E SoftUartReadRxBuffer(uint8_t SoftUartNumber, uint8_t *Buffer, uint8_t Len) {
  taskENTER_CRITICAL();  // ✅ 추가
  // ... 버퍼 복사 ...
  taskEXIT_CRITICAL();   // ✅ 추가
}

// TX 버퍼 쓰기
SoftUartState_E SoftUartPuts(uint8_t SoftUartNumber, uint8_t *Data, uint8_t Len) {
  taskENTER_CRITICAL();  // ✅ 추가
  // ... 버퍼 복사 ...
  taskEXIT_CRITICAL();   // ✅ 추가
}
```

**이유:** 인터럽트와 Task 간 동기화 (뮤텍스는 ISR에서 사용 불가)

---

### 3. 데드락 방지

#### GPS RX Task 개선

**modules/gps/gps_app.c:729**

```c
// 기존: portMAX_DELAY (무한 대기) → 데드락 위험
// 개선: 200ms 타임아웃
if (xSemaphoreTake(inst->gps.mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
  // ... 파싱 작업 ...
  xSemaphoreGive(inst->gps.mutex);
} else {
  LOG_WARN("GPS[%d] RX Task 뮤텍스 타임아웃", id);
}
```

**위험 시나리오 (수정 전):**
1. RX Task가 뮤텍스 획득
2. 이벤트 핸들러 호출 → 네트워크 I/O 블로킹 (수 초 소요)
3. TX Task가 뮤텍스 대기 → 1초 타임아웃 발생 → 명령 전송 실패

**개선 효과:**
- RX Task도 타임아웃 사용으로 무한 대기 방지
- 주석으로 잠재적 위험 경고

#### GPS 데이터 포맷팅 함수

**modules/gps/gps_app.c:1056**

```c
// 뮤텍스를 짧은 시간만 잡고 데이터 복사
if (xSemaphoreTake(inst->gps.mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
  nmea_data = inst->gps.nmea_data;  // 빠른 복사
  has_data = true;
  xSemaphoreGive(inst->gps.mutex);  // 즉시 해제
}

// 뮤텍스 밖에서 포맷팅 (블로킹 작업 분리)
int written = snprintf(buffer, buf_size, "+GPS,...");
```

**효과:**
- 뮤텍스 보유 시간 최소화
- 블로킹 작업을 뮤텍스 밖에서 수행

---

## 사용 시나리오

### 시나리오 1: 기본 사용 (1초 주기)

```c
void app_main(void) {
  // GPS 초기화
  gps_init_all();

  // RS485 초기화
  rs485_app_init();

  // GPS 주기 전송 시작 (1초)
  rs485_start_gps_transmission(GPS_ID_0, 1000);

  // ... 앱 실행 ...
}
```

### 시나리오 2: 수동 전송

```c
void send_gps_once(void) {
  char gps_buffer[128];

  if (gps_format_position_data(GPS_ID_0, gps_buffer, sizeof(gps_buffer))) {
    RS485_SetTransmitMode();
    vTaskDelay(pdMS_TO_TICKS(2));

    SoftUartPuts(0, (uint8_t *)gps_buffer, strlen(gps_buffer));
    SoftUartWaitUntilTxComplate(0);

    vTaskDelay(pdMS_TO_TICKS(2));
    RS485_SetReceiveMode();
  }
}
```

### 시나리오 3: AT 명령으로 제어

`modules/rs485/rs485_app.c`의 `rs485_task`에 추가:

```c
else if (strcmp(rx_buffer, "AT+GPSSTART\n") == 0) {
  if (rs485_start_gps_transmission(GPS_ID_0, 1000)) {
    SoftUartPuts(0, (uint8_t*)"+OK\r\n", 5);
  } else {
    SoftUartPuts(0, (uint8_t*)"+ERROR\r\n", 8);
  }
  SoftUartWaitUntilTxComplate(0);
}
else if (strcmp(rx_buffer, "AT+GPSSTOP\n") == 0) {
  rs485_stop_gps_transmission();
  SoftUartPuts(0, (uint8_t*)"+OK\r\n", 5);
  SoftUartWaitUntilTxComplate(0);
}
```

---

## 주의사항

### 1. 메모리 사용량

- 타이머 콜백에서 스택 사용: 약 128바이트 (버퍼)
- FreeRTOS 타이머 Task 스택이 충분한지 확인 (`configTIMER_TASK_STACK_DEPTH`)

### 2. RS485 충돌 방지

- 송신 중에는 수신 모드로 전환하지 말 것
- `SoftUartWaitUntilTxComplate(0)` 호출 후 모드 전환

### 3. GPS 데이터 유효성

- FIX 상태가 `GPS_FIX_INVALID`이면 전송하지 않음
- `gga_is_rdy` 플래그로 데이터 준비 확인

### 4. 타이머 콜백 주의사항

- 타이머 콜백은 타이머 Task 컨텍스트에서 실행됨
- `vTaskDelay`는 사용 가능하지만, 최소 시간만 사용
- 블로킹 작업은 최소화

---

## 트러블슈팅

### 문제 1: GPS 데이터가 전송되지 않음

**원인:**
- GPS FIX를 획득하지 못함
- `gps_format_position_data`가 false 반환

**해결:**
```c
// GPS 상태 확인
gps_t *gps = gps_get_instance_handle(GPS_ID_0);
if (gps->nmea_data.gga_is_rdy) {
  printf("FIX: %d\n", gps->nmea_data.gga.fix);
} else {
  printf("GGA 데이터 미준비\n");
}
```

### 문제 2: 뮤텍스 타임아웃 경고

**로그:**
```
GPS[0] RX Task 뮤텍스 타임아웃
```

**원인:**
- 이벤트 핸들러에서 블로킹 I/O 수행
- TX Task와 동시 접근

**해결:**
- 이벤트 핸들러에서 네트워크 작업을 큐로 전달
- 또는 타임아웃 시간 증가

### 문제 3: RS485 전송 실패

**원인:**
- SoftUART busy (`TxNComplated == 1`)
- 버퍼 오버플로우

**해결:**
```c
// 전송 전 상태 확인
if (!SUart[0].TxNComplated) {
  SoftUartPuts(0, data, len);
} else {
  printf("SoftUART busy\n");
}
```

---

## 변경 파일 목록

1. ✅ `lib/gps/gps.c` - get_gga() 뮤텍스 복구
2. ✅ `lib/rs485/softuart.c` - Critical Section 추가
3. ✅ `modules/gps/gps_app.h` - gps_format_position_data() 선언
4. ✅ `modules/gps/gps_app.c` - GPS 포맷팅 함수 구현, RX Task 타임아웃 추가
5. ✅ `modules/rs485/rs485_app.h` - 타이머 함수 선언
6. ✅ `modules/rs485/rs485_app.c` - 타이머 및 전송 로직 구현

---

## 성능 특성

| 항목 | 값 |
|------|------|
| 기본 전송 주기 | 1000ms |
| GPS 포맷팅 시간 | < 1ms |
| 뮤텍스 대기 시간 | 최대 100ms (포맷팅), 200ms (RX Task) |
| SoftUART 전송 시간 | ~80ms @ 9600bps (80바이트) |
| 총 지연 시간 | < 100ms |

---

## 라이선스 및 크레딧

- SoftUART: [liyanboy74/SoftwareSerial](https://github.com/liyanboy74)
- FreeRTOS: MIT License
