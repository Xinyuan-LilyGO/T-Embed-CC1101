
#pragma once
#include "lvgl.h"
#include "lvgl_port/lv_port_disp.h"
#include "lvgl_port/lv_port_indev.h"
#include "lvgl_port/scr_mrg.h"
#include "assets/assets.h"
#include "peripheral/peripheral.h"

// The default is landscape screen, HEIGHT and WIDTH swap
#define DISPALY_WIDTH  TFT_HEIGHT
#define DISPALY_HEIGHT TFT_WIDTH

#define COLOR_BG       0x161823   // 漆黑色  0x161823
#define COLOR_FOCUS_ON 0x91B821   // 绿茶色
#define COLOR_TETX     0xffffff   // 丁香淡紫 0xE9D4DF
#define COLOR_4  //
#define COLOR_5  //
#define COLOR_6  //

enum{
    SCREEN0_ID = 0,
    SCREEN1_ID,
    SCREEN2_ID,
    SCREEN3_ID,
    SCREEN4_ID,
    SCREEN5_ID,
    SCREEN6_ID,
    SCREEN7_ID,
    SCREEN_ID_MAX,
};

void ui_entry(void);