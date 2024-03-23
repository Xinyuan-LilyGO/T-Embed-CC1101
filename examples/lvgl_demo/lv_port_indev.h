
/**
 * @file lv_port_indev.h
 *
 */

/*Copy this file as "lv_port_indev.h" and set this value to "1" to enable content*/
#ifndef LV_PORT_INDEV_TEMPL_H
#define LV_PORT_INDEV_TEMPL_H

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"
#include "RotaryEncoder.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
extern RotaryEncoder encoder;
/**********************
 * GLOBAL PROTOTYPES
 **********************/
void lv_port_indev_init(void);

/**********************
 *      MACROS
 **********************/

#endif /*LV_PORT_INDEV_TEMPL_H*/
