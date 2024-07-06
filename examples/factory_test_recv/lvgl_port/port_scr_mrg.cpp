#include "port_scr_mrg.h"

/* 记录所有的屏幕卡片 */ 
scr_card_t *scr_mgr_head;
scr_card_t *scr_mgr_top;

/* 被调用屏幕栈 */
scr_card_t *scr_stack_root;
scr_card_t *scr_stack_top;

uint32_t scr_anim_time = SCR_MGR_ANIM_TIME; 
lv_scr_load_anim_t scr_anim_sw = SCR_MGR_SCR_SWITCH_ANIM;
lv_scr_load_anim_t scr_anim_push = SCR_MGR_SCR_PUSH_ANIM;
lv_scr_load_anim_t scr_anim_pop = SCR_MGR_SCR_POP_ANIM;

uint32_t default_bg_color = 0x000000;
/*********************************************************************************
 *                              STATIC FUNCTION
 *********************************************************************************/
static lv_obj_t *scr_mgr_default_style(scr_card_t *card)
{
/** color schemes
    text: #1e1e00
    bg:   #fffee6
    c1:   #49a6fd
    c2:   #fefdb3
    c3:   #8617fd
*/
    lv_obj_t *obj = lv_obj_create(NULL);
    lv_obj_set_size(obj, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(obj, lv_color_hex(default_bg_color), LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    // lv_obj_align(obj, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    // lv_obj_set_scroll_snap_x(obj, LV_SCROLL_SNAP_CENTER);
    // lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
    // lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);
    // lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    card->life->create(obj);
    return obj;
}

static scr_card_t *scr_mgr_find_by_id(int id) // 检查链表中没有没此 id 
{
    scr_card_t *p = scr_mgr_head->next;
    while(p != NULL){
        if(id == p->id)
            return p;
        p = p->next;
    }
    return NULL;
}

static scr_card_t *scr_mgr_get_top(scr_card_t *head) // 获取链表最前面的节点 
{
    scr_card_t *p = head;

    while(p->next != NULL){
        p = p->next;
    }
    return p;
}

static void scr_mgr_active(scr_card_t *card)  // 设置屏幕卡片为活跃状态 
{
    if(card->st == SCR_MGR_STATE_DESTROYED){
        // card->obj = card->life->create(NULL);
        card->obj = scr_mgr_default_style(card);
        card->life->entry();
        card->st = SCR_MGR_STATE_ACTIVE;
    } else if(card->st == SCR_MGR_STATE_CREATED || card->st == SCR_MGR_STATE_INACTIVE){
        card->life->entry();
        card->st = SCR_MGR_STATE_ACTIVE;
    }
}

static void scr_mgr_inactive(scr_card_t *card)
{
    if(card->st > SCR_MGR_STATE_INACTIVE){
        card->life->exit();
        card->st = SCR_MGR_STATE_INACTIVE;
    }
}

static void scr_mgr_remove(scr_card_t *card)
{
    if(card->st > SCR_MGR_STATE_INACTIVE){
        card->life->exit();
        card->life->destroy();
        card->st = SCR_MGR_STATE_IDLE;
    } else if(card->st > SCR_MGR_STATE_DESTROYED){
        card->life->destroy();
        card->st = SCR_MGR_STATE_IDLE;
    }
}

/*********************************************************************************
 *                              GLOBAL FUNCTION
 *********************************************************************************/
void scr_mgr_init(void)
{
    scr_mgr_head = (scr_card_t *)lv_mem_alloc(sizeof(scr_card_t));
    scr_mgr_head->id = -1;
    scr_mgr_head->obj = NULL;
    scr_mgr_head->st = SCR_MGR_STATE_IDLE;
    scr_mgr_head->life = NULL;
    scr_mgr_head->next = NULL;
    scr_mgr_head->prev = NULL;
    
    scr_mgr_top = scr_mgr_head;

    scr_stack_root = NULL;
    scr_stack_top = NULL;
}

bool scr_mgr_register(int id, scr_lifecycle_t *card_life) // 注册一个屏幕卡片 
{
    if(scr_mgr_find_by_id(id)){
        // printf("scr already registered\n");
        return false;
    }

    scr_card_t *new_card = (scr_card_t *)lv_mem_alloc(sizeof(scr_card_t));
    new_card->id = id;
    new_card->obj = NULL;
    new_card->st = SCR_MGR_STATE_IDLE;
    new_card->life = card_life;
    new_card->next = NULL;
    new_card->prev = scr_mgr_top;

    scr_mgr_top->next = new_card;
    scr_mgr_top = new_card;

    return true;
}

bool scr_mgr_switch(int id, bool anim)  // 清空栈，然后切换到指定 id的屏幕卡片上
{
    scr_card_t *tgt_card = scr_mgr_find_by_id(id);
    scr_card_t *stack_scr = NULL;
    lv_obj_t *curr_obj = NULL;

    if(tgt_card == NULL) // 没有找到该屏幕
        return false;

    if(scr_stack_top != NULL) { // 如果有多张屏幕卡片叠在一起，就先记录顶层卡片
        curr_obj = scr_stack_top->obj;
        stack_scr = scr_stack_top->prev;
        scr_mgr_remove(scr_stack_top);
        lv_mem_free((void *)scr_stack_top);
        scr_stack_top = stack_scr;
    }
    while(scr_stack_top != NULL) { // 然后清除所有卡片
        stack_scr = scr_stack_top->prev;
        curr_obj = scr_stack_top->obj;
        scr_mgr_remove(scr_stack_top);
        lv_mem_free((void *)scr_stack_top);
        scr_stack_top = stack_scr;
    }

    stack_scr = (scr_card_t *)lv_mem_alloc(sizeof(scr_card_t));
    stack_scr->id = tgt_card->id;
    // stack_scr->obj = tgt_card->life->create(NULL);
    stack_scr->obj = scr_mgr_default_style(tgt_card);
    stack_scr->st = SCR_MGR_STATE_CREATED;
    stack_scr->life = tgt_card->life;
    stack_scr->prev = NULL;
    stack_scr->next = NULL;
    scr_stack_root = stack_scr;
    scr_stack_top = stack_scr;

    scr_mgr_active(stack_scr); // 设置屏幕卡片为活跃状态

    if(scr_anim_sw != LV_SCR_LOAD_ANIM_NONE && anim){
        lv_scr_load_anim(stack_scr->obj, scr_anim_sw, scr_anim_time, 0, true);
    } else{
        lv_scr_load(stack_scr->obj);
        if(curr_obj)
            lv_obj_del(curr_obj);
    }
    return true;
}

bool scr_mgr_push(int id, bool anim)
{
    scr_card_t *tgt_card = scr_mgr_find_by_id(id);
    scr_card_t *stack_scr = NULL;

    if(tgt_card == NULL){ // 没有找到该屏幕
        return false;
    }

    if((scr_stack_top != NULL) && (tgt_card->id == scr_stack_top->id)){ // 当前push屏幕和最顶层屏幕一样
        return false;
    }

    stack_scr = (scr_card_t *)lv_mem_alloc(sizeof(scr_card_t));
    stack_scr->id = tgt_card->id;
    // stack_scr->obj = tgt_card->life->create(NULL);
    stack_scr->obj = scr_mgr_default_style(tgt_card);
    stack_scr->st = SCR_MGR_STATE_CREATED;
    stack_scr->life = tgt_card->life;
    if(scr_stack_top == NULL){
        stack_scr->prev = NULL;
        stack_scr->next = NULL;
        scr_stack_root = stack_scr;
        scr_stack_top = stack_scr;
    } else{
        scr_mgr_inactive(scr_stack_top);
        scr_stack_top->next = stack_scr;
        stack_scr->prev = scr_stack_top;
        scr_stack_top->next = NULL;
        scr_stack_top = stack_scr;
    }

    scr_mgr_active(stack_scr);

    if(scr_anim_push != LV_SCR_LOAD_ANIM_NONE && anim){
        lv_scr_load_anim(stack_scr->obj, scr_anim_push, scr_anim_time, 0, false);
    } else{
        lv_scr_load(stack_scr->obj);
    }
    return true;
}

bool scr_mgr_pop(bool anim)
{
    scr_card_t *dst_item = NULL;
    lv_obj_t *cur_obj;

    if(scr_stack_top == NULL || scr_stack_top == scr_stack_root){
        return false;
    }

    cur_obj = scr_stack_top->obj;
    dst_item = scr_stack_top->prev;
    scr_mgr_remove(scr_stack_top);
    lv_mem_free((void *)scr_stack_top);
    scr_stack_top = dst_item;

    scr_mgr_active(dst_item);

    if(scr_anim_pop != LV_SCR_LOAD_ANIM_NONE && anim){
        lv_scr_load_anim(dst_item->obj, scr_anim_pop, scr_anim_time, 0, true);
    } else{
        lv_scr_load(dst_item->obj);
        if (cur_obj) {
            lv_obj_del(cur_obj);
        }
    }
    return false;
}

// set anim
void scr_mgr_set_anim(lv_scr_load_anim_t sw, lv_scr_load_anim_t push, lv_scr_load_anim_t pop)
{
    if(sw >= LV_SCR_LOAD_ANIM_NONE && sw <= LV_SCR_LOAD_ANIM_OUT_BOTTOM){
        scr_anim_sw = sw;
    }
    if(push >= LV_SCR_LOAD_ANIM_NONE && push <= LV_SCR_LOAD_ANIM_OUT_BOTTOM){
        scr_anim_push = push;
    }
    if(pop >= LV_SCR_LOAD_ANIM_NONE && pop <= LV_SCR_LOAD_ANIM_OUT_BOTTOM){
        scr_anim_pop = pop;
    }
}

// set bg color
void scr_mgr_set_bg_color(uint32_t c)
{
    default_bg_color = c;
}


