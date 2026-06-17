	/*
 * bsp_dac_adc.c
 *
 *  Created on: 2026年6月9日
 *      Author: vivian.huowy
 */

#include "bsp_dac_adc.h"
#include "adc.h"
#include "dac.h"


static uint16_t adc_get_average(uint32_t ch, uint8_t times) {
	ADC_ChannelConfTypeDef cfg = {0};
	cfg.Channel			= ch;
	cfg.Rank 			= 1;
	cfg.SamplingTime 	= ADC_SAMPLETIME_480CYCLES;
	cfg.Offset 			= 0;
	HAL_ADC_ConfigChannel(&hadc1, &cfg);

	uint32_t sum = 0;
	for (uint8_t i = 0; i < times; i++) {
		HAL_ADC_Start(&hadc1);
		HAL_ADC_PollForConversion(&hadc1, 10);
		sum += HAL_ADC_GetValue(&hadc1);
	}
	return (uint16_t)(sum / times);
}

void BSP_DAC_SetVoltage(float voltage) {
	if (voltage > DAC_OUTPUT_MAX) voltage = DAC_OUTPUT_MAX;
	if (voltage < 0.0f) voltage = 0.0f;

	float dac_out = voltage / AMP_GAIN;
	if (dac_out > DAC_VREF) dac_out = DAC_VREF;

	uint32_t val = (uint32_t)(dac_out / DAC_VREF * 4095);
	HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, val);

}


float BSP_ADC_GetSensorVoltage(void) {
	uint16_t raw = adc_get_average(ADC_CHANNEL_14, 20);
	return (float)raw / 4096.0f * ADC_VREF;
}

float BSP_ADC_GetSensorCurrent(void) {
	uint16_t raw = adc_get_average(ADC_CH_CURRENT, 20);
	float v_ina = (float)raw / 4096.0f * ADC_VREF;
	float i_mA = (v_ina / (IN333_GAIN * SENSOR_R)) * 1000.0f;
	return i_mA;
}
