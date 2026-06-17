/*
 * ui.c
 *
 *  Created on: 2026年3月22日
 *      Author: vivian.huowy
 */

#include "ui.h"
#include <stdio.h>

/* ================================================================
   调色板 (0x00RRGGBB — 驱动自动转 RGB565)
   ================================================================ */
#define C_TITLE_BG    0x000050C0   /* 深蓝标题背景   */
#define C_TITLE_LINE  0x004488FF   /* 标题底部分隔线  */
#define C_TITLE_TXT   0x00D8EEFF   /* 标题文字       */

#define C_SEN_FILL    0x00001828   /* 传感器块深青背景 */
#define C_SEN_BDR     0x0000AAFF   /* 传感器框线      */
#define C_SEN_LBL     0x0080C0FF   /* 标签文字        */
#define C_SEN_VBKG    0x00001010   /* 数值区暗背景    */
#define C_SEN_VAL     0x0000FF88   /* 传感器数值亮绿  */

#define C_INF_FILL    0x00001020   /* 信息格深背景    */
#define C_INF_BDR     0x00304860   /* 信息格框线      */
#define C_INF_LBL     0x005070A0   /* 信息标签暗蓝    */
#define C_INF_VAL     0x0000FF88   /* 数值亮绿        */
#define C_INF_WARN    0x00FF8800   /* 超限橙色警告    */

#define C_STA_FILL    0x00001A10   /* 状态条背景      */
#define C_STA_BDR     0x00806600   /* 状态条框线琥珀  */
#define C_STA_TXT     0x00FFCC00   /* 状态文字琥珀    */

#define C_BOT_FILL    0x00080810   /* 底部栏背景      */
#define C_BOT_TXT     0x005070A0   /* 底部芯片信息暗蓝 */
#define C_TIME_TXT    0x00A0C8FF   /* 时间文字淡蓝    */
#define C_DOT_OK      0x0000CC44   /* 绿色状态点      */
#define C_DIVIDER     0x00283848   /* 分隔线          */

/* ================================================================
   布局常量（像素）
   ================================================================ */
/* 左列 */
#define LX   10
#define LW   105
/* 右列 */
#define RX   125
#define RW   105

/* 传感器块 */
#define SEN_Y       36
#define SEN_H       70
#define SEN_LBL_DY   4   /* 标签 y 偏移 */
#define SEN_VAL_DY  22   /* 数值 y 偏移 */
#define SEN_VAL_H   40   /* 数值清除高度 */

/* 信息格 */
#define INF_Y      110
#define INF_RH      36   /* 每行高度 */
#define INF_LBL_DY   2
#define INF_VAL_DY  20
#define INF_VAL_H   16

/* 状态条 */
#define STA_Y      186
#define STA_H       22

/* 底部栏 */
#define DIV_Y      208
#define BOT_Y      210
#define BOT_H       30
#define BOT_TY     217   /* 底部文字 y = BOT_Y+7 */
#define TIME_X     150   /* 时间串起始 x        */
#define TIME_CLR_W  72   /* 时间清除宽度        */
#define DOT_X      232
#define DOT_Y      225
#define DOT_R        5

/* ================================================================
   内部辅助宏
   ================================================================ */
/* 填充矩形（使用前景色） */
#define FILL(x,y,w,h,c)  do{ LCD_SetColor(c); LCD_FillRect(x,y,w,h); }while(0)
/* 画框线 */
#define RECT(x,y,w,h,c)  do{ LCD_SetColor(c); LCD_DrawRect(x,y,w,h); }while(0)

/* ================================================================
   UI_Init
   ================================================================ */
