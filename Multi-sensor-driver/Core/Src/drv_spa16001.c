/*
 * drv_spa16001.c
 *
 *  Created on: 2026年4月29日
 *      Author: vivian.huowy
 */


#include "drv_spa16001.h"
#include "stm32f4xx_hal.h"
#include <math.h>
#include "bsp_i2c.h"
#include <stdio.h>

SPA16001_t spa16001;

static int32_t TwosComplement(uint32_t val, uint8_t bits);
static float GetScaleFactor(SPA16001_OSR_t osr);

static int32_t TwosComplement(uint32_t val, uint8_t bits) {
	if (val & (uint32_t)1 << (bits -1))
		return (int32_t)(val | (~0u << bits));

	return (int32_t)val;
}

static float GetScaleFactor(SPA16001_OSR_t osr) {
	switch(osr) {
	case SPA16001_OSR_1:    return 524288.0f;
	case SPA16001_OSR_2:    return 1572864.0f;
	case SPA16001_OSR_4:    return 3670016.0f;
	case SPA16001_OSR_8:    return 7864320.0f;
	case SPA16001_OSR_16:    return 253952.0f;
	case SPA16001_OSR_32:    return 516096.0f;
	case SPA16001_OSR_64:    return 1040384.0f;
	case SPA16001_OSR_128:    return 2088960.0f;
	default:     return 524288.0f;
	}
}

HAL_StatusTypeDef    SPA16001_Init(SPA16001_t *dev, uint8_t i2c_addr) {
	HAL_StatusTypeDef status;
	uint8_t meas_cfg;

	dev->i2c_addr = i2c_addr;

	HAL_Delay(50);

	status = HAL_I2C_IsDeviceReady(&hi2c1, (dev->i2c_addr << 1), 3, 100);
//	printf("IsDeviceReady: %d\r\n", status);  // 0=OK, 1=ERROR, 3=BUSY
	if (status != HAL_OK) return status;

    status = SPA16001_SoftReset(dev);
//    printf("SoftReset: %d\r\n", status);
    if (status != HAL_OK) return status;
    HAL_Delay(50);

	uint8_t chip_id = 0;
	status = SPA16001_ReadID(dev, &chip_id);
//	printf("ReadID: %d, ID=0x%02X (expect 0x11)\r\n", status, chip_id);
	if (status != HAL_OK) return status;
//	status  = SPA16001_ReadID(dev, &chip_id);
//	if (status != HAL_OK || (chip_id & 0x0F) != 0x01)    return HAL_ERROR;

	uint32_t timeout = HAL_GetTick();
	do {
		status = I2C_ReadReg(dev->i2c_addr, SPA16001_REG_MEAS_CFG, &meas_cfg);
		if (status != HAL_OK)  return status;
		if (HAL_GetTick() - timeout > 1000)   return HAL_TIMEOUT;

	} while ((meas_cfg & 0xC0) != 0xC0);

	status = SPA16001_ReadCalibCoef(dev);
	if (status != HAL_OK)  return status;
	status = SPA16001_SetPressureConfig(dev, SPA16001_RATE_25, SPA16001_OSR_8);
	if (status != HAL_OK) return status;

	status = SPA16001_SetTemperatureConfig(dev, SPA16001_RATE_25, SPA16001_OSR_1);
	if (status != HAL_OK) return status;

	status = SPA16001_ConfigPressure(dev);
	if (status != HAL_OK) return status;

	status = SPA16001_ConfigTemperature(dev);
	if (status != HAL_OK) return status;

	status = SPA16001_SetMode(dev, SPA16001_MEAS_CONT_PT);
	if (status != HAL_OK)    return status;


	return HAL_OK;

}

HAL_StatusTypeDef    SPA16001_SoftReset(SPA16001_t *dev) {
	HAL_StatusTypeDef  status;
	uint8_t reset_cmd = 0x09;

	status = I2C_WriteReg(dev->i2c_addr, SPA16001_REG_RESET, reset_cmd);
	if (status != HAL_OK)    return status;

	HAL_Delay(50);
	return HAL_OK;
}


HAL_StatusTypeDef    SPA16001_ReadID(SPA16001_t *dev, uint8_t *chip_id) {
	return I2C_ReadReg(dev->i2c_addr, SPA16001_REG_ID, chip_id);
}

