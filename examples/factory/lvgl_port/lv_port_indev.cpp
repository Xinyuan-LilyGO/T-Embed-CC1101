/**
 * @file lv_port_indev.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_indev.h"
#include "lvgl.h"
#include "utilities.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
RotaryEncoder encoder(ENCODER_INA, ENCODER_INB, RotaryEncoder::LatchMode::TWO03);

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void encoder_init(void);
static void encoder_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);
static void encoder_handler(void);

/**********************
 *  STATIC VARIABLES
 **********************/
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
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
/*------------------
 * Encoder
 * -----------------*/
volatile bool indev_encoder_enabled = true;
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
    // static int pos = 0;
    // int new_pos = encoder.getPosition();
   

    if(indev_encoder_enabled){
        int encoder_dir = (int16_t)encoder.getDirection();
        if(encoder_dir != 0){
            data->enc_diff = -encoder_dir;
        }

        if (digitalRead(ENCODER_KEY) == LOW) {
            data->state = LV_INDEV_STATE_PR;
        } else if (digitalRead(ENCODER_KEY) == HIGH) { 
            data->state = LV_INDEV_STATE_REL;
        }
    } else {
        data->enc_diff = 0;
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

void indev_enabled(bool en)
{
    indev_encoder_enabled = en;
}


