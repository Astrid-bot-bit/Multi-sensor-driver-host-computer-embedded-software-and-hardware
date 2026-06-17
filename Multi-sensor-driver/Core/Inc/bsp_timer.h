/*
 * bsp_timer.h
 *
 *  Created on: 2026年6月3日
 *      Author: vivian.huowy
 */

#ifndef INC_BSP_TIMER_H_
#define INC_BSP_TIMER_H_

#pragma once
#include "stm32f4xx_hal.h"
#include <stdbool.h>

void BSP_Timer_Init(void);

void BSP_Timer_SetRate(uint8_t odr_idx);

bool BSP_Timer_ConsumeTick(void);

#endif /* INC_BSP_TIMER_H_ */