HAL_StatusTypeDef    SPA16001_ReadCalibCoef(SPA16001_t *dev) {
	HAL_StatusTypeDef  status;
	uint8_t coef[23];

	status = I2C_ReadRegs(dev->i2c_addr, SPA16001_REG_COEF, coef, 23);
	if (status != HAL_OK)  return status;

	dev->calib.c0 = TwosComplement(((uint16_t) coef[0] << 4) | (coef[1] >> 4 & 0x0F), 12);
	dev->calib.c1 = TwosComplement((((uint16_t) coef[1] & 0x0F) << 8) | coef[2], 12);
	dev->calib.c00 = TwosComplement(((uint32_t) coef[3] << 12) | ((uint32_t) coef[4] << 4) | ((coef[5] >> 4) & 0x0F), 20);
	dev->calib.c10 = TwosComplement((((uint32_t) coef[5] & 0x0F ) << 16) | ((uint32_t) coef[6] << 8) | coef[7], 20);
	dev->calib.c01 = TwosComplement(((uint16_t) coef[8] << 8) | coef[9], 16);
	dev->calib.c11 = TwosComplement(((uint16_t) coef[10] << 8) | coef[11], 16);

	dev->calib.c20 = TwosComplement(((uint32_t)coef[12] << 12) | ((uint32_t)coef[13] << 4) | (coef[14] >> 4), 20);


	dev->calib.c30 = TwosComplement((((uint32_t)coef[14] & 0x0F) << 16) | ((uint32_t)coef[15] << 8) | coef[16], 20);


	dev->calib.c21 = TwosComplement(((uint16_t)coef[17] << 8) | coef[18], 16);


	dev->calib.c40 = TwosComplement(((uint16_t)coef[19] << 8) | coef[20], 16);


	dev->calib.c31 = TwosComplement(((uint16_t)coef[21] << 6) | (coef[22] >> 2), 14);

	return HAL_OK;
}

HAL_StatusTypeDef    SPA16001_ConfigPressure(SPA16001_t *dev) {
	HAL_StatusTypeDef  status;
	uint8_t cfg, cfg_reg;

	status = I2C_ReadReg(dev->i2c_addr, SPA16001_REG_PRS_CFG, &cfg);
	if (status != HAL_OK)    return status;

	dev->prs_osr = (SPA16001_OSR_t)(cfg & 0x07);
	dev->prs_rate = (SPA16001_Rate_t)((cfg >> 4) & 0x0F);
	dev->kP = GetScaleFactor(dev->prs_osr);

	status = I2C_ReadReg(dev->i2c_addr, SPA16001_REG_CFG_REG, &cfg_reg);
	if (status != HAL_OK) return status;

	if (dev->prs_osr > SPA16001_OSR_8) {
		cfg_reg |= (1 << 2);
	} else {
		cfg_reg &= ~(1 << 2);
	}

	status = I2C_WriteReg(dev->i2c_addr, SPA16001_REG_CFG_REG, cfg_reg);
	if (status != HAL_OK) return status;

	return HAL_OK;
}


HAL_StatusTypeDef    SPA16001_ConfigTemperature(SPA16001_t *dev) {
	HAL_StatusTypeDef status;
	uint8_t cfg, cfg_reg;

	status = I2C_ReadReg(dev->i2c_addr, SPA16001_REG_TMP_CFG, &cfg);
	if (status != HAL_OK)    return status;

	dev->tmp_osr = (SPA16001_OSR_t)(cfg & 0x07);
	dev->tmp_rate = (SPA16001_Rate_t)((cfg >> 4) & 0x0F);
	dev->kT = GetScaleFactor(dev->tmp_osr);

	status = I2C_ReadReg(dev->i2c_addr, SPA16001_REG_CFG_REG, &cfg_reg);
	if (status != HAL_OK) return status;

	if (dev->tmp_osr > SPA16001_OSR_8) {
		cfg_reg |= (1 << 3);
	} else {
		cfg_reg &= ~(1 << 3);
	}

	status = I2C_WriteReg(dev->i2c_addr, SPA16001_REG_CFG_REG, cfg_reg);
	if (status != HAL_OK) return status;

	return HAL_OK;
}

HAL_StatusTypeDef    SPA16001_SetMode(SPA16001_t *dev, uint8_t mode) {
	HAL_StatusTypeDef  status;
	uint8_t meas_cfg;

	status = I2C_ReadReg(dev->i2c_addr, SPA16001_REG_MEAS_CFG, &meas_cfg);
	if (status != HAL_OK)  return status;

	meas_cfg = (meas_cfg & 0xF8) | (mode & 0x07);

	return I2C_WriteReg(dev->i2c_addr, SPA16001_REG_MEAS_CFG, meas_cfg);
}

