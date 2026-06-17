/*
 * drv_spl07003.c
 *
 *  Created on: 2026年4月22日
 *      Author: vivian.huowy
 */

#include "drv_spl07003.h"
#include "stm32f4xx_hal.h"
#include <math.h>
#include "bsp_i2c.h"

SPL07003_t spl07003;

static int32_t TwosComplement(uint32_t val, uint8_t bits);
static float GetScaleFactor(SPL07003_OSR_t osr);

static int32_t TwosComplement(uint32_t val, uint8_t bits) {
	if (val & (uint32_t)1 << (bits -1))
		return (int32_t)(val | (~0u << bits));

	return (int32_t)val;
}

static float GetScaleFactor(SPL07003_OSR_t osr) {
	switch(osr) {
	case SPL07_OSR_1:    return 524288.0f;
	case SPL07_OSR_2:    return 1572864.0f;
	case SPL07_OSR_4:    return 3670016.0f;
	case SPL07_OSR_8:    return 7864320.0f;
	case SPL07_OSR_16:    return 253952.0f;
	case SPL07_OSR_32:    return 516096.0f;
	case SPL07_OSR_64:    return 1040384.0f;
	case SPL07_OSR_128:    return 2088960.0f;
	default:     return 524288.0f;
	}
}

HAL_StatusTypeDef    SPL07003_Init(SPL07003_t *dev, uint8_t i2c_addr) {
	HAL_StatusTypeDef status;
	uint8_t chip_id;
	uint8_t meas_cfg;

	dev->i2c_addr = i2c_addr;

	HAL_Delay(50);

	status  = SPL07003_ReadID(dev, &chip_id);
	if (status != HAL_OK || (chip_id & 0x0F) != 0x01)    return HAL_ERROR;

	uint32_t timeout = HAL_GetTick();
	do {
		status = I2C_ReadReg(dev->i2c_addr, SPL07003_REG_MEAS_CFG, &meas_cfg);
		if (status != HAL_OK)  return status;
		if (HAL_GetTick() - timeout > 1000)   return HAL_TIMEOUT;

	} while ((meas_cfg & 0xC0) != 0xC0);

	status = SPL07003_ReadCalibCoef(dev);
	if (status != HAL_OK)  return status;

	status = SPL07003_SetMode(dev, SPL07003_MEAS_CONT_PT);
	if (status != HAL_OK)    return status;

	return HAL_OK;

}

HAL_StatusTypeDef    SPL07003_SoftReset(SPL07003_t *dev) {
	HAL_StatusTypeDef  status;
	uint8_t reset_cmd = 0x09;

	status = I2C_WriteReg(dev->i2c_addr, SPL07003_REG_RESET, reset_cmd);
	if (status != HAL_OK)    return status;

	HAL_Delay(50);
	return HAL_OK;
}


HAL_StatusTypeDef    SPL07003_ReadID(SPL07003_t *dev, uint8_t *chip_id) {
	return I2C_ReadReg(dev->i2c_addr, SPL07003_REG_ID, chip_id);
}

HAL_StatusTypeDef    SPL07003_ReadCalibCoef(SPL07003_t *dev) {
	HAL_StatusTypeDef  status;
	uint8_t coef[21];

	status = I2C_ReadRegs(dev->i2c_addr, SPL07003_REG_COEF, coef, 21);
	if (status != HAL_OK)  return status;

	dev->calib.c0 = TwosComplement(((uint16_t) coef[0] << 4) | (coef[1] >> 4 & 0x0F), 12);
	dev->calib.c1 = TwosComplement((((uint16_t) coef[1] & 0x0F) << 8) | coef[2], 12);
	dev->calib.c00 = TwosComplement(((uint32_t) coef[3] << 12) | ((uint32_t) coef[4] << 4) | ((coef[5] >> 4) & 0x0F), 20);
	dev->calib.c10 = TwosComplement((((uint32_t) coef[5] & 0x0F ) << 16) | ((uint32_t) coef[6] << 8) | coef[7], 20);
	dev->calib.c01 = TwosComplement(((uint16_t) coef[8] << 8) | coef[9], 16);
	dev->calib.c11 = TwosComplement(((uint16_t) coef[10] << 8) | coef[11], 16);
	dev->calib.c20 = TwosComplement(((uint16_t) coef[12] << 8) | coef[13], 16);
	dev->calib.c21 = TwosComplement(((uint16_t) coef[14] << 8) | coef[15], 16);
	dev->calib.c30 = TwosComplement(((uint16_t) coef[16] << 8) | coef[17], 16);
	dev->calib.c31 = TwosComplement(((uint16_t) coef[18] << 8) | ((coef[19] >> 4) & 0x0F), 12);
	dev->calib.c40 = TwosComplement(((uint16_t) (coef[19] & 0x0F) << 8) | coef[20], 12);

	return HAL_OK;
}

