/*
 * bsp_dac_adc.h
 *
 *  Created on: 2026年6月9日
 *      Author: vivian.huowy
 */

#ifndef INC_BSP_DAC_ADC_H_
#define INC_BSP_DAC_ADC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"


#define DAC_VREF 		5.0f
#define ADC_VREF 		5.0f

#define DAC_OUTPUT_MAX 	3.5f

#define AMP_GAIN 		1.404f

#define IN333_RG 		470.0f
#define IN333_GAIN 		213.7f//(1.0f + 100000.0f / IN333_RG)   //213.7
#define SENSOR_R 		10.0f

#define ADC_CH_VOLTAGE	ADC_CHANNEL_14
#define ADC_CH_CURRENT	ADC_CHANNEL_6


void BSP_DAC_SetVoltage(float voltage);
float BSP_ADC_GetSensorVoltage(void);
float BSP_ADC_GetSensorCurrent(void);

#ifdef __cplusplus
}
#endif

#endif /* INC_BSP_DAC_ADC_H_ */
