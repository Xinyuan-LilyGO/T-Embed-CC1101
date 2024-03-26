
#include "ui.h"

//************************************[ screen 0 ]******************************************

#if 1
lv_obj_t *item_cont;
lv_obj_t *menu_cont;
lv_obj_t *menu_icon;
lv_obj_t *menu_icon_lab;

int fouce_item = 0;
const char *name_buf[] = { "Battery", "Setting", "System",  "Device",  "WS2812", 
                           "Button1" };

void entry_anim(void);
void switch_scr_anim(int user_data);

// event
static void scr0_btn_event_cb(lv_event_t * e)
{   
    int data = (int)e->user_data;

    if(e->code == LV_EVENT_CLICKED){
        switch_scr_anim(SCREEN1_ID);
        fouce_item = data;
    }

    if(e->code == LV_EVENT_FOCUSED){
        switch (data)
        { 
            case 0: lv_img_set_src(menu_icon, &Battery_Charging_48); break; // Battery 
            case 1: lv_img_set_src(menu_icon, &Canada_Flag); break;        // Battery 
            case 2: lv_img_set_src(menu_icon, &China_Flag); break;        // Battery 
            case 3: lv_img_set_src(menu_icon, &European_Union_Flag); break;        // Battery 
            case 4: lv_img_set_src(menu_icon, &United_Kingdom_Flag); break;        // Battery 
            case 5: lv_img_set_src(menu_icon, &US_Outlying_Islands_Flag); break;        // Battery 

            default:
                break;
        }
        lv_label_set_text(menu_icon_lab, name_buf[data]);
    }
}

static void anim_x_cb(void * var, int32_t v) {
    lv_obj_set_x((lv_obj_t *)var, v);
    indev_enabled(false);
}

static void anim_x1_cb(void * var, int32_t v) {
    lv_obj_set_x((lv_obj_t *)var, v);
}

static void anim_ready_cb(struct _lv_anim_t *a){
    indev_enabled(true);
}

static void switch_scr_anim_ready_cb(struct _lv_anim_t *a){
    indev_enabled(true);
    scr_mgr_switch((int)a->user_data, false);
}

void entry_anim(void)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, item_cont);
    lv_anim_set_values(&a, DISPALY_WIDTH*0.6, 0);
    lv_anim_set_time(&a, 600);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&a, anim_x_cb);
    lv_anim_set_ready_cb(&a, anim_ready_cb);
    lv_anim_start(&a);

    lv_anim_set_time(&a, 400);
    lv_anim_set_var(&a, menu_cont);
    lv_anim_set_values(&a, -DISPALY_WIDTH*0.4, 0);
    lv_anim_set_exec_cb(&a, anim_x1_cb);
    lv_anim_start(&a);
}

void switch_scr_anim(int user_data)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, item_cont);
    lv_anim_set_values(&a, 0, DISPALY_WIDTH*0.6);
    lv_anim_set_time(&a, 500);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&a, anim_x_cb);
    lv_anim_set_ready_cb(&a, switch_scr_anim_ready_cb);
    lv_anim_set_user_data(&a, (void *)user_data);
    lv_anim_start(&a);

    lv_anim_set_time(&a, 500);
    lv_anim_set_var(&a, menu_cont);
    lv_anim_set_values(&a, 0, -DISPALY_WIDTH*0.4);
    lv_anim_set_exec_cb(&a, anim_x1_cb);
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
    lv_obj_center(menu_icon);
    
    menu_icon_lab = lv_label_create(menu_cont);
    lv_obj_set_width(menu_icon_lab, DISPALY_WIDTH*0.4);
    lv_obj_set_style_text_align(menu_icon_lab, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(menu_icon_lab, lv_color_hex(COLOR_TETX), LV_PART_MAIN);
    // lv_label_set_text(menu_icon_lab, "battery");
    lv_obj_align_to(menu_icon_lab, menu_icon, LV_ALIGN_OUT_BOTTOM_MID, 0, 30);
    
    lv_obj_t *menu_lab = lv_label_create(menu_cont);
    lv_obj_set_width(menu_lab, DISPALY_WIDTH*0.4);
    lv_obj_align(menu_lab, LV_ALIGN_TOP_MID, 0, 25);
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
        lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TETX), LV_PART_MAIN);
        lv_label_set_text(label, name_buf[i]);
    }

    /*Update the buttons position manually for first*/
    lv_event_send(item_cont, LV_EVENT_SCROLL, NULL);

    /*Be sure the fist button is in the middle*/
    lv_obj_scroll_to_view(lv_obj_get_child(item_cont, 0), LV_ANIM_OFF);

    lv_group_focus_obj(lv_obj_get_child(item_cont, fouce_item));

    // lv_group_set_wrap(lv_group_get_default(), false);
}

void entry0(void) {   
    entry_anim();
}
void exit0(void) {    printf("exit0\n");}
void destroy0(void) { printf("destroy0\n");}

scr_lifecycle_t screen0 = {
    .create = create0,
    .entry = entry0,
    .exit  = exit0,
    .destroy = destroy0,
};
#endif
//************************************[ screen 1 ]******************************************
#if 1
static void scr1_btn_event_cb(lv_event_t * e)
{
    if(e->code == LV_EVENT_CLICKED){
        scr_mgr_switch(SCREEN0_ID, false);
    }
}

void create1(lv_obj_t *parent)
{   
    
    lv_obj_t * btn = lv_btn_create(parent);
    // lv_group_add_obj(lv_group_get_default(), btn);
    lv_obj_set_align(btn, LV_ALIGN_TOP_MID);
    lv_obj_add_event_cb(btn, scr1_btn_event_cb, LV_EVENT_CLICKED, NULL);

    btn = lv_btn_create(parent);
    // lv_group_add_obj(lv_group_get_default(), btn);
    lv_obj_set_align(btn, LV_ALIGN_CENTER);
    lv_obj_add_event_cb(btn, scr1_btn_event_cb, LV_EVENT_CLICKED, NULL);

    btn = lv_btn_create(parent);
    // lv_group_add_obj(lv_group_get_default(), btn);
    lv_obj_set_align(btn, LV_ALIGN_BOTTOM_MID);
    lv_obj_add_event_cb(btn, scr1_btn_event_cb, LV_EVENT_CLICKED, NULL);
}
void entry1(void) {}
void exit1(void) {}
void destroy1(void) {
}

scr_lifecycle_t screen1 = {
    .create = create1,
    .entry = entry1,
    .exit = exit1,
    .destroy = destroy1,
};
#endif
//************************************[ screen 2 ]******************************************
void ui_entry(void)
{
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    // lv_demo_benchmark_set_max_speed(true);
    // lv_demo_benchmark();

    scr_mgr_init();
    scr_mgr_set_bg_color(COLOR_BG);
    scr_mgr_register(SCREEN0_ID, &screen0);
    scr_mgr_register(SCREEN1_ID, &screen1);

    scr_mgr_switch(SCREEN0_ID, false);

    // lv_example_scroll();
}