void UI_Init(void)
{
    /* 清屏黑色 */
    LCD_SetBackColor(0x00000000);
    LCD_SetColor(0x00000000);
    LCD_Clear();

    /* ── 标题栏 ─────────────────────────────── */
    FILL(0, 0, 240, 32, C_TITLE_BG);
    LCD_SetColor(C_TITLE_LINE);
    LCD_DrawLine_H(0, 31, 240);
    LCD_SetBackColor(C_TITLE_BG);
    LCD_SetTextFont(&CH_Font24);
    LCD_SetColor(C_TITLE_TXT);
    LCD_DisplayText(66, 4, "Vivian");

    /* ── 压力块（左） ────────────────────────── */
    FILL(LX, SEN_Y, LW, SEN_H, C_SEN_FILL);
    RECT(LX, SEN_Y, LW, SEN_H, C_SEN_BDR);
    LCD_SetBackColor(C_SEN_FILL);
    LCD_SetTextFont(&CH_Font16);
    LCD_SetColor(C_SEN_LBL);
    LCD_DisplayText(LX + 6, SEN_Y + SEN_LBL_DY, "Pre(Pa)");

    /* ── 温度块（右） ────────────────────────── */
    FILL(RX, SEN_Y, RW, SEN_H, C_SEN_FILL);
    RECT(RX, SEN_Y, RW, SEN_H, C_SEN_BDR);
    LCD_SetBackColor(C_SEN_FILL);
    LCD_SetTextFont(&CH_Font16);
    LCD_SetColor(C_SEN_LBL);
    LCD_DisplayText(RX + 14, SEN_Y + SEN_LBL_DY, "Temp(C)");

    /* ── 信息行 1：SmpRate | OvrSmpl ─────────── */
    {
        uint16_t y = INF_Y;
        /* 左格 */
        FILL(LX, y, LW, INF_RH, C_INF_FILL);
        RECT(LX, y, LW, INF_RH, C_INF_BDR);
        LCD_SetBackColor(C_INF_FILL);
        LCD_SetTextFont(&CH_Font16);
        LCD_SetColor(C_INF_LBL);
        LCD_DisplayText(LX + 4, y + INF_LBL_DY, "SmpRate");
        /* 右格 */
        FILL(RX, y, RW, INF_RH, C_INF_FILL);
        RECT(RX, y, RW, INF_RH, C_INF_BDR);
        LCD_SetBackColor(C_INF_FILL);
        LCD_SetTextFont(&CH_Font16);
        LCD_SetColor(C_INF_LBL);
        LCD_DisplayText(RX + 4, y + INF_LBL_DY, "OvrSmpl");
    }

    /* ── 信息行 2：Voltage | Current ─────────── */
    {
        uint16_t y = INF_Y + INF_RH;
        /* 左格 */
        FILL(LX, y, LW, INF_RH, C_INF_FILL);
        RECT(LX, y, LW, INF_RH, C_INF_BDR);
        LCD_SetBackColor(C_INF_FILL);
        LCD_SetTextFont(&CH_Font16);
        LCD_SetColor(C_INF_LBL);
        LCD_DisplayText(LX + 4, y + INF_LBL_DY, "Voltage");
        /* 右格 */
        FILL(RX, y, RW, INF_RH, C_INF_FILL);
        RECT(RX, y, RW, INF_RH, C_INF_BDR);
        LCD_SetBackColor(C_INF_FILL);
        LCD_SetTextFont(&CH_Font16);
        LCD_SetColor(C_INF_LBL);
        LCD_DisplayText(RX + 4, y + INF_LBL_DY, "Current");
    }

    /* ── 状态条 ──────────────────────────────── */
    FILL(LX, STA_Y, 220, STA_H, C_STA_FILL);
    RECT(LX, STA_Y, 220, STA_H, C_STA_BDR);

    /* ── 分隔线 & 底部栏 ─────────────────────── */
    LCD_SetColor(C_DIVIDER);
    LCD_DrawLine_H(0, DIV_Y, 240);
    FILL(0, BOT_Y, 240, BOT_H, C_BOT_FILL);
    LCD_SetBackColor(C_BOT_FILL);
    LCD_SetTextFont(&CH_Font16);
    LCD_SetColor(C_BOT_TXT);
    LCD_DisplayText(8, BOT_TY, "Sensor");

    /* 绿色状态点 */
    LCD_SetColor(C_DOT_OK);
    LCD_FillCircle(DOT_X, DOT_Y, DOT_R);

    /* ── 初始占位符 ──────────────────────────── */
    UI_UpdatePressure(-1.0f);
    UI_UpdateTemperature(-999.0f);
    UI_UpdateSampleRate(0);
    UI_UpdateOversampling(0);
    UI_UpdateVoltage(-1.0f);
    UI_UpdateCurrent(-1.0f);
    UI_SetSensorType("---");
    UI_UpdateTime(0, 0, 0);
}

