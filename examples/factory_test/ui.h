
#pragma once
#include "lvgl.h"
#include "lvgl_port/port_disp.h"
#include "lvgl_port/port_indev.h"
#include "lvgl_port/port_scr_mrg.h"
#include "assets/assets.h"
#include "peripheral/peripheral.h"


#define T_EMBED_CC1101_SF_VER "v1.0 24.12.03"

// The default is landscape screen, HEIGHT and WIDTH swap
#define DISPALY_WIDTH  TFT_HEIGHT
#define DISPALY_HEIGHT TFT_WIDTH


#define FONT_BOLD_14  &Font_Mono_Bold_14
#define FONT_BOLD_16  &Font_Mono_Bold_16
#define FONT_BOLD_18  &Font_Mono_Bold_18
#define FONT_BOLD_20  &Font_Mono_Bold_20
#define FONT_LIGHT_14 &Font_Mono_Light_14
// #define FONT_LIGHT_16 
// #define FONT_LIGHT_18 

// screen id
enum{
    SCREEN0_ID = 0,
    SCREEN1_ID,
    SCREEN2_ID,
    SCREEN3_ID,
    SCREEN4_ID,
    SCREEN4_1_ID,
    SCREEN5_ID,
    SCREEN6_ID,
    SCREEN7_ID,
    SCREEN7_1_ID,
    SCREEN7_2_ID,
    SCREEN7_3_ID,
    SCREEN8_ID,
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