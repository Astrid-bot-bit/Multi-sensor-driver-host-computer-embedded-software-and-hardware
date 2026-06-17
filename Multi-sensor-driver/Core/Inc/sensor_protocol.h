/*
 * sensor_protocol.h
 *
 *  Created on: 2026年5月6日
 *      Author: vivian.huowy
 */

#ifndef INC_SENSOR_PROTOCOL_H_
#define INC_SENSOR_PROTOCOL_H_

#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "sensor_manager.h"

#define PC_TO_DEV_HEADER1    (0xAAU)
#define PC_TO_DEV_HEADER2    (0xAAU)
#define PC_TO_DEV_TAIL       (0x55U)

#define DEV_TO_PC_HEADER1    0x55
#define DEV_TO_PC_HEADER2    0x55
#define DEV_TO_PC_TAIL       0xAA

#define CMD_GET_PRODUCT_TYPE    (0x01U)
#define CMD_SET_ATMO_PRESS      (0x02U)
#define CMD_SET_ALTITUDE        (0x03U)
#define CMD_SET_BOILING_POINT   (0x04U)
#define CMD_WRITE_REGISTER      (0x05U)
#define CMD_READ_REGISTER       (0x06U)
#define CMD_READ_PRESS_TEMP     (0x07U)
#define CMD_READ_RAW_DATA       (0x08U)
#define CMD_SET_COEXIST_MODE    (0x09U)
#define CMD_SET_MIC_FUNC        (0x0AU)
#define CMD_SET_OVERSAMPLING    (0x0BU)
#define CMD_SET_INT_THRESHOLD   (0x0CU)

/** CMD_READ_PRESS_TEMP 的子命令：读取模式 */
#define READ_MODE_STOP          (0x00U)
#define READ_MODE_CONTINUOUS    (0x01U)

#define SENSOR_RX_RING_BUF_SIZE   (256U)

#define SENSOR_FRAME_MAX_DATA_LEN (64U)


typedef enum {
	PARSE_STATE_HEADER1,
	PARSE_STATE_HEADER2,
	PARSE_STATE_CMD,
	PARSE_STATE_LEN,
	PARSE_STATE_DATA,
	PARSE_STATE_CHECKSUM,
	PARSE_STATE_TAIL
}FrameParseState_t;


typedef struct {
	float pressure;
	float temperature;
	uint32_t pressure_adc;
	uint8_t status_reg;
	uint32_t temperature_adc;
	bool data_valid;
}SensorMeasurement_t;

typedef struct {
	FrameParseState_t state;
	uint8_t cmd;
	uint8_t data_len;
	uint8_t data_buf[SENSOR_FRAME_MAX_DATA_LEN];
	uint8_t data_idx;
	uint8_t checksum_calc;
}FrameParse_t;

typedef struct {
	SensorProductType_t product_type;
	SensorMeasurement_t measurement;
	uint8_t raw_regs[SENSOR_FRAME_MAX_DATA_LEN];
	uint8_t last_reg_read_value;
	bool continuous_mode;
}SensorStatus_t;

extern UART_HandleTypeDef huart1;
extern SensorStatus_t g_sensor_status;
extern volatile bool g_data_updated;
extern volatile bool g_cdc_connected;

void Proto_Init(void);
void Sensor_GetProductType(void);
void Sensor_WriteRegister(uint8_t reg_addr, uint8_t value);
void Sensor_ReadRegister(uint8_t reg_addr);
void Sensor_StartContinuousRead(void);
void Sensor_StopRead(void);
void Sensor_ReadRawData(void);
void Sensor_SetOverSampling(uint8_t press_rate, uint8_t press_osr, uint8_t temp_rate, uint8_t temp_osr);
void Sensor_ProcessRxData(void);
float Sensor_GetPressure(void);
float Sensor_GetTemperature(void);
const char *Sensor_GetProductName(void);
void Proto_SendFrame(uint8_t cmd, const uint8_t *data, uint8_t data_len);
void Proto_PushBytes(const uint8_t *buf, uint16_t len);

#endif /* INC_SENSOR_PROTOCOL_H_ */