/* ================================================================
   UI_UpdatePressure
   ================================================================ */
void UI_UpdatePressure(float pressure)
{
    FILL(LX + 1, SEN_Y + SEN_VAL_DY, LW - 2, SEN_VAL_H, C_SEN_VBKG);
    LCD_SetBackColor(C_SEN_VBKG);
    LCD_SetAsciiFont(&ASCII_Font20);
    LCD_SetColor(C_SEN_VAL);

    if (pressure < 0.0f) {
        LCD_DisplayString(LX + 30, SEN_Y + SEN_VAL_DY + 8, "--.-");
    } else {
        LCD_DisplayDecimals(LX + 4, SEN_Y + SEN_VAL_DY + 8, pressure, 9, 2);
    }
}

/* ================================================================
   UI_UpdateTemperature
   ================================================================ */
void UI_UpdateTemperature(float temp)
{
    FILL(RX + 1, SEN_Y + SEN_VAL_DY, RW - 2, SEN_VAL_H, C_SEN_VBKG);
    LCD_SetBackColor(C_SEN_VBKG);
    LCD_SetAsciiFont(&ASCII_Font20);
    LCD_SetColor(C_SEN_VAL);

    if (temp < -99.0f) {
        LCD_DisplayString(RX + 30, SEN_Y + SEN_VAL_DY + 8, "--.-");
    } else {
        char buf[10];
        int i = (int)temp;
        int d = (int)((temp - i )* 10 + 0.5f);
        sprintf(buf, "%d.%d", i , d);
        LCD_DisplayString(RX + 4, SEN_Y + SEN_VAL_DY + 8, buf);
    }
}

/* ================================================================
   UI_UpdateSampleRate
   ================================================================ */
void UI_UpdateSampleRate(uint32_t hz)
{
    char buf[12];
    uint16_t vy = INF_Y + INF_VAL_DY;

    FILL(LX + 1, vy, LW - 2, INF_VAL_H, C_INF_FILL);
    LCD_SetBackColor(C_INF_FILL);
    LCD_SetAsciiFont(&ASCII_Font16);
    LCD_SetColor(C_INF_VAL);

    if (hz == 0) {
        LCD_DisplayString(LX + 4, vy, " ---Hz");
    } else if (hz < 1000) {
        sprintf(buf, "%4luHz", (unsigned long)hz);
        LCD_DisplayString(LX + 4, vy, buf);
    } else {
        /* >= 1 kHz：最多 "99.99kHz" = 8 chars × 8px = 64px，格内宽 103px ✓ */
        sprintf(buf, "%.2fkHz", hz / 1000.0f);
        LCD_DisplayString(LX + 4, vy, buf);
    }
}

/* ================================================================
   UI_UpdateOversampling
   ================================================================ */
void UI_UpdateOversampling(uint16_t osr)
{
    char buf[10];
    uint16_t vy = INF_Y + INF_VAL_DY;

    FILL(RX + 1, vy, RW - 2, INF_VAL_H, C_INF_FILL);
    LCD_SetBackColor(C_INF_FILL);
    LCD_SetAsciiFont(&ASCII_Font16);
    LCD_SetColor(C_INF_VAL);

    if (osr == 0) {
        LCD_DisplayString(RX + 4, vy, "   x---");
    } else {
        /* "  x256" / "x65536"（最宽 6 chars × 8 = 48px ✓） */
        sprintf(buf, "   x%u", osr);
        LCD_DisplayString(RX + 4, vy, buf);
    }
}

