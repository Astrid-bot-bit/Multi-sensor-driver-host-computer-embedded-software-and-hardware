/*
 * bsp_i2c.h
 *
 *  Created on: 2026年4月22日
 *      Author: vivian.huowy
 */

#ifndef INC_BSP_I2C_H_
#define INC_BSP_I2C_H_

#ifdef __cplusplus
extern "C"{
#endif

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#define I2C_ERR_ARG    HAL_ERROR

extern I2C_HandleTypeDef  hi2c1;

HAL_StatusTypeDef I2C_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t data);
HAL_StatusTypeDef I2C_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *p_data);
HAL_StatusTypeDef I2C_WriteRegs(uint8_t dev_addr, uint8_t reg_addr, const uint8_t *p_data, uint16_t len);
HAL_StatusTypeDef I2C_ReadRegs(uint8_t dev_addr, uint8_t reg_addr, uint8_t *p_data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* INC_BSP_I2C_H_ */
