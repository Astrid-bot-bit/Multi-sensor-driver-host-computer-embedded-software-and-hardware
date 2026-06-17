/*
 * sensor_manager.h
 *
 *  Created on: 2026年4月28日
 *      Author: vivian.huowy
 */

#ifndef INC_SENSOR_MANAGER_H_
#define INC_SENSOR_MANAGER_H_

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

#include "drv_spl07003.h"
#include "drv_spa07005.h"
#include "drv_spa16001.h"

typedef enum {
	PRODUCT_SPL16_006 = 0,
	PRODUCT_SPL07_003 = 1,
	PRODUCT_SPL06_001 = 2,
	PRODUCT_SPL16_008 = 3,
	PRODUCT_SPL17_006 = 4,
	PRODUCT_SPL15_002 = 5,
	PRODUCT_SPL16_005 = 6,
	PRODUCT_SPA07_005 = 7,
	PRODUCT_SPD17_001 = 8,
	PRODUCT_UNKNOWN   = 0xFF
}SensorProductType_t;


typedef struct {
	float pressure_pa;
	float temperature;

	uint32_t press_raw;
	int32_t temp_raw;

	uint8_t press_osr;
	uint8_t press_rate;

	uint8_t temp_osr;
	uint8_t temp_rate;

}SensorData_t;

SensorProductType_t Sensor_GetType(void);
SensorProductType_t    Sensor_AutoDetect(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef    Sensor_Init(void);
HAL_StatusTypeDef    Sensor_Read(SensorData_t *out);
const char    *Sensor_GetName(void);
HAL_StatusTypeDef Sensor_SetConfig(uint8_t press_rate, uint8_t press_osr, uint8_t temp_rate,  uint8_t temp_osr);

#endif /* INC_SENSOR_MANAGER_H_ */
