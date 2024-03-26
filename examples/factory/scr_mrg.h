#pragma once
#include "lvgl.h"
#include <Arduino.h>
#include "ui.h"

#define SCR_MGR_ANIM_TIME 500
#define SCR_MGR_SCR_SWITCH_ANIM    LV_SCR_LOAD_ANIM_NONE
#define SCR_MGR_SCR_PUSH_ANIM      LV_SCR_LOAD_ANIM_MOVE_LEFT
#define SCR_MGR_SCR_POP_ANIM       LV_SCR_LOAD_ANIM_MOVE_RIGHT

typedef enum scr_mgr_state {
    SCR_MGR_STATE_IDLE = 0,  /* Not in use */
    SCR_MGR_STATE_DESTROYED, /* Not active and having been destroyed */
    SCR_MGR_STATE_CREATED,   /* Created */
    SCR_MGR_STATE_INACTIVE,  /* Not active */
    SCR_MGR_STATE_ACTIVE_BG, /* Active but at the background */
    SCR_MGR_STATE_ACTIVE,    /* Active and at the foreground */
} scr_mgr_state_e;

typedef struct scr_lifecycle {
    void (*create)(lv_obj_t *parent);
    void (*entry)(void);
    void (*exit)(void);
    void (*destroy)(void);
} scr_lifecycle_t;

typedef struct scr_card {
    int id;
    lv_obj_t        *obj;
    scr_mgr_state_e  st;
    scr_lifecycle_t *life;
    struct scr_card *next;
    struct scr_card *prev;
} scr_card_t;

/*********************************************************************************
 *                              GLOBAL PROTOTYPES
 * *******************************************************************************/
void scr_mgr_init(void);
bool scr_mgr_register(int id, scr_lifecycle_t *card_life);
bool scr_mgr_switch(int id, bool anim);
bool scr_mgr_push(int id, bool anim);
bool scr_mgr_pop(bool anim);

// set anim
void scr_mgr_set_anim(lv_scr_load_anim_t sw, lv_scr_load_anim_t push, lv_scr_load_anim_t pop);
// set bg color
void scr_mgr_set_bg_color(uint32_t c);

