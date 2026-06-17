/*
 * drv_spl07003.h
 *
 *  Created on: 2026年4月22日
 *      Author: vivian.huowy
 */

#ifndef INC_DRV_SPL07003_H_
#define INC_DRV_SPL07003_H_

#include <stdint.h>
#include "stm32f4xx_hal.h"

#define SPL07003_I2C_ADDR_LOW      0x76
#define SPL07003_I2C_ADDR_HIGH     0x77

#define SPL07003_REG_PSR_B2        0x00
#define SPL07003_REG_PSR_B1        0x01
#define SPL07003_REG_PSR_B0        0x02
#define SPL07003_REG_TMP_B2        0x03
#define SPL07003_REG_TMP_B1        0x04
#define SPL07003_REG_TMP_B0        0x05
#define SPL07003_REG_PRS_CFG       0x06
#define SPL07003_REG_TMP_CFG       0x07
#define SPL07003_REG_MEAS_CFG      0x08
#define SPL07003_REG_CFG_REG       0x09
#define SPL07003_REG_INT_STS       0x0A
#define SPL07003_REG_FIFO_STS      0x0B
#define SPL07003_REG_RESET         0x0C
#define SPL07003_REG_ID            0x0D
#define SPL07003_REG_COEF          0x10

#define SPL07003_PRODUCT_ID        0x11

#define SPL07003_MEAS_IDLE         0x00
#define SPL07003_MEAS_PRESSURE     0x01
#define SPL07003_MEAS_TEMP         0x02
#define SPL07003_MEAS_CONT_P       0x05
#define SPL07003_MEAS_CONT_T       0x06
#define SPL07003_MEAS_CONT_PT      0x07

typedef enum{
	SPL07_OSR_1   = 0x00,
	SPL07_OSR_2   = 0x01,
	SPL07_OSR_4   = 0x02,
	SPL07_OSR_8   = 0x03,
	SPL07_OSR_16  = 0x04,
	SPL07_OSR_32  = 0x05,
	SPL07_OSR_64  = 0x06,
	SPL07_OSR_128 = 0x07
}SPL07003_OSR_t;

typedef enum{
	SPL07_RATE_1   = 0x00,
	SPL07_RATE_2   = 0x01,
	SPL07_RATE_4   = 0x02,
	SPL07_RATE_8   = 0x03,
	SPL07_RATE_16  = 0x04,
	SPL07_RATE_32  = 0x05,
	SPL07_RATE_64  = 0x06,
	SPL07_RATE_128 = 0x07
}SPL07003_Rate_t;

typedef struct{
	int16_t c0, c1;
	int32_t c00, c10;
	int16_t c01, c11, c20, c21, c30, c31, c40;
}SPL07003_Calib_t;

typedef struct{
	uint8_t i2c_addr;
	SPL07003_OSR_t prs_osr;
	SPL07003_OSR_t tmp_osr;
	SPL07003_Rate_t prs_rate;
	SPL07003_Rate_t tmp_rate;
	SPL07003_Calib_t calib;
	float kP;
	float kT;
}SPL07003_t;

extern SPL07003_t spl07003;

HAL_StatusTypeDef    SPL07003_Init(SPL07003_t *dev, uint8_t i2c_addr);
HAL_StatusTypeDef    SPL07003_SoftReset(SPL07003_t *dev);
HAL_StatusTypeDef    SPL07003_ReadID(SPL07003_t *dev, uint8_t *chip_id);
HAL_StatusTypeDef    SPL07003_ReadCalibCoef(SPL07003_t *dev);
HAL_StatusTypeDef    SPL07003_ConfigPressure(SPL07003_t *dev);
HAL_StatusTypeDef    SPL07003_ConfigTemperature(SPL07003_t *dev);
HAL_StatusTypeDef    SPL07003_SetMode(SPL07003_t *dev, uint8_t mode);
HAL_StatusTypeDef    SPL07003_GetRawPressure(SPL07003_t *dev, int32_t *raw_prs);
HAL_StatusTypeDef    SPL07003_GetRawTemperature(SPL07003_t *dev, int32_t *raw_temp);
float                SPL07003_CalcCompPressure(SPL07003_t *dev, int32_t raw_prs, int32_t raw_temp);
float                SPL07003_CalcCompTemperature(SPL07003_t *dev, int32_t raw_temp);
HAL_StatusTypeDef    SPL07003_GetPressure(SPL07003_t *dev, float *pressure);
HAL_StatusTypeDef    SPL07003_GetTemperature(SPL07003_t *dev, float *temperature);
HAL_StatusTypeDef    SPL07003_SetPressureConfig(SPL07003_t *dev, SPL07003_Rate_t rate, SPL07003_OSR_t osr);
HAL_StatusTypeDef    SPL07003_SetTemperatureConfig(SPL07003_t *dev, SPL07003_Rate_t rate, SPL07003_OSR_t osr);

#endif /* INC_DRV_SPL07003_H_ */
