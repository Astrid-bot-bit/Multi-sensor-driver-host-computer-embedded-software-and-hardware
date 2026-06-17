/*
 * sensor_protocol.c
 *
 *  Created on: 2026年5月6日
 *      Author: vivian.huowy
 */


#include "sensor_protocol.h"
#include <string.h>
#include "sensor_manager.h"
#include "usbd_cdc_if.h"
#include "bsp_i2c.h"
#include "bsp_timer.h"

SensorStatus_t g_sensor_status;
volatile bool g_data_updated = false;
volatile bool g_cdc_connected = false;

static uint8_t s_ring_buf[SENSOR_RX_RING_BUF_SIZE];
static volatile uint16_t s_ring_write = 0U;
static volatile uint16_t s_ring_read = 0U;

static uint8_t s_rx_byte = 0U;
static FrameParse_t s_parser;

static uint8_t Proto_CalcXOR(const uint8_t *data, uint8_t len);
static bool RingBuf_Read(uint8_t *out_byte);
static void Parser_FeedByte(uint8_t byte);
static void Parser_HandleFrame(uint8_t cmd, const uint8_t *data, uint8_t len);

void Proto_Init(void) {
	memset(&g_sensor_status, 0, sizeof(g_sensor_status));
	g_sensor_status.product_type = PRODUCT_UNKNOWN;


	memset(s_ring_buf, 0, sizeof(s_ring_buf));
	s_ring_write = 0U;
	s_ring_read = 0U;

	memset(&s_parser, 0, sizeof(s_parser));
	s_parser.state = PARSE_STATE_HEADER1;
	g_data_updated = false;

	HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1U);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance != USART1)    return;

//	HAL_UART_Transmit(&huart1, &s_rx_byte, 1, 10);   //回显

	uint16_t next_write = (uint16_t)((s_ring_write + 1U) % SENSOR_RX_RING_BUF_SIZE);
	if (next_write != s_ring_read) {
		s_ring_buf[s_ring_write] = s_rx_byte;
		s_ring_write = next_write;
	}

	HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1U);
}

void Sensor_ProcessRxData(void) {
	uint8_t byte;
	uint8_t processed = 0;

	while ((processed < 64U) && RingBuf_Read(&byte)) {
		Parser_FeedByte(byte);
		processed++;
	}
}

void Sensor_GetProductType(void) {
	uint8_t data = 0x01U;
	Proto_SendFrame(CMD_GET_PRODUCT_TYPE, &data, 1U);
}

void Sensor_WriteRegister(uint8_t reg_addr, uint8_t value) {
	uint8_t data[2];
	data[0] = reg_addr;
	data[1] = value;

	Proto_SendFrame(CMD_WRITE_REGISTER, data, 2U);

}

void Sensor_ReadRegister(uint8_t reg_addr) {
	Proto_SendFrame(CMD_READ_REGISTER, &reg_addr, 1U);
}

void Sensor_StartContinuousRead(void) {

	g_sensor_status.continuous_mode = true;
}

void Sensor_StopRead(void) {

	g_sensor_status.continuous_mode = false;

}

void Sensor_ReadRawData(void) {
	uint8_t data = 0x1U;

	Proto_SendFrame(CMD_READ_RAW_DATA, &data, 1U);
}

void Sensor_SetOverSampling(uint8_t press_rate, uint8_t press_osr, uint8_t temp_rate, uint8_t temp_osr) {
	uint8_t data[4];
	data[0] = press_rate;
	data[1] = press_osr;
	data[2] = temp_rate;
	data[3] = temp_osr;

	Proto_SendFrame(CMD_SET_OVERSAMPLING, data, 4U);
}

float Sensor_GetPressure(void) {
	return g_sensor_status.measurement.pressure;
}

float Sensor_GetTemperature(void) {
	return g_sensor_status.measurement.temperature;
}