HAL_StatusTypeDef    SPL07003_ConfigPressure(SPL07003_t *dev) {
	HAL_StatusTypeDef  status;
	uint8_t cfg;

	status = I2C_ReadReg(dev->i2c_addr, SPL07003_REG_PRS_CFG, &cfg);
	if (status != HAL_OK)    return status;

	dev->prs_osr = (SPL07003_OSR_t)(cfg & 0x07);
	dev->prs_rate = (SPL07003_Rate_t)((cfg >> 4) & 0x07);
	dev->kP = GetScaleFactor(dev->prs_osr);

	return HAL_OK;
}


HAL_StatusTypeDef    SPL07003_ConfigTemperature(SPL07003_t *dev) {
	HAL_StatusTypeDef status;
	uint8_t cfg;

	status = I2C_ReadReg(dev->i2c_addr, SPL07003_REG_TMP_CFG, &cfg);
	if (status != HAL_OK)    return status;

	dev->tmp_osr = (SPL07003_OSR_t)(cfg & 0x07);
	dev->tmp_rate = (SPL07003_Rate_t)((cfg >> 4) & 0x07);
	dev->kT = GetScaleFactor(dev->tmp_osr);

	return HAL_OK;
}

HAL_StatusTypeDef    SPL07003_SetMode(SPL07003_t *dev, uint8_t mode) {
	HAL_StatusTypeDef  status;
	uint8_t meas_cfg;

	status = I2C_ReadReg(dev->i2c_addr, SPL07003_REG_MEAS_CFG, &meas_cfg);
	if (status != HAL_OK)  return status;

	meas_cfg = (meas_cfg & 0xF8) | (mode & 0x07);

	return I2C_WriteReg(dev->i2c_addr, SPL07003_REG_MEAS_CFG, meas_cfg);
}

HAL_StatusTypeDef    SPL07003_GetRawPressure(SPL07003_t *dev, int32_t *raw_prs) {
	HAL_StatusTypeDef  status;
	uint8_t data[3];

	status = I2C_ReadRegs(dev->i2c_addr, SPL07003_REG_PSR_B2, data, 3);
	if (status != HAL_OK)  return status;

	uint32_t raw = ((uint32_t)data[0] << 16 )| ((uint32_t)data[1] << 8) | (uint32_t)data[2];
	*raw_prs = TwosComplement(raw, 24);

	return HAL_OK;
}

HAL_StatusTypeDef    SPL07003_GetRawTemperature(SPL07003_t *dev, int32_t *raw_temp) {
	HAL_StatusTypeDef status;
	uint8_t data[3];

	status = I2C_ReadRegs(dev->i2c_addr, SPL07003_REG_TMP_B2, data, 3);
	if (status != HAL_OK)    return status;

	uint32_t raw = ((uint32_t)data[0] << 16 ) | ((uint32_t)data[1] << 8 ) | (uint32_t)data[2];
	*raw_temp = TwosComplement(raw, 24);

	return HAL_OK;
}

