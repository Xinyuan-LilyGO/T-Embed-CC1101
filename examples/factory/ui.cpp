
#include "ui.h"
int display_rotation = 3;

/**
 * screen 0: 
*/

//************************************[ screen 0 ]****************************************** menu
#if 1
lv_obj_t *item_cont;
lv_obj_t *menu_cont;
lv_obj_t *menu_icon;
lv_obj_t *menu_icon_lab;

int fouce_item = 0;
const char *name_buf[] = { "Lora", "WS2812", "NFC", "Battery",  "Device",
                           "Rotation" };

void entry0_anim(void);
void switch_scr0_anim(int user_data);

// event
static void scr0_btn_event_cb(lv_event_t * e)
{   
    int data = (int)e->user_data;

    if(e->code == LV_EVENT_CLICKED){
        fouce_item = data;
        switch (fouce_item)
        {
            case 0: switch_scr0_anim(SCREEN2_ID); break; // name_buf[0]: lora
            case 1: switch_scr0_anim(SCREEN1_ID); break; // name_buf[1]: WS2812
            case 2: switch_scr0_anim(SCREEN3_ID); break; // name_buf[1]: nfc
            case 3: switch_scr0_anim(SCREEN4_ID); break;
            case 4: switch_scr0_anim(SCREEN4_ID); break;
            case 5:
                if(display_rotation == 3){
                    // tft.fillScreen(TFT_BLACK);
                    tft.setRotation(1);
                    display_rotation = 1;
                } else if(display_rotation == 1){
                    // tft.fillScreen(TFT_BLACK);
                    tft.setRotation(3);
                    display_rotation = 3;
                }
                break;
            default:
                break;
        }
    }

    if(e->code == LV_EVENT_FOCUSED){
        switch (data)
        { 
            case 0: lv_img_set_src(menu_icon, &Battery_Charging_48); break; // Battery 
            case 1: lv_img_set_src(menu_icon, &Canada_Flag); break;        // Battery 
            case 2: lv_img_set_src(menu_icon, &China_Flag); break;             // Battery 
            case 3: lv_img_set_src(menu_icon, &European_Union_Flag); break;        // Battery 
            case 4: lv_img_set_src(menu_icon, &United_Kingdom_Flag); break;        // Battery 
            case 5: lv_img_set_src(menu_icon, &US_Outlying_Islands_Flag); break;        // Battery 

            default:
                break;
        }
        lv_label_set_text(menu_icon_lab, name_buf[data]);
    }
}

void entry0_anim(void)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, item_cont);
    lv_anim_set_values(&a, DISPALY_WIDTH*0.6, 0);
    lv_anim_set_time(&a, 400);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&a, [](void * var, int32_t v){
        lv_obj_set_x((lv_obj_t *)var, v);
        lv_port_indev_enabled(false);
    });
    lv_anim_set_ready_cb(&a, [](struct _lv_anim_t *a){
        lv_port_indev_enabled(true);
    });
    lv_anim_start(&a);

    lv_anim_set_time(&a, 400);
    lv_anim_set_var(&a, menu_cont);
    lv_anim_set_values(&a, -DISPALY_WIDTH*0.4, 0);
    lv_anim_set_exec_cb(&a, [](void * var, int32_t v){
        lv_obj_set_x((lv_obj_t *)var, v);
        lv_port_indev_enabled(false);
    });
    lv_anim_start(&a);
}

void switch_scr0_anim(int user_data)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, item_cont);
    lv_anim_set_values(&a, 0, DISPALY_WIDTH*0.6);
    lv_anim_set_time(&a, 400);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&a, [](void * var, int32_t v){
        lv_obj_set_x((lv_obj_t *)var, v);
        lv_port_indev_enabled(false);
    });
    lv_anim_set_ready_cb(&a, [](struct _lv_anim_t *a){
        scr_mgr_switch((int)a->user_data, false);
        lv_port_indev_enabled(true);
    });
    lv_anim_set_user_data(&a, (void *)user_data);
    lv_anim_start(&a);

    lv_anim_set_time(&a, 400);
    lv_anim_set_var(&a, menu_cont);
    lv_anim_set_values(&a, 0, -DISPALY_WIDTH*0.4);
    lv_anim_set_exec_cb(&a, [](void * var, int32_t v){
        lv_obj_set_x((lv_obj_t *)var, v);
        lv_port_indev_enabled(false);
    });
    lv_anim_start(&a);
}

