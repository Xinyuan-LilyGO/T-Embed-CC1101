/**
 * @file lv_port_indev.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "port_indev.h"
#include "lvgl.h"
#include "utilities.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
RotaryEncoder encoder(ENCODER_INA, ENCODER_INB, RotaryEncoder::LatchMode::TWO03);

volatile bool indev_encoder_enabled = true;
volatile bool indev_keypad_enabled = true;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void keypad_init(void);
static void keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);
static uint32_t keypad_get_key(void);

static void encoder_init(void);
static void encoder_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);
static void encoder_handler(void);

/**********************
 *  STATIC VARIABLES
 **********************/
lv_indev_t * indev_keypad;
lv_indev_t * indev_encoder;

static int32_t encoder_diff;
static lv_indev_state_t encoder_state;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_indev_init(void)
{
    static lv_indev_drv_t indev_drv;
    static lv_indev_drv_t indev_drv_keypad;


    /*------------------
     * Encoder
     * -----------------*/

    /*Initialize your encoder if you have*/
    encoder_init();

    /*Register a encoder input device*/
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_ENCODER;
    indev_drv.read_cb = encoder_read;
    indev_encoder = lv_indev_drv_register(&indev_drv);
    lv_group_t *group = lv_group_create();
    lv_indev_set_group(indev_encoder, group);
    lv_group_set_default(group);

    /*Later you should create group(s) with `lv_group_t * group = lv_group_create()`,
     *add objects to the group with `lv_group_add_obj(group, obj)`
     *and assign this input device to group to navigate in it:
     *`lv_indev_set_group(indev_encoder, group);`*/

    /*------------------
     * Keypad
     * -----------------*/

    /*Initialize your keypad or keyboard if you have*/
    keypad_init();

    /*Register a keypad input device*/
    lv_indev_drv_init(&indev_drv_keypad);
    indev_drv_keypad.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv_keypad.read_cb = keypad_read;
    indev_keypad = lv_indev_drv_register(&indev_drv_keypad);
    // lv_group_t *keypad_group = lv_group_create();
    lv_indev_set_group(indev_keypad, group);

    /*Later you should create group(s) with `lv_group_t * group = lv_group_create()`,
     *add objects to the group with `lv_group_add_obj(group, obj)`
     *and assign this input device to group to navigate in it:
     *`lv_indev_set_group(indev_keypad, group);`*/

}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*------------------
 * Keypad
 * -----------------*/
extern uint16_t infared_get_cmd(void);

/*Initialize your keypad*/
static void keypad_init(void)
{
    /*Your code comes here*/
}

/*Will be called by the library to read the mouse*/
static void keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    static uint32_t last_key = 0;

    if(indev_keypad_enabled == false) {
        uint32_t act_key = infared_get_cmd();
        if(act_key == 0x44) {
            act_key = LV_KEY_ENTER;
            data->key = last_key;
            data->state = LV_INDEV_STATE_PR;
        } else {
            data->state = LV_INDEV_STATE_REL;
        }
        return;
    }

    uint32_t act_key = infared_get_cmd();
    if(act_key != 0) {
        data->state = LV_INDEV_STATE_PR;

        /*Translate the keys to LVGL control characters according to your key definitions*/
        switch(act_key) {
            case 0x41:
                act_key = LV_KEY_NEXT;
                break;
            case 0x40:
                act_key = LV_KEY_PREV;
                break;
            case 0x7:
                act_key = LV_KEY_LEFT;
                break;
            case 0x6:
                act_key = LV_KEY_RIGHT;
                break;
            case 0x44:
                act_key = LV_KEY_ENTER;
                break;
        }

        last_key = act_key;
    }
    else {
        data->state = LV_INDEV_STATE_REL;
    }

    data->key = last_key;
}

/*Get the currently being pressed key.  0 if no key is pressed*/
static uint32_t keypad_get_key(void)
{
    /*Your code comes here*/

    return 0;
}
/*------------------
 * Encoder
 * -----------------*/
int indev_encoder_pos = 0;
/*Initialize your keypad*/
static void encoder_init(void)
{
    /*Your code comes here*/
    pinMode(ENCODER_KEY, INPUT);
    attachInterrupt(ENCODER_INA, encoder_handler, CHANGE);
    attachInterrupt(ENCODER_INB, encoder_handler, CHANGE);
}

/*Will be called by the library to read the encoder*/
static void encoder_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    if(indev_encoder_enabled){
        indev_encoder_pos= encoder.getPosition();
        int encoder_dir = (int16_t)encoder.getDirection();
        if(encoder_dir != 0){
            data->enc_diff = -encoder_dir;
        }
    } else {
        data->enc_diff = 0;
        data->state = LV_INDEV_STATE_REL;
    }

    if (digitalRead(ENCODER_KEY) == LOW) {
        data->state = LV_INDEV_STATE_PR;
    } else if (digitalRead(ENCODER_KEY) == HIGH) { 
        data->state = LV_INDEV_STATE_REL;
    }
    // Serial.printf("enc_diff:%d, sta:%d\n",data->enc_diff, data->state);
}

/*Call this function in an interrupt to process encoder events (turn, press)*/
static void encoder_handler(void)
{
    /*Your code comes here*/
    if(indev_encoder_enabled) {
        encoder.tick();
    }
}

void lv_port_indev_enabled(bool en)
{
    indev_encoder_enabled = en;
    indev_keypad_enabled = en;
}

int lv_port_indev_get_pos(void)
{
    return indev_encoder_pos;
}