HAL_StatusTypeDef    SPA16001_GetRawPressure(SPA16001_t *dev, int32_t *raw_prs) {
	HAL_StatusTypeDef  status;
	uint8_t data[3];

	status = I2C_ReadRegs(dev->i2c_addr, SPA16001_REG_PSR_B2, data, 3);
	if (status != HAL_OK)  return status;

	uint32_t raw = ((uint32_t)data[0] << 16 )| ((uint32_t)data[1] << 8) | (uint32_t)data[2];
	*raw_prs = TwosComplement(raw, 24);

	return HAL_OK;
}

HAL_StatusTypeDef    SPA16001_GetRawTemperature(SPA16001_t *dev, int32_t *raw_temp) {
	HAL_StatusTypeDef status;
	uint8_t data[3];

	status = I2C_ReadRegs(dev->i2c_addr, SPA16001_REG_TMP_B2, data, 3);
	if (status != HAL_OK)    return status;

	uint32_t raw = ((uint32_t)data[0] << 16 ) | ((uint32_t)data[1] << 8 ) | (uint32_t)data[2];
	*raw_temp = TwosComplement(raw, 24);

	return HAL_OK;
}

float                SPA16001_CalcCompPressure(SPA16001_t *dev, int32_t raw_prs, int32_t raw_temp) {
	float prs_sc = (float)raw_prs / dev->kP;
	float temp_sc = (float)raw_temp / dev->kT;

	return (float)dev->calib.c00 + prs_sc * ((float)dev->calib.c10 + prs_sc * ((float)dev->calib.c20 + prs_sc * ((float)dev->calib.c30 + prs_sc * (float)dev->calib.c40)))
			+ temp_sc * ((float)dev->calib.c01 + prs_sc * ((float)dev->calib.c11 + prs_sc * ((float)dev->calib.c21 + prs_sc * (float)dev->calib.c31)));
}

float                SPA16001_CalcCompTemperature(SPA16001_t *dev, int32_t raw_temp) {
	float temp_sc = (float)raw_temp /dev->kT;

	return (float)dev->calib.c0 * 0.5f + (float)dev->calib.c1 * temp_sc;

}

HAL_StatusTypeDef    SPA16001_GetPressure(SPA16001_t *dev, float *pressure) {
	HAL_StatusTypeDef status;
	int32_t raw_prs, raw_temp;

	status = SPA16001_GetRawPressure(dev, &raw_prs);
	if (status != HAL_OK)    return status;

	status = SPA16001_GetRawTemperature(dev, &raw_temp);
	if (status != HAL_OK)    return status;

	*pressure = SPA16001_CalcCompPressure(dev, raw_prs, raw_temp);

	return HAL_OK;

}

HAL_StatusTypeDef    SPA16001_GetTemperature(SPA16001_t *dev, float *temperature) {
	HAL_StatusTypeDef status;
	int32_t raw_temp;

	status = SPA16001_GetRawTemperature(dev, &raw_temp);
	if (status != HAL_OK)    return status;

	*temperature = SPA16001_CalcCompTemperature(dev, raw_temp);
	return HAL_OK;

}


HAL_StatusTypeDef    SPA16001_SetPressureConfig(SPA16001_t *dev, SPA16001_Rate_t rate, SPA16001_OSR_t osr) {
	HAL_StatusTypeDef status;
	uint8_t cfg = ((uint8_t)rate << 4) | ((uint8_t)osr & 0x07);

	dev->prs_osr = osr;
	dev->prs_rate = rate;
	dev->kP = GetScaleFactor(dev->prs_osr);

	status = I2C_WriteReg(dev->i2c_addr, SPA16001_REG_CFG_REG, cfg);
	if (status != HAL_OK)    return status;

	if (osr > SPA16001_OSR_8){
		uint8_t cfg_reg;

		status = I2C_ReadReg(dev->i2c_addr, SPA16001_REG_CFG_REG, &cfg_reg);
		if (status != HAL_OK)    return status;

		cfg_reg |= 0x04;
		status = I2C_WriteReg(dev->i2c_addr, SPA16001_REG_CFG_REG, cfg_reg);
		if(status != HAL_OK)    return status;

	}
	return HAL_OK;

}
HAL_StatusTypeDef    SPA16001_SetTemperatureConfig(SPA16001_t *dev, SPA16001_Rate_t rate, SPA16001_OSR_t osr) {
	HAL_StatusTypeDef status;
	uint8_t cfg = ((uint8_t)rate << 4) | ((uint8_t)osr & 0x07);

	dev->tmp_osr = osr;
	dev->tmp_rate = rate;
	dev->kT = GetScaleFactor(dev->tmp_osr);

	status = I2C_WriteReg(dev->i2c_addr, SPA16001_REG_CFG_REG, cfg);
	if (status != HAL_OK)    return status;

	if (osr > SPA16001_OSR_8){
		uint8_t cfg_reg;

		status = I2C_ReadReg(dev->i2c_addr, SPA16001_REG_CFG_REG, &cfg_reg);
		if (status != HAL_OK)    return status;

		cfg_reg |= 0x08;

		status = I2C_WriteReg(dev->i2c_addr, SPA16001_REG_CFG_REG, cfg_reg);
		if (status != HAL_OK)    return status;
	}
	return HAL_OK;

}