static void scroll_event_cb(lv_event_t * e)
{
    lv_obj_t * cont = lv_event_get_target(e);

    lv_area_t cont_a;
    lv_obj_get_coords(cont, &cont_a);
    lv_coord_t cont_y_center = cont_a.y1 + lv_area_get_height(&cont_a) / 2;

    lv_coord_t r = lv_obj_get_height(cont) * 7 / 10;
    uint32_t i;
    uint32_t child_cnt = lv_obj_get_child_cnt(cont);
    for(i = 0; i < child_cnt; i++) {
        lv_obj_t * child = lv_obj_get_child(cont, i);
        lv_area_t child_a;
        lv_obj_get_coords(child, &child_a);

        lv_coord_t child_y_center = child_a.y1 + lv_area_get_height(&child_a) / 2;

        lv_coord_t diff_y = child_y_center - cont_y_center;
        diff_y = LV_ABS(diff_y);

        /*Get the x of diff_y on a circle.*/
        lv_coord_t x;
        /*If diff_y is out of the circle use the last point of the circle (the radius)*/
        if(diff_y >= r) {
            x = r;
        }
        else {
            x =  (diff_y * 0.866);
        }

        /*Translate the item by the calculated X coordinate*/
        lv_obj_set_style_translate_x(child, x, 0);

        /*Use some opacity with larger translations*/
        lv_opa_t opa = lv_map(x, 0, r, LV_OPA_TRANSP, LV_OPA_COVER);
        lv_obj_set_style_opa(child, LV_OPA_COVER - opa, 0);
    }
}

