/**
 * @file lv_port_disp.h
 *
 */

#ifndef LV_PORT_DISP_TEMPL_H
#define LV_PORT_DISP_TEMPL_H

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"
#include "TFT_eSPI.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
extern TFT_eSPI tft;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/* Initialize low level display driver */
void lv_port_disp_init(void);

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void);

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void);

/**********************
 *      MACROS
 **********************/

#endif /*LV_PORT_DISP_TEMPL_H*/