const char *Sensor_GetProductName(void) {
	switch (g_sensor_status.product_type) {
		case PRODUCT_SPL16_006: return "SPL16_006";
		case PRODUCT_SPL07_003: return "SPL07_003";
		case PRODUCT_SPL06_001: return "SPL06_001";
		case PRODUCT_SPL16_008: return "SPL16_008";
		case PRODUCT_SPL17_006: return "SPL17_006";
		case PRODUCT_SPL15_002: return "SPL15_002";
		case PRODUCT_SPL16_005: return "SPL16_005";
		case PRODUCT_SPA07_005: return "SPA07_005";
		case PRODUCT_SPD17_001: return "SPD17_001";
		default:                return "UNKNOWN";
	}
}

static uint8_t Proto_CalcXOR(const uint8_t *data, uint8_t len) {
	uint8_t xor_val = 0U;

	for (uint8_t i = 0U; i < len; i++) {
		xor_val ^= data[i];
	}

	return xor_val;
}

void Proto_SendFrame(uint8_t cmd, const uint8_t *data, uint8_t data_len) {
    uint8_t frame[SENSOR_FRAME_MAX_DATA_LEN + 6U];
    uint16_t idx = 0U;

    frame[idx++] = DEV_TO_PC_HEADER1;
    frame[idx++] = DEV_TO_PC_HEADER2;
    frame[idx++] = cmd;
    frame[idx++] = data_len;
    for (uint8_t i = 0U; i < data_len; i++) {
        frame[idx++] = data[i];
    }
    frame[idx++] = Proto_CalcXOR(data, data_len);
    frame[idx++] = DEV_TO_PC_TAIL;

//    HAL_UART_Transmit(&huart1, frame, idx, 100);

//    	uint8_t dbg[30];
//    	uint8_t dlen = (uint8_t)snprintf((char*)dbg, sizeof(dbg), "S = %d B = %02X\r\n", s_parser.state, byte);
//    	HAL_UART_Transmit(&huart1, dbg, dlen, 10);

    // ✅ 改为 USB CDC 发送
//    uint8_t frame[] = {0x55,0x55,0x01,0x01,0x01,0x01,0xAA};

//    HAL_UART_Transmit(&huart1, frame, sizeof(frame), 100);
//    HAL_UART_Transmit(&huart1, frame, idx, 100);
//    CDC_Transmit_FS(frame, idx);

    for (int retry = 0; retry < 3; retry++) {
    	if (CDC_Transmit_FS(frame, idx) == USBD_OK) {
    		break;
    	}
    	HAL_Delay(5);
    }
}

static bool RingBuf_Read(uint8_t *out_byte) {
	if (s_ring_write == s_ring_read) {
		return false;
	}

	*out_byte = s_ring_buf[s_ring_read];
	s_ring_read = (uint16_t)((s_ring_read + 1U) % SENSOR_RX_RING_BUF_SIZE);

	return true;
}

static void Parser_FeedByte(uint8_t byte) {

	switch (s_parser.state) {
		case PARSE_STATE_HEADER1:
			if (byte == PC_TO_DEV_HEADER1) {
				s_parser.state = PARSE_STATE_HEADER2;

			} break;
		case PARSE_STATE_HEADER2: {
			if (byte == PC_TO_DEV_HEADER2) {
				s_parser.state = PARSE_STATE_CMD;

			} else {
				s_parser.state = PARSE_STATE_HEADER1;
			}
		}break;
		case PARSE_STATE_CMD: {
			s_parser.cmd = byte;
			s_parser.state = PARSE_STATE_LEN;

		}break;
		case PARSE_STATE_LEN: {
			s_parser.data_len = byte;
			s_parser.data_idx = 0U;
			s_parser.checksum_calc = 0U;

			if (byte == 0U) {
				s_parser.state = PARSE_STATE_CHECKSUM;
			}
			else if (byte <= SENSOR_FRAME_MAX_DATA_LEN) {
				s_parser.state = PARSE_STATE_DATA;
			}
			else {
				s_parser.state = PARSE_STATE_HEADER1;
			}
		}break;
		case PARSE_STATE_DATA: {
			s_parser.data_buf[s_parser.data_idx] = byte;
			s_parser.data_idx++;
			s_parser.checksum_calc ^= byte;

			if (s_parser.data_idx >= s_parser.data_len) {
				s_parser.state = PARSE_STATE_CHECKSUM;
			}
		}break;
		case PARSE_STATE_CHECKSUM: {
			if (byte == s_parser.checksum_calc) {
				s_parser.state = PARSE_STATE_TAIL;
			}
			else {
				s_parser.state = PARSE_STATE_HEADER1;
			}
		}break;
		case PARSE_STATE_TAIL: {
			if (byte == PC_TO_DEV_TAIL) {
				Parser_HandleFrame(s_parser.cmd, s_parser.data_buf, s_parser.data_len);
				s_parser.state = PARSE_STATE_HEADER1;
			}
		}break;
		default: s_parser.state = PARSE_STATE_HEADER1; break;
	}

//	uint8_t dbg[30];
//	uint8_t dlen = (uint8_t)snprintf((char*)dbg, sizeof(dbg), "S = %d B = %02X\r\n", s_parser.state, byte);
//	HAL_UART_Transmit(&huart1, dbg, dlen, 10);

}

