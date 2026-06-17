
/*
 * ui.h
 *
 *  Created on: 2026年3月22日
 *      Author: vivian.huowy
 */

#ifndef INC_UI_H_
#define INC_UI_H_

#ifndef __UI_H__
#define __UI_H__

#include "lcd_spi_154.h"
#include <stdint.h>

/*
   屏幕布局 (240 × 240)
   ─────────────────────────────────────
   Y=  0  │       标题栏        │ H=32
   Y= 36  │ Pre(kPa) │ Temp(°C) │ H=70
   Y=110  │ SmpRate  │ OvrSmpl  │ H=36
   Y=146  │ Voltage  │ Current  │ H=36
   Y=186  │       状态条        │ H=22
   Y=208  ├─── 分隔线 ──────────┤
   Y=210  │STM32F4 168MHz HH:MM:SS •│ H=30
*/

void UI_Init(void);
void UI_UpdatePressure(float pressure);    /* < 0    → "--.-"   */
void UI_UpdateTemperature(float temp);     /* < -99  → "--.-"   */
void UI_UpdateSampleRate(uint32_t hz);     /* 0      → "---Hz"  */
void UI_UpdateOversampling(uint16_t osr);  /* 0      → "x---"   */
void UI_UpdateVoltage(float voltage);      /* < 0    → "-.--V"；超 3.0~3.6V 橙色 */
void UI_UpdateCurrent(float current);      /* < 0    → "--.-mA" */
void UI_SetSensorType(const char *model);
void UI_UpdateTime(uint8_t h, uint8_t m, uint8_t s);

#endif /* __UI_H__ */


#endif /* INC_UI_H_ */