void create0(lv_obj_t *parent)
{
    menu_cont = lv_obj_create(parent);
    lv_obj_set_size(menu_cont, DISPALY_WIDTH*0.4, DISPALY_HEIGHT);
    lv_obj_set_scrollbar_mode(menu_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_align(menu_cont, LV_ALIGN_LEFT_MID);
    lv_obj_set_style_bg_color(menu_cont, lv_color_hex(COLOR_BG), LV_PART_MAIN);
    lv_obj_set_style_pad_all(menu_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(menu_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(menu_cont, 0, LV_PART_MAIN);

    menu_icon = lv_img_create(menu_cont);
    // lv_img_set_src(menu_icon, &Battery_Charging_48);
    lv_obj_align(menu_icon, LV_ALIGN_CENTER, 5, 0);
    
    menu_icon_lab = lv_label_create(menu_cont);
    lv_obj_set_width(menu_icon_lab, DISPALY_WIDTH*0.4);
    lv_obj_set_style_text_align(menu_icon_lab, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(menu_icon_lab, lv_color_hex(COLOR_TEXT), LV_PART_MAIN);
    // lv_label_set_text(menu_icon_lab, "battery");
    lv_obj_align_to(menu_icon_lab, menu_cont, LV_ALIGN_BOTTOM_MID, 5, -25);
    
    lv_obj_t *menu_lab = lv_label_create(menu_cont);
    lv_obj_set_width(menu_lab, DISPALY_WIDTH*0.4);
    lv_obj_align(menu_lab, LV_ALIGN_TOP_MID, 5, 25);
    lv_obj_set_style_text_align(menu_lab, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    // lv_obj_set_style_text_font(menu_lab, &lv_font_montserrat_26, LV_PART_MAIN);
    lv_label_set_recolor(menu_lab, true);                      /*Enable re-coloring by commands in the text*/
    lv_label_set_text_fmt(menu_lab, "#EE781F %02d# #ffffff :# #D8E699 %02d#", 18, 59);

    ///////////////////////////////////////////////////////
    item_cont = lv_obj_create(parent);
    lv_obj_set_size(item_cont, DISPALY_WIDTH*0.6, DISPALY_HEIGHT);
    lv_obj_set_align(item_cont, LV_ALIGN_RIGHT_MID);
    lv_obj_set_flex_flow(item_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_color(item_cont, lv_color_hex(COLOR_BG), LV_PART_MAIN);
    lv_obj_set_style_border_width(item_cont, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(item_cont, scroll_event_cb, LV_EVENT_SCROLL, NULL);
    lv_obj_set_style_radius(item_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_clip_corner(item_cont, true, 0);
    lv_obj_set_scroll_dir(item_cont, LV_DIR_VER);
    lv_obj_set_scroll_snap_y(item_cont, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scrollbar_mode(item_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_row(item_cont, 10, LV_PART_MAIN);

    int buf_len = sizeof(name_buf)/sizeof(name_buf[0]);

    uint32_t i;
    for(i = 0; i < buf_len; i++) {
        lv_obj_t * btn = lv_btn_create(item_cont);
        lv_obj_set_width(btn, lv_pct(100));
        lv_obj_set_style_radius(btn, 10, LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn, lv_color_hex(COLOR_BG), LV_PART_MAIN);
        lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
        lv_obj_remove_style(btn, NULL, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(btn, 2, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_color(btn, lv_color_hex(COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_bg_img_opa(btn, LV_OPA_100, LV_PART_MAIN);
        lv_obj_add_event_cb(btn, scr0_btn_event_cb, LV_EVENT_CLICKED, (void *)i);
        lv_obj_add_event_cb(btn, scr0_btn_event_cb, LV_EVENT_FOCUSED, (void *)i);

        lv_obj_t * label = lv_label_create(btn);
        lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), LV_PART_MAIN);
        lv_label_set_text(label, name_buf[i]);
    }

    /*Update the buttons position manually for first*/
    lv_event_send(item_cont, LV_EVENT_SCROLL, NULL);

    /*Be sure the fist button is in the middle*/
    lv_obj_scroll_to_view(lv_obj_get_child(item_cont, 0), LV_ANIM_OFF);

    lv_group_focus_obj(lv_obj_get_child(item_cont, fouce_item));

    lv_group_set_wrap(lv_group_get_default(), false);
}

void entry0(void) {   
    entry0_anim();
}
void exit0(void) {}
void destroy0(void) {}

scr_lifecycle_t screen0 = {
    .create = create0,
    .entry = entry0,
    .exit  = exit0,
    .destroy = destroy0,
};
#endif
//************************************[ screen 1 ]****************************************** ws2812
#if 1
lv_obj_t *scr1_cont;
lv_obj_t *light_acr;
int ws2812_light = 10;
lv_color_t ws2812_color ={.full = 0xF800};

void entry1_anim(lv_obj_t *obj);
void exit1_anim(int user_data, lv_obj_t *obj);

void colorwheel_focus_event(lv_event_t *e)
{
    lv_obj_t *tgt = lv_event_get_target(e);
    lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
    if(e->code == LV_EVENT_CLICKED){
        if (!lv_obj_has_flag(tgt, LV_OBJ_FLAG_CHECKABLE)) {
            lv_obj_clear_flag(tgt, LV_OBJ_FLAG_CHECKABLE);
            lv_group_set_editing(lv_group_get_default(), false);
        } else {
            lv_obj_add_flag(tgt, LV_OBJ_FLAG_CHECKABLE);
            lv_group_set_editing(lv_group_get_default(), true);
        }
    }
    else if(e->code == LV_EVENT_VALUE_CHANGED){
        ws2812_color = lv_colorwheel_get_rgb(tgt);
        CRGB c;
        lv_color32_t c32;
        c32.full = lv_color_to32(ws2812_color);
        c.red = c32.ch.red;
        c.green = c32.ch.green;
        c.blue = c32.ch.blue;
        if(ws2812_get_mode() == 0){
            ws2812_set_color(c);
        }
        lv_label_set_text_fmt(label, "0x%02X%02X%02X", c.red, c.green, c.blue);
        lv_obj_set_style_bg_color(light_acr, ws2812_color, LV_PART_KNOB);
        lv_obj_set_style_arc_color(light_acr, ws2812_color, LV_PART_INDICATOR);
        // lv_msg_send(MSG_WS2812_COLOR, &colorwheel_buf);
        // printf("full=0x%x, r=0x%x, g=0x%x, b=0x%x\n",c.full, c.ch.red, c.ch.green, c.ch.blue);
    }
}

static void value_changed_event_cb(lv_event_t * e)
{
    lv_obj_t * arc = lv_event_get_target(e);
    lv_obj_t * label = (lv_obj_t *)lv_event_get_user_data(e);

    ws2812_light = lv_arc_get_value(arc);
    lv_label_set_text_fmt(label, "%d", ws2812_light);
    ws2812_set_light(ws2812_light);
    
    /*Rotate the label to the current position of the arc*/
    // lv_arc_rotate_obj_to_angle(arc, label, 25);
}

static void scr1_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        // scr_mgr_set_anim(LV_SCR_LOAD_ANIM_FADE_OUT, -1, -1);
        // scr_mgr_switch(SCREEN0_ID, false);
        exit1_anim(SCREEN0_ID, scr1_cont);
    }
}

static void ws2812_mode_event_cb(lv_event_t * e)
{
    static int mode = 0;
    lv_obj_t *tgt = (lv_obj_t *)e->target;
    lv_obj_t *lab = (lv_obj_t *)e->user_data;
    if(e->code == LV_EVENT_CLICKED){
        mode++;
        mode &= 0x3;
        switch (mode)
        {
            case 0: lv_label_set_text(lab, " OFF ");    break;
            case 1: lv_label_set_text(lab, " demo1 "); break;
            case 2: lv_label_set_text(lab, " demo2 "); break;
            case 3: lv_label_set_text(lab, " demo3 "); break;
            default:
                break;
        }
        ws2812_set_mode(mode);
        if(mode == 0){
            vTaskSuspend(ws2812_handle);
        } else {
            vTaskResume(ws2812_handle);
        }
    }
}

void entry1_anim(lv_obj_t *obj)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, LV_OPA_0, LV_OPA_100);
    lv_anim_set_time(&a, 200);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_set_exec_cb(&a, [](void * var, int32_t v){ 
        lv_port_indev_enabled(false);
        lv_obj_set_style_opa((lv_obj_t *)var, v, LV_PART_MAIN);
    });
    lv_anim_set_ready_cb(&a, [](_lv_anim_t *a){ 
        lv_port_indev_enabled(true);
    });
    lv_anim_start(&a);
}

void exit1_anim(int user_data, lv_obj_t *obj)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, LV_OPA_100, LV_OPA_0);
    lv_anim_set_time(&a, 200);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_set_exec_cb(&a, [](void * var, int32_t v){ 
        lv_port_indev_enabled(false);
        lv_obj_set_style_opa((lv_obj_t *)var, v, LV_PART_MAIN);
    });
    lv_anim_set_ready_cb(&a, [](_lv_anim_t *a){
        scr_mgr_switch((int)a->user_data, false);
        lv_port_indev_enabled(true);
    });
    lv_anim_set_user_data(&a, (void *)user_data);
    lv_anim_start(&a);
}

void create1(lv_obj_t *parent)
{   
    scr1_cont = lv_obj_create(parent);
    lv_obj_set_size(scr1_cont, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(scr1_cont, lv_color_hex(COLOR_BG), LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(scr1_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(scr1_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scr1_cont, 0, LV_PART_MAIN);

    lv_obj_t *colorwheel = lv_colorwheel_create(scr1_cont, true);
    lv_obj_set_size(colorwheel, 100, 100);
    lv_obj_set_style_arc_width(colorwheel, 6, LV_PART_MAIN);
    lv_obj_set_style_arc_width(colorwheel, 6, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(colorwheel, 6, LV_PART_KNOB);
    lv_obj_align(colorwheel, LV_ALIGN_RIGHT_MID, -35, 0);
    lv_colorwheel_set_rgb(colorwheel, ws2812_color);
    lv_obj_set_style_outline_pad(colorwheel, 4, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_radius(colorwheel, LV_RADIUS_CIRCLE, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(colorwheel, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(colorwheel, lv_color_hex(COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_group_add_obj(lv_group_get_default(), colorwheel);
    lv_obj_t * label = lv_label_create(colorwheel);
    lv_obj_center(label);
    lv_label_set_text(label, "color");
    lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), LV_PART_MAIN);
    lv_obj_add_event_cb(colorwheel, colorwheel_focus_event, LV_EVENT_CLICKED, label);
    lv_obj_add_event_cb(colorwheel, colorwheel_focus_event, LV_EVENT_VALUE_CHANGED, label);
    // lv_event_send(colorwheel, LV_EVENT_VALUE_CHANGED, NULL);

    light_acr = lv_arc_create(scr1_cont);
    lv_obj_set_size(light_acr, 100, 100);
    lv_obj_align(light_acr, LV_ALIGN_LEFT_MID, 35, 0);
    lv_obj_set_style_bg_color(light_acr, ws2812_color, LV_PART_KNOB);
    lv_obj_set_style_arc_color(light_acr, ws2812_color, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(light_acr, 6, LV_PART_MAIN);
    lv_obj_set_style_arc_width(light_acr, 10, LV_PART_INDICATOR);
    lv_arc_set_rotation(light_acr, 90);
    lv_arc_set_bg_angles(light_acr, 0, 360);
    lv_arc_set_value(light_acr, ws2812_light);
    lv_arc_set_range(light_acr, 0, 255);
    lv_obj_set_style_outline_pad(light_acr, 4, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_radius(light_acr, LV_RADIUS_CIRCLE, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(light_acr, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(light_acr, lv_color_hex(COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_group_add_obj(lv_group_get_default(), light_acr);
    lv_obj_t * label1 = lv_label_create(light_acr);
    lv_obj_center(label1);
    lv_obj_set_style_text_color(label1, lv_color_hex(COLOR_TEXT), LV_PART_MAIN);
    lv_obj_add_event_cb(light_acr, value_changed_event_cb, LV_EVENT_VALUE_CHANGED, label1);

    /*Manually update the label for the first time*/
    lv_event_send(light_acr, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t * mode_btn = lv_btn_create(scr1_cont);
    // lv_group_add_obj(lv_group_get_default(), mode_btn);
    lv_obj_set_style_pad_all(mode_btn, 0, 0);
    lv_obj_set_height(mode_btn, 30);
    lv_obj_align(mode_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    // lv_obj_set_style_border_color(mode_btn, lv_color_hex(COLOR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(mode_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(mode_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(mode_btn, lv_color_hex(COLOR_BG), LV_PART_MAIN);
    lv_obj_remove_style(mode_btn, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(mode_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(mode_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(mode_btn, lv_color_hex(COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_t * mode_lab = lv_label_create(mode_btn);
    lv_obj_align(mode_lab, LV_ALIGN_LEFT_MID, 0, 0);
    if(ws2812_get_mode() == 0) {
        lv_label_set_text(mode_lab, " OFF ");
    } else {
        lv_label_set_text_fmt(mode_lab, " demo%d ", ws2812_get_mode());
    }
    lv_obj_add_event_cb(mode_btn, ws2812_mode_event_cb, LV_EVENT_CLICKED, mode_lab);
    lv_event_send(mode_btn, LV_EVENT_VALUE_CHANGED, mode_lab);

    lv_obj_t * btn = lv_btn_create(scr1_cont);
    // lv_group_add_obj(lv_group_get_default(), btn);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_set_height(btn, 20);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 8, 8);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(COLOR_BG), LV_PART_MAIN);
    lv_obj_remove_style(btn, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(btn, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(btn, lv_color_hex(COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(btn, scr1_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label2 = lv_label_create(btn);
    lv_obj_align(label2, LV_ALIGN_LEFT_MID, 0, 0);
    lv_label_set_text(label2, " " LV_SYMBOL_LEFT " ");
}
void entry1(void) {
    entry1_anim(scr1_cont);
    lv_group_set_wrap(lv_group_get_default(), true);
}
void exit1(void) {
    lv_group_set_wrap(lv_group_get_default(), false);
}
void destroy1(void) {}

scr_lifecycle_t screen1 = {
    .create = create1,
    .entry = entry1,
    .exit = exit1,
    .destroy = destroy1,
};
#endif
//************************************[ screen 2 ]****************************************** lora
#if 1
lv_obj_t *scr2_cont;
lv_obj_t *lora_kb;
lv_obj_t *lora_recv_ta;
lv_obj_t *lora_recv_lab;
lv_obj_t *lora_send_ta;
lv_obj_t *lora_send_lab;
lv_obj_t *lora_mode_btn;
lv_timer_t *lora_recv_timer;
int lora_recv_cnt = 0;
int lora_send_cnt = 0;

extern int lora_recv_success;
extern String lora_recv_str;

void entry2_anim(lv_obj_t *obj);
void exit2_anim(int user_data, lv_obj_t *obj);

void lora_recv_event(lv_timer_t *t)
{
    if(lora_recv_success) {
        lora_recv_success = 0;
        lora_recv_cnt++;
        lv_textarea_set_text(lora_recv_ta, " ");
        lv_label_set_text_fmt(lora_recv_lab, "R: %d", lora_recv_cnt);
        lv_textarea_add_text(lora_recv_ta, lora_recv_str.c_str());
    }
}

void mode_sw_event(lv_event_t *e)
{
    lv_obj_t *tgt =  (lv_obj_t *)e->target;
    lv_obj_t *data = (lv_obj_t *)e->user_data;
    
    if(e->code == LV_EVENT_CLICKED) {
        switch (lora_get_mode())
        {
            case LORA_MODE_SEND: lora_mode_sw(LORA_MODE_RECV); lv_label_set_text(data, "recv"); break;
            case LORA_MODE_RECV: lora_mode_sw(LORA_MODE_SEND); lv_label_set_text(data, "send"); break;
            default: break;
        }
    }
}

static void ta_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED || code == LV_EVENT_FOCUSED) {
        /*Focus on the clicked text area*/
        if(lora_kb != NULL) lv_keyboard_set_textarea(lora_kb, ta);
    }

    else if(code == LV_EVENT_READY) {
        // LV_LOG_USER("Ready, current text: %s", lv_textarea_get_text(ta));
        uint32_t max_length = lv_textarea_get_cursor_pos(ta);
        if(max_length != 0){
            if(lora_get_mode() == LORA_MODE_SEND){
                lora_send(lv_textarea_get_text(ta));
            } else {
                Serial.println("Current Lora mode is RECV!");
            }
            // lv_textarea_set_text(ta, " "); // clear textarea
            Serial.printf("mode=%d, %s\n", lora_get_mode(), lv_textarea_get_text(ta));
            lora_send_cnt++;
            lv_label_set_text_fmt(lora_send_lab, "S: %d", lora_send_cnt);
        }
    }
}

static void scr2_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        exit2_anim(SCREEN0_ID, scr2_cont);
    }
}

void entry2_anim(lv_obj_t *obj)
{
    entry1_anim(obj);
}

void exit2_anim(int user_data, lv_obj_t *obj)
{
    exit1_anim(user_data, obj);
}

void create2(lv_obj_t *parent){
    scr2_cont = lv_obj_create(parent);
    lv_obj_set_size(scr2_cont, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(scr2_cont, lv_color_hex(COLOR_BG), LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(scr2_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(scr2_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scr2_cont, 0, LV_PART_MAIN);

    lora_recv_ta = lv_textarea_create(scr2_cont);
    lv_obj_set_size(lora_recv_ta, 150, 60);
    lv_obj_set_style_border_width(lora_recv_ta, 1, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(lora_recv_ta, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(lora_recv_ta, lv_color_hex(COLOR_BG), LV_PART_MAIN);
    lv_obj_set_style_text_color(lora_recv_ta, lv_color_hex(COLOR_TEXT), LV_PART_MAIN);
    lv_obj_set_style_border_color(lora_recv_ta, lv_color_hex(COLOR_TEXT), LV_PART_CURSOR | LV_STATE_FOCUSED);
    lv_textarea_set_text(lora_recv_ta, "");
    lv_obj_align(lora_recv_ta, LV_ALIGN_TOP_LEFT, 6, 30);
    lv_textarea_set_placeholder_text(lora_recv_ta, "Hello");
    // lv_obj_add_event_cb(lora_recv_ta, ta_event_cb, LV_EVENT_ALL, NULL);
    lv_group_remove_obj(lora_recv_ta);

    /*Create a label and position it above the text box*/
    lora_recv_lab = lv_label_create(scr2_cont);
    lv_obj_set_style_text_color(lora_recv_lab, lv_color_hex(COLOR_TEXT), LV_PART_MAIN);
    lv_label_set_text_fmt(lora_recv_lab, "R: %d", lora_recv_cnt);
    lv_obj_align_to(lora_recv_lab, lora_recv_ta, LV_ALIGN_OUT_TOP_MID, 0, -5);

    lora_mode_btn = lv_btn_create(scr2_cont);
    lv_obj_set_height(lora_mode_btn, 20);
    lv_obj_set_style_shadow_width(lora_mode_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(lora_mode_btn, 0, LV_PART_MAIN);
    lv_obj_remove_style(lora_mode_btn, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(lora_mode_btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(lora_mode_btn, lv_color_hex(COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_align(lora_mode_btn, LV_ALIGN_TOP_MID, 0, 6);
    lv_obj_t *mode_lab = lv_label_create(lora_mode_btn);
    lv_obj_center(mode_lab);
    switch (lora_get_mode())
    {
        case LORA_MODE_SEND: lv_label_set_text(mode_lab, "send"); break;
        case LORA_MODE_RECV: lv_label_set_text(mode_lab, "recv"); break;
        default: break;
    }
    lv_obj_add_event_cb(lora_mode_btn, mode_sw_event, LV_EVENT_CLICKED, mode_lab);


    /*Create the one-line mode text area*/
    lora_send_ta = lv_textarea_create(scr2_cont);
    lv_obj_set_size(lora_send_ta, 150, 60);
    lv_obj_set_style_border_width(lora_send_ta, 1, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(lora_send_ta, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(lora_send_ta, lv_color_hex(COLOR_BG), LV_PART_MAIN);
    lv_obj_set_style_text_color(lora_send_ta, lv_color_hex(COLOR_TEXT), LV_PART_MAIN);
    lv_obj_set_style_border_color(lora_send_ta, lv_color_hex(COLOR_TEXT), LV_PART_CURSOR | LV_STATE_FOCUSED);
    lv_textarea_set_one_line(lora_send_ta, false);
    lv_textarea_set_password_mode(lora_send_ta, false);
    lv_obj_add_event_cb(lora_send_ta, ta_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_align(lora_send_ta, LV_ALIGN_TOP_RIGHT, -6, 30);
    lv_group_remove_obj(lora_send_ta);

    /*Create a label and position it above the text box*/
    lora_send_lab = lv_label_create(scr2_cont);
    lv_obj_set_style_text_color(lora_send_lab, lv_color_hex(COLOR_TEXT), LV_PART_MAIN);
    lv_label_set_text_fmt(lora_send_lab, "S: %d", lora_send_cnt);
    lv_obj_align_to(lora_send_lab, lora_send_ta, LV_ALIGN_OUT_TOP_MID, 0, -5);

    /*Create a keyboard*/
    lora_kb = lv_keyboard_create(scr2_cont);
    lv_obj_set_style_border_width(lora_kb, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(lora_kb, lv_color_hex(COLOR_BORDER), LV_PART_MAIN);
    lv_obj_remove_style(lora_kb, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(lora_kb, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(lora_kb, lv_color_hex(COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(lora_kb, lv_color_hex(COLOR_FOCUS_ON), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(lora_kb, lv_color_hex(COLOR_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_color(lora_kb, lv_color_hex(COLOR_BG), LV_PART_ITEMS);
    lv_obj_set_style_bg_color(lora_kb, lv_color_hex(COLOR_BG), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(lora_kb, lv_color_hex(COLOR_TEXT), LV_PART_ITEMS);
    lv_obj_set_style_text_color(lora_kb, lv_color_hex(COLOR_TEXT), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_size(lora_kb,  LV_HOR_RES, LV_VER_RES / 2 - 10);
    lv_keyboard_set_textarea(lora_kb, lora_send_ta); /*Focus it on one of the text areas to start*/

    lv_event_send(lora_send_ta, LV_EVENT_FOCUSED, NULL);

    lv_obj_t * btn = lv_btn_create(scr2_cont);
    // lv_group_add_obj(lv_group_get_default(), btn);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_set_height(btn, 20);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 8, 8);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(COLOR_BG), LV_PART_MAIN);
    lv_obj_remove_style(btn, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(btn, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(btn, lv_color_hex(COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(btn, scr2_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label2 = lv_label_create(btn);
    lv_obj_align(label2, LV_ALIGN_LEFT_MID, 0, 0);
    lv_label_set_text(label2, " " LV_SYMBOL_LEFT " ");
}
void entry2(void) {
    entry2_anim(scr2_cont);
    lv_group_set_wrap(lv_group_get_default(), true);
    lora_recv_timer = lv_timer_create(lora_recv_event, 50, NULL);
    vTaskResume(lora_handle);
}
void exit2(void) {
    lv_timer_del(lora_recv_timer);
    lora_recv_timer = NULL;
    lv_group_set_wrap(lv_group_get_default(), false);
    vTaskSuspend(lora_handle);
}
void destroy2(void) {
}
scr_lifecycle_t screen2 = {
    .create = create2,
    .entry = entry2,
    .exit = exit2,
    .destroy = destroy2,
};
#endif
//************************************[ screen 3 ]****************************************** nfc
#if 1
extern bool nfc_upd_falg;
extern uint32_t cardid;
extern uint8_t uid[];
lv_obj_t *scr3_cont;
lv_obj_t *list;
lv_obj_t *nfc_led;
lv_obj_t *nfc_ledlab;
lv_timer_t *nfc_chk_timer = NULL;
uint32_t nfc_id[10] = {0};
uint32_t nfc_recode_cnt = 0;

void list_item_event(lv_event_t *e)
{

}

void nfc_chk_timer_event(lv_timer_t *t)
{
    int is_exist = -1;
    static int led_flag = 0;
    if(nfc_upd_falg) {
        nfc_upd_falg = false;

        for(int i = 0; i < sizeof(nfc_id) / sizeof(nfc_id[0]); i++) {
            if(nfc_id[i] == cardid) {
                is_exist = i;
                break;
            }
        }

        if(is_exist != -1) {
            lv_label_set_text_fmt(nfc_ledlab, "%02d -", is_exist+1);
            if(led_flag){
                lv_led_on(nfc_led);
            }else{
                lv_led_off(nfc_led);
            }
            led_flag = !led_flag;
        } else {
            char buf[33];
            nfc_id[nfc_recode_cnt] = cardid;
            nfc_recode_cnt++;
            
            lv_snprintf(buf, 33, "%02d-UID-[%02X:%02X:%02X:%02X]-#%d", 
                nfc_recode_cnt, uid[0], uid[1], uid[2], uid[3], cardid);
            lv_obj_t *item = lv_list_add_btn(list, NULL, buf);
            lv_obj_set_style_bg_color(item, lv_color_hex(COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
            lv_obj_set_style_bg_color(item, lv_color_hex(COLOR_BG), LV_PART_MAIN);
            lv_obj_set_style_text_color(item, lv_color_hex(COLOR_TEXT), LV_PART_MAIN);
        }
    } 
    else {
        lv_led_off(nfc_led);
        lv_label_set_text(nfc_ledlab, "xx -");
    }
}

void entry3_anim(lv_obj_t *obj)
{
    entry1_anim(obj);
}

void exit3_anim(int user_data, lv_obj_t *obj)
{
    exit1_anim(user_data, obj);
}

static void scr3_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        // scr_mgr_set_anim(LV_SCR_LOAD_ANIM_FADE_OUT, -1, -1);
        // scr_mgr_switch(SCREEN0_ID, false);
        exit3_anim(SCREEN0_ID, scr3_cont);
    }
}

void create3(lv_obj_t *parent){
    scr3_cont = lv_obj_create(parent);
    lv_obj_set_size(scr3_cont, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(scr3_cont, lv_color_hex(COLOR_BG), LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(scr3_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(scr3_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scr3_cont, 0, LV_PART_MAIN);

    lv_obj_t *lable = lv_label_create(scr3_cont);
    lv_obj_set_style_text_color(lable, lv_color_hex(COLOR_TEXT), LV_PART_MAIN);
    lv_obj_align(lable, LV_ALIGN_TOP_MID, 0, 5);

    uint32_t verisondata = nfc_get_ver_data();
    uint32_t chip = (verisondata >> 24) & 0xFF;
    uint32_t verH = (verisondata >> 16) & 0xFF;
    uint32_t verL = (verisondata >>  8) & 0xFF;
    lv_label_set_text_fmt(lable, "PN5%x  ver:%d.%d", chip, verH, verL);

    nfc_led  = lv_led_create(scr3_cont);
    lv_obj_set_size(nfc_led, 16, 16);
    lv_obj_align(nfc_led, LV_ALIGN_TOP_RIGHT, -10, 5);
    lv_led_off(nfc_led);

    nfc_ledlab = lv_label_create(scr3_cont);
    lv_obj_set_style_text_color(nfc_ledlab, lv_color_hex(COLOR_TEXT), LV_PART_MAIN);
    lv_obj_align_to(nfc_ledlab, nfc_led, LV_ALIGN_OUT_LEFT_MID, -6, 0);
    lv_label_set_text(nfc_ledlab, "xx -");

    list = lv_list_create(scr3_cont);
    lv_obj_set_size(list, LV_HOR_RES, 135);
    lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(list, lv_color_hex(COLOR_BG), LV_PART_MAIN);
    lv_obj_set_style_pad_row(list, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(list, 0, LV_PART_MAIN);
    // lv_obj_set_style_outline_pad(list, 2, LV_PART_MAIN);
    lv_obj_set_style_border_width(list, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(list, 0, LV_PART_MAIN);

    for(int i = 0; i < nfc_recode_cnt; i++) {
        char buf[33];
        lv_snprintf(buf, 33, "%02d-UID-[%02X:%02X:%02X:%02X]-#%d", 
                i + 1, 
                (nfc_id[i] >> 24) & 0xFF, 
                (nfc_id[i] >> 16) & 0xFF, 
                (nfc_id[i] >> 8)  & 0xFF, 
                nfc_id[i] & 0xFF, 
                nfc_id[i]);
        lv_obj_t *item = lv_list_add_btn(list, NULL, buf);
        lv_obj_set_style_bg_color(item, lv_color_hex(COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_bg_color(item, lv_color_hex(COLOR_BG), LV_PART_MAIN);
        lv_obj_set_style_text_color(item, lv_color_hex(COLOR_TEXT), LV_PART_MAIN);
        // lv_obj_add_event_cb(item, list_item_event, LV_EVENT_CLICKED, (void *)i);
    }

    lv_obj_t * btn = lv_btn_create(scr3_cont);
    // lv_group_add_obj(lv_group_get_default(), btn);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_set_height(btn, 20);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 8, 8);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(COLOR_BG), LV_PART_MAIN);
    lv_obj_remove_style(btn, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(btn, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(btn, lv_color_hex(COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(btn, scr3_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label2 = lv_label_create(btn);
    lv_obj_align(label2, LV_ALIGN_LEFT_MID, 0, 0);
    lv_label_set_text(label2, " " LV_SYMBOL_LEFT " ");
}
void entry3(void) {
    entry3_anim(scr3_cont);
    nfc_chk_timer = lv_timer_create(nfc_chk_timer_event, 100, NULL);
    vTaskResume(nfc_handle);
}
void exit3(void) {
    lv_timer_del(nfc_chk_timer);
    nfc_chk_timer = NULL;
    vTaskSuspend(nfc_handle);
}
void destroy3(void) {
}
scr_lifecycle_t screen3 = {
    .create = create3,
    .entry = entry3,
    .exit = exit3,
    .destroy = destroy3,
};
#endif
//************************************[ screen 4 ]******************************************
#if 1

lv_obj_t *scr4_cont;

void entry4_anim(lv_obj_t *obj)
{
    entry1_anim(obj);
}

void exit4_anim(int user_data, lv_obj_t *obj)
{
    exit1_anim(user_data, obj);
}

static void scr4_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        // scr_mgr_set_anim(LV_SCR_LOAD_ANIM_FADE_OUT, -1, -1);
        // scr_mgr_switch(SCREEN0_ID, false);
        exit4_anim(SCREEN0_ID, scr4_cont);
    }
}

void create4(lv_obj_t *parent){
    scr4_cont = lv_obj_create(parent);
    lv_obj_set_size(scr4_cont, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(scr4_cont, lv_color_hex(COLOR_BG), LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(scr4_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(scr4_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scr4_cont, 0, LV_PART_MAIN);

    lv_obj_t * btn = lv_btn_create(scr4_cont);
    // lv_group_add_obj(lv_group_get_default(), btn);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_set_height(btn, 20);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 8, 8);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(COLOR_BG), LV_PART_MAIN);
    lv_obj_remove_style(btn, NULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(btn, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(btn, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(btn, lv_color_hex(COLOR_FOCUS_ON), LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(btn, scr4_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label2 = lv_label_create(btn);
    lv_obj_align(label2, LV_ALIGN_LEFT_MID, 0, 0);
    lv_label_set_text(label2, " " LV_SYMBOL_LEFT " ");
}
void entry4(void) {
    entry4_anim(scr4_cont);
    vTaskResume(battery_handle);
}
void exit4(void) {
    vTaskSuspend(battery_handle);
}
void destroy4(void) {
}
scr_lifecycle_t screen4 = {
    .create = create4,
    .entry = entry4,
    .exit = exit4,
    .destroy = destroy4,
};
#endif

//************************************[ UI ENTRY ]******************************************
void ui_entry(void)
{
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    scr_mgr_init();
    scr_mgr_set_bg_color(COLOR_BG);
    scr_mgr_register(SCREEN0_ID, &screen0);
    scr_mgr_register(SCREEN1_ID, &screen1);
    scr_mgr_register(SCREEN2_ID, &screen2);
    scr_mgr_register(SCREEN3_ID, &screen3);
    scr_mgr_register(SCREEN4_ID, &screen4);

    scr_mgr_switch(SCREEN0_ID, false); // main scr
}