/* ================================================================
   UI_UpdateVoltage
   超出 3.0 ~ 3.6 V 自动切换橙色警告
   ================================================================ */
void UI_UpdateVoltage(float voltage)
{
    char buf[10];
    uint16_t vy = INF_Y + INF_RH + INF_VAL_DY;

    FILL(LX + 1, vy, LW - 2, INF_VAL_H, C_INF_FILL);
    LCD_SetBackColor(C_INF_FILL);
    LCD_SetAsciiFont(&ASCII_Font16);

    if (voltage < 0.0f) {
        LCD_SetColor(C_INF_LBL);
        LCD_DisplayString(LX + 4, vy, " -.--V");
    } else {
        uint32_t color = (voltage < 3.0f || voltage > 3.6f) ? C_INF_WARN : C_INF_VAL;
        LCD_SetColor(color);
        sprintf(buf, "%5.2fV", voltage);   /* " 3.30V" */
        LCD_DisplayString(LX + 4, vy, buf);
    }
}

/* ================================================================
   UI_UpdateCurrent
   ================================================================ */
void UI_UpdateCurrent(float current)
{
    char buf[12];
    uint16_t vy = INF_Y + INF_RH + INF_VAL_DY;

    FILL(RX + 1, vy, RW - 2, INF_VAL_H, C_INF_FILL);
    LCD_SetBackColor(C_INF_FILL);
    LCD_SetAsciiFont(&ASCII_Font16);
    LCD_SetColor(C_INF_VAL);

    if (current < 0.0f) {
        LCD_DisplayString(RX + 4, vy, "--.-mA");
    } else {
        sprintf(buf, "%4.1fmA", current);  /* "12.5mA" */
        LCD_DisplayString(RX + 4, vy, buf);
    }
}

/* ================================================================
   UI_SetSensorType
   在状态条内显示传感器型号，格式：" Type: BMP390"
   字体使用 ASCII_Font20（20px 高），垂直居中于 STA_H=22 的状态条
   ================================================================ */
void UI_SetSensorType(const char *model)
{
    char buf[24];

    /* 清除内容区（保留框线各 1px） */
    FILL(LX + 1, STA_Y + 1, 218, STA_H - 2, C_STA_FILL);

    LCD_SetBackColor(C_STA_FILL);
    LCD_SetColor(C_STA_TXT);

    /* 拼接 "Type: " + 型号，整体显示 */
    snprintf(buf, sizeof(buf), "Type: %s", model);

    /* ASCII_Font20：字高 20px，y 偏移 1 使其在 22px 条内垂直居中 */
    LCD_SetAsciiFont(&ASCII_Font20);
    LCD_DisplayString(LX + 6, STA_Y + 1, buf);
}

/* ================================================================
   UI_UpdateTime
   只刷新底部栏时间区域，不重绘芯片信息和状态点
   ================================================================ */
void UI_UpdateTime(uint8_t hour, uint8_t min, uint8_t sec)
{
    char buf[12];
    sprintf(buf, "%02d:%02d:%02d", hour, min, sec);

    /* "HH:MM:SS" = 8 chars × 8px = 64px，从 TIME_X=150 到 214，不碰状态点 */
    FILL(TIME_X, BOT_Y + 1, TIME_CLR_W, BOT_H - 2, C_BOT_FILL);
    LCD_SetBackColor(C_BOT_FILL);
    LCD_SetAsciiFont(&ASCII_Font16);
    LCD_SetColor(C_TIME_TXT);
    LCD_DisplayString(TIME_X, BOT_TY, buf);
}
