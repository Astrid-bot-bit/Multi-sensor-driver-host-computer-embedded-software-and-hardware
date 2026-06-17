/*
 * bsp_timer.c
 *
 *  Created on: 2026年6月3日
 *      Author: vivian.huowy
 */


#include "bsp_timer.h"

extern TIM_HandleTypeDef htim6;

static const uint16_t s_period_lut[8] = {
	9999u,  /* 0: 1   Hz */
	4999u,  /* 1: 2   Hz */
	2499u,  /* 2: 4   Hz */
	 999u,  /* 3: 10  Hz  ← 上电默认 */
	 499u,  /* 4: 20  Hz */
	 249u,  /* 5: 40  Hz */
	 142u,  /* 6: ~70 Hz (实际69.9Hz) */
	  49u,  /* 7: 200 Hz */
};

static volatile bool s_tick = false;

void BSP_Timer_Init(void) {
	BSP_Timer_SetRate(3U);
	HAL_TIM_Base_Start_IT(&htim6);
}

void BSP_Timer_SetRate(uint8_t odr_idx) {
	if (odr_idx > 7U) odr_idx = 7U;

	HAL_TIM_Base_Stop_IT(&htim6);
	__HAL_TIM_SET_AUTORELOAD(&htim6, s_period_lut[odr_idx]);
	__HAL_TIM_SET_COUNTER(&htim6, 0U);
	s_tick = false;
	HAL_TIM_Base_Start_IT(&htim6);
}

bool BSP_Timer_ConsumeTick(void) {
	if (!s_tick) return false;
	s_tick = false;
	return true;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM6) {
		s_tick = true;
	}
}
