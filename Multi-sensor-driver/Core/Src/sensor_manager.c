/*
 * sensor_manager.c
 *
 *  Created on: 2026年4月28日
 *      Author: vivian.huowy
 */


#include "sensor_manager.h"
#include <string.h>
#include "drv_spl07003.h"
#include "drv_spa07005.h"
#include "drv_spa16001.h"
#include <stdio.h>

typedef struct {
	SensorProductType_t   type;
	const char    *name;
	HAL_StatusTypeDef    (*detect)(uint8_t *found_addr);
	HAL_StatusTypeDef    (*init)(uint8_t addr);
	HAL_StatusTypeDef    (*read)(SensorData_t *out);
}SensorEntry_t;


static const SPA07005_P_ODR_t      s_odr_map[8]  = {
    SPA07005_ODR_1HZ, SPA07005_ODR_2HZ, SPA07005_ODR_4HZ,  SPA07005_ODR_10HZ,
    SPA07005_ODR_20HZ,SPA07005_ODR_40HZ,SPA07005_ODR_70HZ, SPA07005_ODR_200HZ,
};
static const SPA07005_PressureMode_t s_posr_map[8] = {
    SPA07005_PRESSURE_LLP, SPA07005_PRESSURE_LP,  SPA07005_PRESSURE_STD,
    SPA07005_PRESSURE_HP,  SPA07005_PRESSURE_HHP, SPA07005_PRESSURE_HHP,
    SPA07005_PRESSURE_HHP, SPA07005_PRESSURE_HHP,
};
static const SPA07005_TempMode_t    s_tosr_map[8] = {
    SPA07005_TEMP_LLP, SPA07005_TEMP_LP,  SPA07005_TEMP_STD,
    SPA07005_TEMP_HP,  SPA07005_TEMP_HHP, SPA07005_TEMP_HHP,
    SPA07005_TEMP_HHP, SPA07005_TEMP_HHP,
};

static HAL_StatusTypeDef    spl07003_detect(uint8_t *found_addr);
static HAL_StatusTypeDef    spl07003_init(uint8_t addr);
static HAL_StatusTypeDef    spl07003_read(SensorData_t *out);

static HAL_StatusTypeDef    spa07005_detect(uint8_t *found_addr);
static HAL_StatusTypeDef    spa07005_init(uint8_t addr);
static HAL_StatusTypeDef    spa07005_read(SensorData_t *out);

//static HAL_StatusTypeDef    spa16001_detect(uint8_t *found_addr);
//static HAL_StatusTypeDef    spa16001_init(uint8_t addr);
//static HAL_StatusTypeDef    spa16001_read(SensorData_t *out);

static const SensorEntry_t SENSOR_TABLE[] = {
    {PRODUCT_SPA07_005, "SPA07005", spa07005_detect, spa07005_init, spa07005_read},
    {PRODUCT_SPL07_003, "SPL07003", spl07003_detect, spl07003_init, spl07003_read},
};

#define SENSOR_TABLE_SIZE (sizeof(SENSOR_TABLE) / sizeof(SENSOR_TABLE[0]))

static const SensorEntry_t *g_entry = NULL;
uint8_t g_addr = 0x00;

static SPL07003_t g_spl07003;
static SPA07005_t g_spa07005;
//static SPA16001_t g_spa16001;


static HAL_StatusTypeDef    spl07003_detect(uint8_t *found_addr) {
	HAL_StatusTypeDef  status;
	uint8_t chip_id = 0;

	const uint8_t addrs[] = {SPL07003_I2C_ADDR_LOW, SPL07003_I2C_ADDR_HIGH};
	for (int i = 0; i < 2; i ++) {
		g_spl07003.i2c_addr = addrs[i];
		status = SPL07003_ReadID(&g_spl07003, &chip_id);
		if (status == HAL_OK && chip_id == SPL07003_PRODUCT_ID) {
			*found_addr = addrs[i];
			return HAL_OK;
		}
	}
//	g_spl07003.i2c_addr = 0;
	return HAL_ERROR;
}

static HAL_StatusTypeDef    spl07003_init(uint8_t addr) {
	HAL_StatusTypeDef status;

	status = SPL07003_Init(&g_spl07003, addr);
	if (status != HAL_OK)    return status;

	SPL07003_ConfigPressure(&g_spl07003);
	SPL07003_ConfigTemperature(&g_spl07003);

	return HAL_OK;
}

static HAL_StatusTypeDef    spl07003_read(SensorData_t *out) {
	HAL_StatusTypeDef status;
	int32_t raw_p = 0;
	int32_t raw_t = 0;
	float pressure, temperature;

	status = SPL07003_GetRawPressure(&g_spl07003, &raw_p);
	if (status != HAL_OK) return status;
	status = SPL07003_GetRawTemperature(&g_spl07003, &raw_t);
	if (status != HAL_OK) return status;

	status  = SPL07003_GetPressure(&g_spl07003, &pressure);
	if (status != HAL_OK)    return status;

	status = SPL07003_GetTemperature(&g_spl07003, &temperature);
	if (status != HAL_OK)    return status;

	out->pressure_pa = pressure;
	out->temperature = temperature;
	out->press_raw = (uint32_t)raw_p;
	out->temp_raw = raw_t;

	out->press_osr = (uint8_t)g_spl07003.prs_osr;
	out->press_rate = (uint8_t)g_spl07003.prs_rate;
	out->temp_osr = (uint8_t)g_spl07003.tmp_osr;
	out->temp_rate = (uint8_t)g_spl07003.tmp_rate;

	return HAL_OK;
}

