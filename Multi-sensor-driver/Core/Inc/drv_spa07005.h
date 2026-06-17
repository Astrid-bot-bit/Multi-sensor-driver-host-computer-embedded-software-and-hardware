/*
 * drv_spa07005.h
 *
 *  Created on: 2026年4月27日
 *      Author: vivian.huowy
 */

#ifndef INC_DRV_SPA07005_H_
#define INC_DRV_SPA07005_H_

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

#define SPA07005_I2C_ADDR_HIGH          0x73
#define SPA07005_I2C_ADDR_LOW           0x72

#define SPA07005_REG_MANU_ID            0x00
#define SPA07005_REG_CHIP_ID            0x01
#define SPA07005_REG_REVC_ID            0x02
#define SPA07005_REG_TRIM_ID            0x03
#define SPA07005_REG_STATUS             0x04
#define SPA07005_REG_ODR_CONFIG         0x05
#define SPA07005_REG_PRESSURE_MODE      0x06
#define SPA07005_REG_TEMPERATURE_MODE   0x07
#define SPA07005_REG_PT_RATE_CONFIG     0x08
#define SPA07005_REG_INTERFACE_CONFIG   0x0B
#define SPA07005_REG_FIFO_CONFIG        0x0C
#define SPA07005_REG_FIFO_THS_CONFIG    0x0E
#define SPA07005_REG_INT_SOURCE         0x0F
#define SPA07005_REG_MEASUREMENT_CTRL   0x10
#define SPA07005_REG_SOFT_RESET         0x13
#define SPA07005_REG_SENSOR_CTRL        0x14
#define SPA07005_REG_TEMP_DATA_XLSB     0x1D
#define SPA07005_REG_TEMP_DATA_LSB      0x1E
#define SPA07005_REG_TEMP_DATA_MSB      0x1F
#define SPA07005_REG_PRESS_DATA_XLSB    0x20
#define SPA07005_REG_PRESS_DATA_LSB     0x21
#define SPA07005_REG_PRESS_DATA_MSB     0x22
#define SPA07005_REG_INT_STATUS         0x27
#define SPA07005_REG_FIFO_COUNT         0x28
#define SPA07005_REG_FIFO_DATA          0x29
#define SPA07005_REG_SENSOR_CONFIG      0x52
#define SPA07005_REG_INT_P_CONFIG       0x70

typedef enum {
    SPA07005_STATUS_INIT_COMPLETE  = 0x04,
    SPA07005_STATUS_CALIBRATED     = 0x08,
    SPA07005_STATUS_NUM_ERROR      = 0x10,
    SPA07005_STATUS_NOT_CONFIGURED = 0x40,
    SPA07005_STATUS_ODR_LIMITED    = 0x80
} SPA07005_Status_t;

typedef enum {
    SPA07005_ODR_0_1HZ  = 0x00,
    SPA07005_ODR_0_13HZ = 0x01,
    SPA07005_ODR_0_2HZ  = 0x02,
    SPA07005_ODR_0_25HZ = 0x03,
    SPA07005_ODR_0_33HZ = 0x04,
    SPA07005_ODR_0_5HZ  = 0x05,
    SPA07005_ODR_1HZ    = 0x06,
    SPA07005_ODR_1_5HZ  = 0x07,
    SPA07005_ODR_2HZ    = 0x08,
    SPA07005_ODR_2_5HZ  = 0x09,
    SPA07005_ODR_3HZ    = 0x0A,
    SPA07005_ODR_4HZ    = 0x0B,
    SPA07005_ODR_5HZ    = 0x0C,
    SPA07005_ODR_6HZ    = 0x0D,
    SPA07005_ODR_7_5HZ  = 0x0E,
    SPA07005_ODR_10HZ   = 0x0F,
    SPA07005_ODR_15HZ   = 0x10,
    SPA07005_ODR_20HZ   = 0x11,
    SPA07005_ODR_25HZ   = 0x12,
    SPA07005_ODR_30HZ   = 0x13,
    SPA07005_ODR_40HZ   = 0x14,
    SPA07005_ODR_50HZ   = 0x15,
    SPA07005_ODR_60HZ   = 0x16,
    SPA07005_ODR_70HZ   = 0x17,
    SPA07005_ODR_100HZ  = 0x18,
    SPA07005_ODR_120HZ  = 0x19,
    SPA07005_ODR_150HZ  = 0x1A,
    SPA07005_ODR_200HZ  = 0x1B,
    SPA07005_ODR_250HZ  = 0x1C,
    SPA07005_ODR_308HZ  = 0x1D,
    SPA07005_ODR_400HZ  = 0x1E,
    SPA07005_ODR_MAX    = 0x1F
} SPA07005_P_ODR_t;

