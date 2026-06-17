/*
 * drv_spa07005.c
 *
 *  Created on: 2026年4月27日
 *      Author: vivian.huowy
 */


#include "drv_spa07005.h"
#include "bsp_i2c.h"
#include <string.h>
#include <stdbool.h>

static const float odr_hz_table[] = {
    0.1f, 0.13f, 0.2f, 0.25f, 0.33f, 0.5f,
    1.0f, 1.5f,  2.0f, 2.5f,  3.0f,  4.0f,
    5.0f, 6.0f,  7.5f, 10.0f, 15.0f, 20.0f,
    25.0f, 30.0f, 40.0f, 50.0f, 60.0f, 70.0f,
    100.0f, 120.0f, 150.0f, 200.0f, 250.0f,
    308.0f, 400.0f, 400.0f
};

static SPA07005_PTRate_t calc_pt_ratio(SPA07005_P_ODR_t prs_odr, SPA07005_P_ODR_t tmp_odr) {
    float prs_hz = odr_hz_table[(uint8_t)prs_odr & 0x1F];
    float tmp_hz = odr_hz_table[(uint8_t)tmp_odr & 0x1F];

    if (tmp_hz <= 0.0f) return SPA07005_PT_RATIO_1_50;

    float ratio = prs_hz / tmp_hz;


    if      (ratio <= 1.2f)  return SPA07005_PT_RATIO_1_1;
    else if (ratio <= 3.0f)  return SPA07005_PT_RATIO_1_2;
    else if (ratio <= 6.0f)  return SPA07005_PT_RATIO_1_5;
    else if (ratio <= 9.0f)  return SPA07005_PT_RATIO_1_8;
    else if (ratio <= 25.0f) return SPA07005_PT_RATIO_1_10;
    else                     return SPA07005_PT_RATIO_1_50;
}


static SPA07005_P_ODR_t calc_tmp_odr(SPA07005_P_ODR_t prs_odr, SPA07005_PTRate_t pt_ratio) {
    float prs_hz = odr_hz_table[(uint8_t)prs_odr & 0x1F];
    float tmp_hz = prs_hz / (float)pt_ratio;

    // 找最接近的ODR枚举
    float min_diff = 1e9f;
    uint8_t best = 0;
    for (uint8_t i = 0; i < 32; i++) {
        float diff = odr_hz_table[i] - tmp_hz;
        if (diff < 0) diff = -diff;
        if (diff < min_diff) {
            min_diff = diff;
            best = i;
        }
    }
    return (SPA07005_P_ODR_t)best;
}

HAL_StatusTypeDef SPA07005_Init(SPA07005_t *dev, uint8_t i2c_addr) {
    HAL_StatusTypeDef status;
    uint8_t chip_id = 0;

    dev->i2c_addr = i2c_addr;

    status = SPA07005_SoftReset(dev);
    if (status != HAL_OK) return status;

    status = SPA07005_ReadID(dev, &chip_id);
    if (status != HAL_OK) return status;

    status = SPA07005_SetMode(dev, SPA07005_MEAS_IDLE);
    if (status != HAL_OK) return status;

    status = SPA07005_ReadConfig(dev);
    if (status != HAL_OK) return status;

    status = SPA07005_SetMode(dev, SPA07005_MEAS_CONT_TP);
    if (status != HAL_OK) return status;

    dev->is_initialized = true;
    return HAL_OK;
}

HAL_StatusTypeDef SPA07005_SoftReset(SPA07005_t *dev) {
    HAL_StatusTypeDef status;

    status = I2C_WriteReg(dev->i2c_addr, SPA07005_REG_SOFT_RESET, (uint8_t)SPA07005_RESET_FULL);
    if (status != HAL_OK) return status;

    HAL_Delay(10);
    return HAL_OK;
}

HAL_StatusTypeDef SPA07005_ReadID(SPA07005_t *dev, uint8_t *chip_id) {
    return I2C_ReadReg(dev->i2c_addr, SPA07005_REG_CHIP_ID, chip_id);
}

HAL_StatusTypeDef SPA07005_SetMode(SPA07005_t *dev, SPA07005_MeasCtrl_t mode) {
    return I2C_WriteReg(dev->i2c_addr, SPA07005_REG_MEASUREMENT_CTRL, (uint8_t)mode);
}