//FIFO配置
HAL_StatusTypeDef    SPA16001_EnableFIFO(SPA16001_t *dev, uint8_t enable) {
	HAL_StatusTypeDef status;
	uint8_t cfg_reg;

	status = I2C_ReadReg(dev->i2c_addr, SPA16001_REG_CFG_REG, &cfg_reg);
	if (status != HAL_OK) return status;

	if (enable) {
		cfg_reg |= (1 << 1);
	} else {
		cfg_reg &= ~(1 << 1);
	}

	status = I2C_WriteReg(dev->i2c_addr, SPA16001_REG_CFG_REG, cfg_reg);
	if (status != HAL_OK) return status;

	return HAL_OK;
}

HAL_StatusTypeDef    SPA16001_GetFIFOStatus(SPA16001_t *dev, uint8_t *full, uint8_t *empty) {
	HAL_StatusTypeDef status;
	uint8_t sts;

	status = I2C_ReadReg(dev->i2c_addr, SPA16001_REG_FIFO_STS, &sts);
	if (status != HAL_OK) return status;

	*full = (sts & SPA16001_FIFO_FULL_BIT) ? 1: 0;
	*empty = (sts & SPA16001_FIFO_EMPTY_BIT) ? 1: 0;

	return HAL_OK;
}

HAL_StatusTypeDef SPA16001_ReadFIFO(SPA16001_t *dev, SPA16001_FIFOEntry_t *entries, uint8_t max_count, uint8_t *out_count) {
	HAL_StatusTypeDef status;

	uint8_t fifo_full, fifo_empty;
	uint8_t count = 0;
	//先把输出数量清0，防止外部读到上次的残留值。
	*out_count = 0;

	//第一次读取FIFO状态，知道现在是满，是空，还是有数据
	status = SPA16001_GetFIFOStatus(dev, &fifo_full, &fifo_empty);
	if (status != HAL_OK) return HAL_OK;
	//是空就直接返回，不用读取
	if (fifo_empty)  return HAL_OK;

	//满了就是读取32条，不满的话就读到空为止
	uint8_t n = fifo_full ? 32 : max_count;
	if (n > max_count) n = max_count;

	for(uint8_t i = 0; i < n; i ++) {
		uint8_t buf[3];//每条数据3个字节

		//每次都从寄存器地址0X00开始读取3字节，传感器读完移交会自动把下一条放到0X00
		status = I2C_ReadRegs(dev->i2c_addr, SPA16001_REG_PSR_B2, buf, 3);
		if (status != HAL_OK) return status;

		//把3个字节拼接成一个24位的数，24位原始数据，buf[0]是最高字节，buf[2]是最低字节。
		uint32_t raw = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | (uint32_t)buf[2];

		//FIFO空标志：硬件返回0x8000000，datasheet Page11规定，FIFO读空后硬件会一直返回0x8000000,遇到这个就说明没有数据了，停止读取。
		if (raw == 0x8000000u) break;

		//根据datasheet Page11确认，LSB = 1 是压力， LSB = 0 是温度。最低位判断类型。
		entries[count].is_pressure = (uint8_t)(raw & 0x01u);
		// 去掉最低位的标志位。去掉LSB后做24位有符号转换
		entries[count].raw = TwosComplement(raw & 0xFFFFFEu, 24);
		count++;

		// 如果FIFO不是满的，没次读取一条就检查一次是不是空了
		if (!fifo_full) {
			status = SPA16001_GetFIFOStatus(dev, &fifo_full, &fifo_empty);
			if (status != HAL_OK) return status;
			if (fifo_empty) break;

		}
	}

	//讲实际读取到的条数告诉调用者
	*out_count = count;
	 // 读完后清空FIFO
	status = SPA16001_FlushFIFO(dev);
	if (status != HAL_OK) return status;

	return HAL_OK;
}

HAL_StatusTypeDef    SPA16001_FlushFIFO(SPA16001_t *dev) {
	HAL_StatusTypeDef status;

	status = I2C_WriteReg(dev->i2c_addr, SPA16001_REG_RESET, SPA16001_FIFO_FLUSH);
	if (status != HAL_OK) return status;

	return HAL_OK;
}

