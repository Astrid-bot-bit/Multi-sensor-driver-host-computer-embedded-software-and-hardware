/*
 * drv_spa16001.h
 *
 *  Created on: 2026年4月29日
 *      Author: vivian.huowy
 */

#ifndef INC_DRV_SPA16001_H_
#define INC_DRV_SPA16001_H_

#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdint.h>

#define SPA16001_I2C_ADDR_LOW      0x76
#define SPA16001_I2C_ADDR_HIGH     0x77

#define SPA16001_REG_PSR_B2        0x00
#define SPA16001_REG_PSR_B1        0x01
#define SPA16001_REG_PSR_B0        0x02
#define SPA16001_REG_TMP_B2        0x03
#define SPA16001_REG_TMP_B1        0x04
#define SPA16001_REG_TMP_B0        0x05
#define SPA16001_REG_PRS_CFG       0x06
#define SPA16001_REG_TMP_CFG       0x07
#define SPA16001_REG_MEAS_CFG      0x08
#define SPA16001_REG_CFG_REG       0x09
#define SPA16001_REG_FIFO_STS      0x0B
#define SPA16001_REG_RESET         0x0C
#define SPA16001_REG_ID            0x0D
#define SPA16001_REG_COEF          0x10

#define SPA16001_PRODUCT_ID        0x11

#define SPA16001_MEAS_IDLE         0x00
#define SPA16001_MEAS_PRESSURE     0x01
#define SPA16001_MEAS_TEMP         0x02
#define SPA16001_MEAS_CONT_P       0x05
#define SPA16001_MEAS_CONT_T       0x06
#define SPA16001_MEAS_CONT_PT      0x07

#define SPA16001_FIFO_FULL_BIT     (1 << 1)
#define SPA16001_FIFO_EMPTY_BIT    (1 << 0)

#define SPA16001_FIFO_FLUSH        (1 << 7)
#define SPA16001_SOFT_RST          0x09

typedef enum{
	SPA16001_OSR_1   = 0x00,
	SPA16001_OSR_2   = 0x01,
	SPA16001_OSR_4   = 0x02,
	SPA16001_OSR_8   = 0x03,
	SPA16001_OSR_16  = 0x04,
	SPA16001_OSR_32  = 0x05,
	SPA16001_OSR_64  = 0x06,
	SPA16001_OSR_128 = 0x07
}SPA16001_OSR_t;

typedef enum{
	SPA16001_RATE_1   		= 0x00,
	SPA16001_RATE_2   		= 0x01,
	SPA16001_RATE_4   		= 0x02,
	SPA16001_RATE_8   		= 0x03,
	SPA16001_RATE_16  		= 0x04,
	SPA16001_RATE_32  		= 0x05,
	SPA16001_RATE_64  		= 0x06,
	SPA16001_RATE_128 		= 0x07,
	SPA16001_RATE_25DIV8 	= 0x09,
	SPA16001_RATE_25DIV4 	= 0x0A,
	SPA16001_RATE_25DIV2 	= 0x0B,
	SPA16001_RATE_25 		= 0x0C,
	SPA16001_RATE_50 		= 0x0D,
	SPA16001_RATE_100		= 0x0E,
	SPA16001_RATE_200 		= 0x0F
}SPA16001_Rate_t;

typedef struct{
	int16_t c0, c1;
	int32_t c00, c10, c20, c30;
	int16_t c01, c11, c21, c31, c40;
}SPA16001_Calib_t;

typedef struct{
	uint8_t i2c_addr;
	SPA16001_OSR_t prs_osr;
	SPA16001_OSR_t tmp_osr;
	SPA16001_Rate_t prs_rate;
	SPA16001_Rate_t tmp_rate;
	SPA16001_Calib_t calib;
	float kP;
	float kT;
}SPA16001_t;

typedef struct {
	int32_t raw;
	uint8_t is_pressure;
}SPA16001_FIFOEntry_t;

typedef enum {
	READ_MODE_POLL = 0,
	READ_MODE_FIFO = 1
}ReadMode_t;

extern SPA16001_t spa16001;

HAL_StatusTypeDef    SPA16001_Init(SPA16001_t *dev, uint8_t i2c_addr);
HAL_StatusTypeDef    SPA16001_SoftReset(SPA16001_t *dev);
HAL_StatusTypeDef    SPA16001_ReadID(SPA16001_t *dev, uint8_t *chip_id);
HAL_StatusTypeDef    SPA16001_ReadCalibCoef(SPA16001_t *dev);
HAL_StatusTypeDef    SPA16001_ConfigPressure(SPA16001_t *dev);
HAL_StatusTypeDef    SPA16001_ConfigTemperature(SPA16001_t *dev);
HAL_StatusTypeDef    SPA16001_SetMode(SPA16001_t *dev, uint8_t mode);
HAL_StatusTypeDef    SPA16001_GetRawPressure(SPA16001_t *dev, int32_t *raw_prs);
HAL_StatusTypeDef    SPA16001_GetRawTemperature(SPA16001_t *dev, int32_t *raw_temp);
float                SPA16001_CalcCompPressure(SPA16001_t *dev, int32_t raw_prs, int32_t raw_temp);
float                SPA16001_CalcCompTemperature(SPA16001_t *dev, int32_t raw_temp);
HAL_StatusTypeDef    SPA16001_GetPressure(SPA16001_t *dev, float *pressure);
HAL_StatusTypeDef    SPA16001_GetTemperature(SPA16001_t *dev, float *temperature);
HAL_StatusTypeDef    SPA16001_SetPressureConfig(SPA16001_t *dev, SPA16001_Rate_t rate, SPA16001_OSR_t osr);
HAL_StatusTypeDef    SPA16001_SetTemperatureConfig(SPA16001_t *dev, SPA16001_Rate_t rate, SPA16001_OSR_t osr);

//FIFO配置
//FIFO像“缓冲队列”，传感器往里边存数据，MCU不必每次测试完就去读数，可以节省CPU的资源。
HAL_StatusTypeDef    SPA16001_EnableFIFO(SPA16001_t *dev, uint8_t enable);
HAL_StatusTypeDef    SPA16001_GetFIFOStatus(SPA16001_t *dev, uint8_t *full, uint8_t *empty);
HAL_StatusTypeDef    SPA16001_ReadFIFO(SPA16001_t *dev, SPA16001_FIFOEntry_t *entries, uint8_t max_count, uint8_t *out_count);
HAL_StatusTypeDef    SPA16001_FlushFIFO(SPA16001_t *dev);
#endif /* INC_DRV_SPA16001_H_ */