static void Parser_HandleFrame(uint8_t cmd, const uint8_t *data, uint8_t len) {

//	uint8_t dbg[] = "FRAME_OK\r\n";
//	HAL_UART_Transm	ait(&huart1, dbg, sizeof(dbg) - 1, 100);

	switch (cmd) {
	case CMD_GET_PRODUCT_TYPE: {
	    uint8_t type_byte = (uint8_t)Sensor_GetType();

//	    uint8_t dbg[20];
//	    uint8_t dlen = (uint8_t)snprintf((char*)dbg, sizeof(dbg), "TYPE = %d\r\n", type_byte);
//	    HAL_UART_Transmit(&huart1, dbg, dlen, 100);

	    Proto_SendFrame(CMD_GET_PRODUCT_TYPE, &type_byte, 1U);
	} break;
	case CMD_WRITE_REGISTER: {
		extern uint8_t g_addr;
		uint8_t reg_addr = data[0];
		uint8_t reg_val = data[1];

		uint8_t result = 0x00;

		if (I2C_WriteReg(g_addr, reg_addr, reg_val) == HAL_OK) {
			result = 0x01;
		}
		 Proto_SendFrame(CMD_WRITE_REGISTER, &result, 1U);
	}break;
	case CMD_READ_REGISTER: {
		g_sensor_status.last_reg_read_value = data[0];

		extern uint8_t g_addr;
		uint8_t reg_addr = data[0];
		uint8_t reg_val = 0;

		I2C_ReadReg(g_addr, reg_addr, &reg_val);
		Proto_SendFrame(CMD_READ_REGISTER, &reg_val, 1U);
	}break;
	case CMD_READ_PRESS_TEMP: {
		if (len >= 1U) {
			if (data[0] == 0x01) {
				g_sensor_status.continuous_mode = true;
			}
			else {
				g_sensor_status.continuous_mode = false;
			}
		}

	}break;
	case CMD_READ_RAW_DATA: {
		if (len <= (uint8_t)sizeof(g_sensor_status.raw_regs)) {
			memcpy(g_sensor_status.raw_regs, data, len);
		}
	}break;
	case CMD_SET_OVERSAMPLING: {
		if (len >= 4U) {
		        uint8_t result = 0x00U;
		        if (Sensor_SetConfig(data[0], data[1], data[2], data[3]) == HAL_OK) {
		            result = 0x01U;
		            BSP_Timer_SetRate(data[0]);
		        }
		        Proto_SendFrame(CMD_SET_OVERSAMPLING, &result, 1U);
		    }
	} break;
	default: break;
	}
}
// sensor_protocol.c 新增
void Proto_PushBytes(const uint8_t *buf, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        uint16_t next_write = (s_ring_write + 1U) % SENSOR_RX_RING_BUF_SIZE;
        if (next_write != s_ring_read) {
            s_ring_buf[s_ring_write] = buf[i];
            s_ring_write = next_write;
        }
    }
}