float                SPL07003_CalcCompPressure(SPL07003_t *dev, int32_t raw_prs, int32_t raw_temp) {
	float prs_sc = (float)raw_prs / dev->kP;
	float temp_sc = (float)raw_temp / dev->kT;

	return (float)dev->calib.c00 + prs_sc * ((float)dev->calib.c10 + prs_sc * ((float)dev->calib.c20 + prs_sc * ((float)dev->calib.c30 + prs_sc * (float)dev->calib.c40)))
			+ temp_sc * ((float)dev->calib.c01 + prs_sc * ((float)dev->calib.c11 + prs_sc * ((float)dev->calib.c21 + prs_sc * (float)dev->calib.c31)));
}

float                SPL07003_CalcCompTemperature(SPL07003_t *dev, int32_t raw_temp) {
	float temp_sc = (float)raw_temp /dev->kT;

	return (float)dev->calib.c0 * 0.5f + (float)dev->calib.c1 * temp_sc;

}

HAL_StatusTypeDef    SPL07003_GetPressure(SPL07003_t *dev, float *pressure) {
	HAL_StatusTypeDef status;
	int32_t raw_prs, raw_temp;

	status = SPL07003_GetRawPressure(dev, &raw_prs);
	if (status != HAL_OK)    return status;

	status = SPL07003_GetRawTemperature(dev, &raw_temp);
	if (status != HAL_OK)    return status;

	*pressure = SPL07003_CalcCompPressure(dev, raw_prs, raw_temp);

	return HAL_OK;

}

HAL_StatusTypeDef    SPL07003_GetTemperature(SPL07003_t *dev, float *temperature) {
	HAL_StatusTypeDef status;
	int32_t raw_temp;

	status = SPL07003_GetRawTemperature(dev, &raw_temp);
	if (status != HAL_OK)    return status;

	*temperature = SPL07003_CalcCompTemperature(dev, raw_temp);
	return HAL_OK;

}


HAL_StatusTypeDef    SPL07003_SetPressureConfig(SPL07003_t *dev, SPL07003_Rate_t rate, SPL07003_OSR_t osr) {
	HAL_StatusTypeDef status;
	uint8_t cfg = ((uint8_t)rate << 4) | ((uint8_t)osr & 0x07);

	dev->prs_osr = osr;
	dev->prs_rate = rate;
	dev->kP = GetScaleFactor(dev->prs_osr);

	status = I2C_WriteReg(dev->i2c_addr, SPL07003_REG_PRS_CFG, cfg);
	if (status != HAL_OK)    return status;

	if (osr > SPL07_OSR_8){
		uint8_t cfg_reg;

		status = I2C_ReadReg(dev->i2c_addr, SPL07003_REG_PRS_CFG, &cfg_reg);
		if (status != HAL_OK)    return status;

		cfg_reg |= 0x04;
		status = I2C_WriteReg(dev->i2c_addr, SPL07003_REG_PRS_CFG, cfg_reg);
		if(status != HAL_OK)    return status;

	}
	return HAL_OK;

}
HAL_StatusTypeDef    SPL07003_SetTemperatureConfig(SPL07003_t *dev, SPL07003_Rate_t rate, SPL07003_OSR_t osr) {
	HAL_StatusTypeDef status;
	uint8_t cfg = ((uint8_t)rate << 4) | ((uint8_t)osr & 0x07);

	dev->tmp_osr = osr;
	dev->tmp_rate = rate;
	dev->kT = GetScaleFactor(dev->tmp_osr);

	status = I2C_WriteReg(dev->i2c_addr, SPL07003_REG_TMP_CFG, cfg);
	if (status != HAL_OK)    return status;

	if (osr > SPL07_OSR_8){
		uint8_t cfg_reg;

		status = I2C_ReadReg(dev->i2c_addr, SPL07003_REG_TMP_CFG, &cfg_reg);
		if (status != HAL_OK)    return status;

		cfg_reg |= 0x08;

		status = I2C_WriteReg(dev->i2c_addr, SPL07003_REG_TMP_CFG, cfg_reg);
		if (status != HAL_OK)    return status;
	}
	return HAL_OK;

}
