/*
 * bsp_i2c.c
 *
 *  Created on: 2026年4月22日
 *      Author: vivian.huowy
 */


#include "bsp_i2c.h"
#include <string.h>

HAL_StatusTypeDef I2C_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t data) {
	uint8_t buf[2] = {reg_addr, data};
	HAL_StatusTypeDef ret;

	ret = HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(dev_addr << 1), buf, 2, 100);

	if (ret == HAL_OK) return HAL_OK;

	return ret;
}


HAL_StatusTypeDef I2C_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *p_data) {
	HAL_StatusTypeDef ret;

	if (p_data == NULL)    return I2C_ERR_ARG;

	ret = HAL_I2C_Mem_Read(&hi2c1, (uint16_t)(dev_addr << 1), reg_addr, I2C_MEMADD_SIZE_8BIT, p_data, 1, 100);

	if (ret == HAL_OK)  return HAL_OK;

	return ret;
}

HAL_StatusTypeDef I2C_WriteRegs(uint8_t dev_addr, uint8_t reg_addr, const uint8_t *p_data, uint16_t len) {
	HAL_StatusTypeDef ret;

	if (p_data == NULL || len == 0)    return I2C_ERR_ARG;

	ret = HAL_I2C_Mem_Write(&hi2c1, (uint16_t)(dev_addr << 1), reg_addr, I2C_MEMADD_SIZE_8BIT, (uint8_t *)p_data, len, 100);

	return ret;
}


HAL_StatusTypeDef I2C_ReadRegs(uint8_t dev_addr, uint8_t reg_addr, uint8_t *p_data, uint16_t len) {
	HAL_StatusTypeDef ret;

	if (p_data == NULL || len == 0)    return I2C_ERR_ARG;

	ret = HAL_I2C_Mem_Read(&hi2c1, (uint16_t)(dev_addr << 1), reg_addr, I2C_MEMADD_SIZE_8BIT, p_data, len, 100);

	return ret;
}
