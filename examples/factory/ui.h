
#pragma once
#include "lvgl.h"
#include "lvgl_port/port_disp.h"
#include "lvgl_port/port_indev.h"
#include "lvgl_port/port_scr_mrg.h"
#include "assets/assets.h"
#include "peripheral/peripheral.h"

// The default is landscape screen, HEIGHT and WIDTH swap
#define DISPALY_WIDTH  TFT_HEIGHT
#define DISPALY_HEIGHT TFT_WIDTH

#define FONT_MONO_BOLD  &Font_Mono_Bold_14
#define FONT_MONO_LIGHT &Font_Mono_Light_14
#define FONT_MENU_ITEM  &Font_Mono_Bold_14

// color

// #define COLOR_BG       0x161823   // 漆黑色   0x161823
// #define COLOR_FOCUS_ON 0x91B821   // 绿茶色   0x91B821
// #define COLOR_TEXT     0xffffff   // 丁香淡紫 0xE9D4DF
// #define COLOR_BORDER   0xBBBBBB   // 
// #define COLOR_5  //
// #define COLOR_6  //

// #define COLOR_BG       0xffffff   // 漆黑色   0x161823
// #define COLOR_FOCUS_ON 0x91B821   // 绿茶色   0x91B821
// #define COLOR_TEXT     0x161823   // 丁香淡紫 0xE9D4DF
// #define COLOR_BORDER   0xBBBBBB   // 
// #define COLOR_5  //
// #define COLOR_6  //

/**
 * 漆黑色   0x161823
 * 绿茶色   0x91B821
 * 丁香淡紫 0xE9D4DF
 * 纯白色   0xffffff
*/

// screen id
enum{
    SCREEN0_ID = 0,
    SCREEN1_ID,
    SCREEN2_ID,
    SCREEN3_ID,
    SCREEN4_ID,
    SCREEN5_ID,
    SCREEN6_ID,
    SCREEN7_ID,
    SCREEN7_1_ID,
    SCREEN7_2_ID,
    SCREEN7_3_ID,
    SCREEN_ID_MAX,
};

// msg id
enum{
    // clock msg subsribe
    MSG_CLOCK_HOUR,
    MSG_CLOCK_MINUTE,
    MSG_CLOCK_SECOND,

    // ui rotation send
    MSG_UI_ROTATION_ST, // uint8_t
    // ui theme send
    MSG_UI_THEME_MODE,  // uint8_t
};

void ui_entry(void);