static HAL_StatusTypeDef    spa07005_detect(uint8_t *found_addr) {
	HAL_StatusTypeDef status;
	uint8_t chip_id = 0;

	const uint8_t addrs[] = {SPA07005_I2C_ADDR_HIGH, SPA07005_I2C_ADDR_LOW};
	for (int i = 0; i < 2; i++) {
		g_spa07005.i2c_addr = addrs[i];
		status = SPA07005_ReadID(&g_spa07005, &chip_id);
		if (status == HAL_OK) {
			*found_addr = addrs[i];
			return HAL_OK;
		}
	}
	g_spa07005.i2c_addr = 0;
	return HAL_ERROR;
}

static HAL_StatusTypeDef    spa07005_init(uint8_t addr) {
	return SPA07005_Init(&g_spa07005, addr);
}

static HAL_StatusTypeDef    spa07005_read(SensorData_t *out) {
	HAL_StatusTypeDef status;

	status = SPA07005_ReadPT(&g_spa07005);
	if (status != HAL_OK)    return status;

	out->pressure_pa = g_spa07005.pressure;
	out->temperature = g_spa07005.temperature;
	out->press_raw   = (int32_t)g_spa07005.raw_pressure;
	out->temp_raw    = g_spa07005.raw_temperature;

	out->press_osr   = (uint8_t)g_spa07005.prs_osr;
	out->press_rate  = (uint8_t)g_spa07005.prs_odr;
	out->temp_osr    = (uint8_t)g_spa07005.tmp_osr;
	out->temp_rate   = (uint8_t)g_spa07005.tmp_odr;

	return HAL_OK;
}

//static HAL_StatusTypeDef spa16001_detect(uint8_t *found_addr)
//{
//    (void)found_addr;
//    /* TODO: 找到区分寄存器后填入 */
//    return HAL_ERROR;
//}
//
//static HAL_StatusTypeDef spa16001_init(uint8_t addr)
//{
//    /* 与 SPL07003 硬件兼容，直接复用 */
//    return SPL07003_Init(&g_spa16001, addr);
//}
//
//static HAL_StatusTypeDef spa16001_read(SensorData_t *out)
//{
//    /* 读取逻辑与 SPL07003 完全一致，压力偏差 1kPa 是硬件个体差异 */
//    float pressure, temperature;
//    if (SPL07003_GetPressure   (&g_spa16001, &pressure)    != HAL_OK) return HAL_ERROR;
//    if (SPL07003_GetTemperature(&g_spa16001, &temperature) != HAL_OK) return HAL_ERROR;
//    out->pressure_pa    = pressure;
//    out->temperature    = temperature;
//    out->oversampling   = (uint16_t) g_spa16001.prs_osr;
//    out->sample_rate_hz = (uint32_t)g_spa16001.prs_rate;
//    return HAL_OK;
//}

SensorProductType_t Sensor_GetType(void) {
	if (g_entry == NULL) return PRODUCT_UNKNOWN;
	return g_entry->type;
}

SensorProductType_t    Sensor_AutoDetect(I2C_HandleTypeDef *hi2c) {
	HAL_StatusTypeDef status;
	(void)hi2c;
	g_entry = NULL;

	for (size_t i = 0; i < SENSOR_TABLE_SIZE; i++) {
		status = SENSOR_TABLE[i].detect(&g_addr);
		if (status == HAL_OK) {
			g_entry = &SENSOR_TABLE[i];
			return g_entry->type;
		}
	}
	return PRODUCT_UNKNOWN;
}

HAL_StatusTypeDef    Sensor_Init(void) {
	if (g_entry == NULL)    return HAL_ERROR;
	return g_entry->init(g_addr);
}

HAL_StatusTypeDef    Sensor_Read(SensorData_t *out) {
	if (g_entry == NULL || out == NULL)    return HAL_ERROR;
	return g_entry->read(out);
}

const char    *Sensor_GetName(void) {
	if (g_entry == NULL)    return "NO SENSOR";
	return g_entry->name;
}

HAL_StatusTypeDef Sensor_SetConfig(uint8_t press_rate, uint8_t press_osr, uint8_t temp_rate,  uint8_t temp_osr) {
	HAL_StatusTypeDef status;

	if (g_entry == NULL) return HAL_ERROR;
	if (press_rate > 7U) press_rate = 7U;
	if (press_osr > 7U) press_osr = 7U;
	if (temp_rate > 6U) temp_rate = 6U;
	if (temp_osr > 7U) temp_osr = 7U;

	if (g_entry->type == PRODUCT_SPL07_003) {
		g_spl07003.prs_rate = press_rate;
		g_spl07003.prs_osr = press_osr;
		g_spl07003.tmp_rate = temp_rate;
		g_spl07003.tmp_osr = temp_osr;

		status = SPL07003_ConfigPressure(&g_spl07003);
		if (status != HAL_OK) return status;
		status = SPL07003_ConfigTemperature(&g_spl07003);
		if (status != HAL_OK) return status;

		return HAL_OK;
	}

	if (g_entry->type == PRODUCT_SPA07_005) {
		status = SPA07005_SetPressureConfig(&g_spa07005, s_odr_map[press_rate], s_posr_map[press_osr]);
		if (status != HAL_OK) return status;

		status = SPA07005_SetTemperatureConfig(&g_spa07005, s_odr_map[temp_rate], s_tosr_map[temp_osr]);
		if (status != HAL_OK) return status;
	}

	return HAL_ERROR;
}
