#include "ui_pages.h"
#include <stdio.h>

/* 灯光设备卡片点击事件处理函数 */
static void light_card_click_event(lv_event_t *e) {
    // 获取设备索引
    int device_index = (int)lv_event_get_user_data(e);
    
    // 切换设备状态
    bool new_state = !light_devices[device_index].status;
    update_light_state(device_index, new_state);
    
    // 同步机器人状态汇总
    update_robot_summary();
}

/* 初始化智能灯控制页面 */
/* 初始化智能灯控制页面 - 固定排版版 */
void init_light_page(lv_obj_t *parent)
{
    // 1. 基础页面样式：纯白背景，禁用滚动
    lv_obj_set_style_bg_color(parent, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_pad_all(parent, 0, 0);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    // 2. 创建主容器 (田字格布局)
    // 高度设置为 360，宽度 780，确保给底部导航栏留出足够空间
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 780, 360); 
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 15); // 顶部偏移 15px
    lv_obj_set_style_bg_opa(cont, 0, 0);        // 背景透明
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 5, 0);       // 内部边距微调
    
    // 设置 Flex 布局 (自动换行，形成 2x2)
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // 设置卡片之间的垂直间距 (row gap) 和水平间距 (column gap)
    lv_obj_set_style_pad_row(cont, 15, 0);    // 行间距
    lv_obj_set_style_pad_column(cont, 10, 0); // 列间距
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    // 3. 定义通用卡片样式
    static lv_style_t style_card;
    static lv_style_t style_card_pressed;
    if(style_card.prop_cnt == 0) { // 静态初始化，防止重复分配内存
        // 正常状态样式
        lv_style_init(&style_card);
        lv_style_set_border_color(&style_card, lv_color_hex(0x00A0E9)); // 亮蓝色边框
        lv_style_set_border_width(&style_card, 4);
        lv_style_set_radius(&style_card, 25);                          // 大圆角感
        lv_style_set_bg_color(&style_card, lv_color_hex(0xFFFFFF));
        // 增加轻微阴影，增强立体感
        lv_style_set_shadow_width(&style_card, 10);
        lv_style_set_shadow_opa(&style_card, 20);
        lv_style_set_shadow_ofs_y(&style_card, 5);
        
        // 按下状态样式
        lv_style_init(&style_card_pressed);
        lv_style_set_transform_zoom(&style_card_pressed, 243); // 缩放0.95倍（256 * 0.95 ≈ 243）
        lv_style_set_bg_color(&style_card_pressed, lv_color_hex(0xF0F0F0)); // 背景变暗
    }


    // 4. 循环创建 4 个灯光设备卡片
    for (int i = 0; i < light_device_count; i++) {
        // 创建卡片主体
        lv_obj_t *card = lv_obj_create(cont);
        lv_obj_set_size(card, 340, 155); // 调整尺寸：宽340，高155，确保垂直方向不溢出
        lv_obj_add_style(card, &style_card, 0);
        lv_obj_add_style(card, &style_card_pressed, LV_STATE_PRESSED);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE); // 禁用内容滚动

        // 状态显示 (左上角 ON/OFF)
        lv_obj_t *status_label = lv_label_create(card);
        lv_label_set_text(status_label, light_devices[i].status ? "ON" : "OFF");
        lv_obj_align(status_label, LV_ALIGN_TOP_LEFT, 0, 0);
        
        // 根据状态设置颜色：开(黄)，关(灰)
        lv_color_t color = light_devices[i].status ? lv_color_hex(0xFCE616) : lv_color_hex(0x888888);
        lv_obj_set_style_text_color(status_label, color, 0);
        lv_obj_set_style_text_font(status_label, &lv_font_montserrat_20, 0);

        // 设备名称 (底部居中)
        lv_obj_t *name_label = lv_label_create(card);
        lv_label_set_text(name_label, light_devices[i].name);
        lv_obj_align(name_label, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_text_font(name_label, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(name_label, lv_color_hex(0x1D1D1F), 0);

        // 为卡片添加点击事件
        lv_obj_add_event_cb(card, light_card_click_event, LV_EVENT_CLICKED, (void *)i);

        // 将 UI 对象句柄存入数组，方便后续手势逻辑更新状态
        light_devices[i].card = card;
        light_devices[i].card_label = status_label;
    }
}

/* 初始化智能灯设置子页面 (覆盖层) */
void init_light_settings(lv_obj_t *parent)
{
    // 不再创建提示画面，直接创建一个空的子页面
    light_sub_page = lv_obj_create(parent);
    lv_obj_set_size(light_sub_page, lv_pct(100), lv_pct(100));
    lv_obj_align(light_sub_page, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(light_sub_page, 0, 0); // 透明背景
    lv_obj_set_style_border_width(light_sub_page, 0, 0);
    
    /* 默认隐藏此覆盖层 */
    lv_obj_add_flag(light_sub_page, LV_OBJ_FLAG_HIDDEN);
}
