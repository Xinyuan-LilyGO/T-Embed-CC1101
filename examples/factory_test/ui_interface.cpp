
#include "lvgl.h"
#include "peripheral/peripheral.h"

//************************************[ screen 0 ]****************************************** menu
#if 1

#endif
//************************************[ screen 1 ]****************************************** ws2812
#if 1
int ui_scr1_get_led_mode(void) {
    return ws2812_get_mode();
}

void ui_scr1_set_color(lv_color32_t c32) {
    CRGB c;
    c.red = c32.ch.red;
    c.green = c32.ch.green;
    c.blue = c32.ch.blue;
    ws2812_set_color(c);
}

void ui_scr1_set_light(uint8_t light) {
    ws2812_set_light(light);
}

void ui_scr1_set_mode(int mode) {
    ws2812_set_mode(mode);
    if(mode == 0){
        vTaskSuspend(ws2812_handle);
    } else {
        vTaskResume(ws2812_handle);
    }
}
#endif
//************************************[ screen 2 ]****************************************** lora
#if 1

#endif
//************************************[ screen 3 ]****************************************** nfc
#if 1

#endif
//************************************[ screen 4 ]****************************************** setting
#if 1
bool ui_scr4_get_lora_st(void) { 
    return lora_is_init();
}
bool ui_scr4_get_pmu_st(void) {
    
}
bool ui_scr4_get_nfc_st(void) {
    return nfc_is_init();
}
bool ui_scr4_get_sd_st(void) {
    return sd_is_valid();
}
#endif
//************************************[ screen 5 ]****************************************** battery
#if 1

#endif
//************************************[ screen 6 ]****************************************** wifi
#if 1

#endif
//************************************[ screen 7 ]****************************************** other(IR、MIC、SD)

// --------------------- screen 7.1 --------------------- IR

// --------------------- screen 7.2 --------------------- MIC

// --------------------- screen 7.3 --------------------- SD

// --------------------- screen 7 -----------------------