typedef enum {
    SPA07005_PRESSURE_LLP = 0x00,
    SPA07005_PRESSURE_LP  = 0x03,
    SPA07005_PRESSURE_STD = 0x09,
    SPA07005_PRESSURE_HP  = 0x0C,
    SPA07005_PRESSURE_HHP = 0x0F
} SPA07005_PressureMode_t;

typedef enum {
    SPA07005_TEMP_LLP = 0x00,
    SPA07005_TEMP_LP  = 0x02,
    SPA07005_TEMP_STD = 0x03,
    SPA07005_TEMP_HP  = 0x05,
    SPA07005_TEMP_HHP = 0x07
} SPA07005_TempMode_t;

typedef enum {
    SPA07005_PT_RATIO_1_1  = 0x01,
    SPA07005_PT_RATIO_1_2  = 0x02,
    SPA07005_PT_RATIO_1_5  = 0x05,
    SPA07005_PT_RATIO_1_8  = 0x08,
    SPA07005_PT_RATIO_1_10 = 0x0A,
    SPA07005_PT_RATIO_1_50 = 0x32
} SPA07005_PTRate_t;

typedef enum {
    SPA07005_MEAS_IDLE      = 0x00,
    SPA07005_MEAS_SINGLE_P  = 0x01,
    SPA07005_MEAS_SINGLE_T  = 0x02,
    SPA07005_MEAS_SINGLE_PT = 0x03,
    SPA07005_MEAS_CONT_P    = 0x05,
    SPA07005_MEAS_CONT_T    = 0x06,
    SPA07005_MEAS_CONT_TP   = 0x07
} SPA07005_MeasCtrl_t;

typedef enum {
    SPA07005_RESET_FULL       = 0x03,
    SPA07005_RESET_CLEAR_IIR  = 0x04,
    SPA07005_RESET_FLUSH_FIFO = 0x80,
    SPA07005_RESET_CLEAR_ALL  = 0x87
} SPA07005_SoftReset_t;

typedef struct {
    uint8_t  i2c_addr;
    bool     is_initialized;

    SPA07005_P_ODR_t        prs_odr;
    SPA07005_P_ODR_t        tmp_odr;
    SPA07005_PTRate_t       pt_ratio;
    SPA07005_PressureMode_t prs_osr;
    SPA07005_TempMode_t     tmp_osr;
    int32_t  raw_temperature;
    uint32_t raw_pressure;
    float    temperature;
    float    pressure;
} SPA07005_t;


HAL_StatusTypeDef SPA07005_Init(SPA07005_t *dev, uint8_t i2c_addr);
HAL_StatusTypeDef SPA07005_SoftReset(SPA07005_t *dev);
HAL_StatusTypeDef SPA07005_ReadID(SPA07005_t *dev, uint8_t *chip_id);
HAL_StatusTypeDef SPA07005_SetMode(SPA07005_t *dev, SPA07005_MeasCtrl_t mode);
HAL_StatusTypeDef SPA07005_ReadConfig(SPA07005_t *dev);
HAL_StatusTypeDef SPA07005_ReadPT(SPA07005_t *dev);
HAL_StatusTypeDef SPA07005_SetPressureConfig(SPA07005_t *dev, SPA07005_P_ODR_t odr, SPA07005_PressureMode_t osr);
HAL_StatusTypeDef SPA07005_SetTemperatureConfig(SPA07005_t *dev, SPA07005_P_ODR_t tmp_odr, SPA07005_TempMode_t osr);

#endif /* INC_DRV_SPA07005_H_ */