HAL_StatusTypeDef SPA07005_ReadConfig(SPA07005_t *dev) {
    HAL_StatusTypeDef status;
    uint8_t reg;

    // 读压力ODR，bit[4:0]
    status = I2C_ReadReg(dev->i2c_addr, SPA07005_REG_ODR_CONFIG, &reg);
    if (status != HAL_OK) return status;
    dev->prs_odr = (SPA07005_P_ODR_t)(reg & 0x1F);

    // 读压力OSR，bit[3:0]
    status = I2C_ReadReg(dev->i2c_addr, SPA07005_REG_PRESSURE_MODE, &reg);
    if (status != HAL_OK) return status;
    dev->prs_osr = (SPA07005_PressureMode_t)(reg & 0x0F);

    // 读温度OSR，bit[2:0]
    status = I2C_ReadReg(dev->i2c_addr, SPA07005_REG_TEMPERATURE_MODE, &reg);
    if (status != HAL_OK) return status;
    dev->tmp_osr = (SPA07005_TempMode_t)(reg & 0x07);

    // 读PT ratio
    status = I2C_ReadReg(dev->i2c_addr, SPA07005_REG_PT_RATE_CONFIG, &reg);
    if (status != HAL_OK) return status;
    dev->pt_ratio = (SPA07005_PTRate_t)reg;

    // 换算温度ODR（只读）
    dev->tmp_odr = calc_tmp_odr(dev->prs_odr, dev->pt_ratio);

    return HAL_OK;
}

HAL_StatusTypeDef SPA07005_ReadPT(SPA07005_t *dev) {
    HAL_StatusTypeDef status;
    uint8_t raw[3];

    // 温度
    status = I2C_ReadRegs(dev->i2c_addr, SPA07005_REG_TEMP_DATA_XLSB, raw, 3);
    if (status != HAL_OK) return status;
    int32_t t_raw = ((int32_t)raw[2] << 16) | ((int32_t)raw[1] << 8) | (int32_t)raw[0];
    if (t_raw & 0x800000) t_raw |= (int32_t)0xFF000000;
    dev->raw_temperature = t_raw;
    dev->temperature     = (float)t_raw / 65536.0f;

    // 压力
    status = I2C_ReadRegs(dev->i2c_addr, SPA07005_REG_PRESS_DATA_XLSB, raw, 3);
    if (status != HAL_OK) return status;
    uint32_t p_raw = ((uint32_t)raw[2] << 16) | ((uint32_t)raw[1] << 8) | (uint32_t)raw[0];
    dev->raw_pressure = p_raw;
    dev->pressure     = (float)p_raw / 64.0f;

    return HAL_OK;
}

HAL_StatusTypeDef SPA07005_SetPressureConfig(SPA07005_t *dev, SPA07005_P_ODR_t odr, SPA07005_PressureMode_t osr) {
    HAL_StatusTypeDef status;
    uint8_t reg;


    status = I2C_ReadReg(dev->i2c_addr, SPA07005_REG_ODR_CONFIG, &reg);
    if (status != HAL_OK) return status;
    reg = (reg & ~0x1F) | ((uint8_t)odr & 0x1F);
    status = I2C_WriteReg(dev->i2c_addr, SPA07005_REG_ODR_CONFIG, reg);
    if (status != HAL_OK) return status;
    dev->prs_odr = odr;


    status = I2C_ReadReg(dev->i2c_addr, SPA07005_REG_PRESSURE_MODE, &reg);
    if (status != HAL_OK) return status;
    reg = (reg & ~0x0F) | ((uint8_t)osr & 0x0F);
    status = I2C_WriteReg(dev->i2c_addr, SPA07005_REG_PRESSURE_MODE, reg);
    if (status != HAL_OK) return status;
    dev->prs_osr = osr;


    dev->pt_ratio = calc_pt_ratio(dev->prs_odr, dev->tmp_odr);
    status = I2C_WriteReg(dev->i2c_addr, SPA07005_REG_PT_RATE_CONFIG, (uint8_t)dev->pt_ratio);
    if (status != HAL_OK) return status;
    dev->tmp_odr = calc_tmp_odr(dev->prs_odr, dev->pt_ratio);

    return HAL_OK;
}

HAL_StatusTypeDef SPA07005_SetTemperatureConfig(SPA07005_t *dev, SPA07005_P_ODR_t tmp_odr, SPA07005_TempMode_t osr) {
    HAL_StatusTypeDef status;
    uint8_t reg;


    status = I2C_ReadReg(dev->i2c_addr, SPA07005_REG_TEMPERATURE_MODE, &reg);
    if (status != HAL_OK) return status;
    reg = (reg & ~0x07) | ((uint8_t)osr & 0x07);
    status = I2C_WriteReg(dev->i2c_addr, SPA07005_REG_TEMPERATURE_MODE, reg);
    if (status != HAL_OK) return status;
    dev->tmp_osr = osr;


    dev->pt_ratio = calc_pt_ratio(dev->prs_odr, tmp_odr);
    status = I2C_WriteReg(dev->i2c_addr, SPA07005_REG_PT_RATE_CONFIG, (uint8_t)dev->pt_ratio);
    if (status != HAL_OK) return status;


    dev->tmp_odr = calc_tmp_odr(dev->prs_odr, dev->pt_ratio);

    return HAL_OK;
}
