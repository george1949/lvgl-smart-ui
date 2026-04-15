#include "ui_pages.h"

/* Home page component references */

static lv_obj_t *create_card(lv_obj_t *parent, int width, const char *title, const char *text1, const char *text2)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, width, 180);
    lv_obj_set_style_border_color(card, lv_color_hex(0x9C5B9A), 0); // Purple border
    lv_obj_set_style_border_width(card, 5, 0);
    lv_obj_set_style_radius(card, 15, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xFFFFFF), 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    if (text1) {
        lv_obj_t *lbl1 = lv_label_create(card);
        lv_label_set_text(lbl1, text1);
        lv_obj_align(lbl1, LV_ALIGN_TOP_LEFT, 0, 10);
        lv_obj_set_style_text_font(lbl1, &lv_font_montserrat_24, 0);
    }
    
    if (text2) {
        lv_obj_t *lbl2 = lv_label_create(card);
        lv_label_set_text(lbl2, text2);
        lv_obj_align(lbl2, LV_ALIGN_BOTTOM_MID, 0, -10);
        lv_obj_set_style_text_font(lbl2, &lv_font_montserrat_20, 0);
    }

    return card;
}

/* 初始化主页 */
void init_home_page(lv_obj_t *parent)
{
    // 设置主背景
    lv_obj_set_style_bg_color(parent, lv_color_hex(0xF4F4F7), 0); // 改为浅灰色背景，更有质感
    lv_obj_set_style_pad_all(parent, 0, 0);

    // --- 1. 创建顶部时间日期显示框 ---
    lv_obj_t *time_date_box = lv_obj_create(parent);
    lv_obj_set_size(time_date_box, 600, 110); 
    lv_obj_align(time_date_box, LV_ALIGN_TOP_MID, 0, 30); // 距离顶部 30px
    lv_obj_set_style_bg_color(time_date_box, lv_color_hex(0xDCD0E9), 0);
    lv_obj_set_style_border_color(time_date_box, lv_color_hex(0x9C5B9A), 0);
    lv_obj_set_style_border_width(time_date_box, 3, 0);
    lv_obj_set_style_radius(time_date_box, 15, 0);
    lv_obj_set_style_shadow_width(time_date_box, 10, 0); // 增加一点阴影
    lv_obj_set_style_shadow_opa(time_date_box, 40, 0);
    lv_obj_set_scrollbar_mode(time_date_box, LV_SCROLLBAR_MODE_OFF);

    // 时间显示
    home_time_label = lv_label_create(time_date_box);
    lv_label_set_text(home_time_label, "12:30");
    lv_obj_align(home_time_label, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_text_font(home_time_label, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(home_time_label, lv_color_hex(0x1D1D1F), 0);

    // 日期显示
    home_date_label = lv_label_create(time_date_box);
    lv_label_set_text(home_date_label, "2026-3-7");
    lv_obj_align(home_date_label, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_style_text_font(home_date_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(home_date_label, lv_color_hex(0x666666), 0);

    // --- 1.5. 创建警告图标（Home页面专属）---
    lv_obj_t *alert_icon = lv_label_create(parent);
    lv_label_set_text(alert_icon, LV_SYMBOL_WARNING); // Exclamation mark icon
    lv_obj_align(alert_icon, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_set_style_text_color(alert_icon, lv_color_hex(0xA15B9A), 0);
    lv_obj_set_style_text_font(alert_icon, &lv_font_montserrat_32, 0);
    lv_obj_set_style_bg_opa(alert_icon, LV_OPA_TRANSP, 0);
    // 开启点击属性
    lv_obj_add_flag(alert_icon, LV_OBJ_FLAG_CLICKABLE);
    // 绑定点击回调函数
    lv_obj_add_event_cb(alert_icon, alert_icon_click_event, LV_EVENT_CLICKED, NULL);
    // 添加按下时的视觉反馈
    lv_obj_set_style_text_color(alert_icon, lv_color_hex(0xFF6B6B), LV_STATE_PRESSED);
    lv_obj_set_style_transform_zoom(alert_icon, 243, LV_STATE_PRESSED); // 缩放0.95倍
    // 提升层级到顶层，确保能被点击
    lv_obj_move_foreground(alert_icon);

    // --- 2. 底部功能卡片弹性容器 ---
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 760, 220); 
    lv_obj_set_style_bg_opa(cont, 0, 0);     // 容器透明，只显示里面的卡片
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, -20); // 距离底部 20px
    
    // 设置弹性布局
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);

    // --- 3. Card 1: Smart Light ---
    lv_obj_t *light_card = lv_obj_create(cont);
    lv_obj_set_size(light_card, 210, 160);
    lv_obj_set_style_border_color(light_card, lv_color_hex(0x1396CE), 0);
    lv_obj_set_style_border_width(light_card, 2, 0);
    lv_obj_set_style_radius(light_card, 12, 0);
    lv_obj_clear_flag(light_card, LV_OBJ_FLAG_SCROLLABLE); // 禁用卡片内部滚动

    lv_obj_t *light_title = lv_label_create(light_card);
    lv_label_set_text(light_title, "Smart Light");
    lv_obj_align(light_title, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_text_font(light_title, &lv_font_montserrat_14, 0);

    lv_obj_t *light_icon = lv_label_create(light_card);
    lv_label_set_text(light_icon, "LIGHT");
    lv_obj_align(light_icon, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(light_icon, lv_color_hex(0xFCE616), 0);

    home_light_label = lv_label_create(light_card);
    // 初始化时检查是否所有灯光都关闭
    bool all_off = true;
    for (int i = 0; i < light_device_count; i++) {
        if (light_devices[i].status) {
            all_off = false;
            break;
        }
    }
    light_status = !all_off;
    lv_label_set_text(home_light_label, light_status ? "ON" : "OFF");
    lv_obj_align(home_light_label, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_text_color(home_light_label, light_status ? lv_color_hex(0xFCE616) : lv_color_hex(0xCCCCCC), 0);
    
    // 将 home_light_label 保存到第一个灯光设备的 home_label 字段
    if (light_device_count > 0) {
        light_devices[0].home_label = home_light_label;
    }
    
    // 初始化智能灯光汇总标签
    menu_smart_light_summary.title = light_title;
    menu_smart_light_summary.value = home_light_label;

    // --- 4. Card 2: Sweep Robot ---
    lv_obj_t *robot_card = lv_obj_create(cont);
    lv_obj_set_size(robot_card, 210, 160);
    lv_obj_set_style_border_color(robot_card, lv_color_hex(0x1396CE), 0);
    lv_obj_set_style_border_width(robot_card, 2, 0);
    lv_obj_set_style_radius(robot_card, 12, 0);
    lv_obj_clear_flag(robot_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *robot_title = lv_label_create(robot_card);
    lv_label_set_text(robot_title, "Sweep Robot");
    lv_obj_align(robot_title, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_text_font(robot_title, &lv_font_montserrat_14, 0);

    lv_obj_t *robot_icon = lv_label_create(robot_card);
    lv_label_set_text(robot_icon, "ROBOT");
    lv_obj_align(robot_icon, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(robot_icon, lv_color_hex(0x1396CE), 0);

    home_robot_label = lv_label_create(robot_card);
    lv_label_set_text(home_robot_label, "STOP");
    lv_obj_align(home_robot_label, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_text_color(home_robot_label, lv_color_hex(0xFF0000), 0);

    // --- 5. Card 3: Mode ---
    lv_obj_t *mode_card = lv_obj_create(cont);
    lv_obj_set_size(mode_card, 210, 160);
    lv_obj_set_style_border_color(mode_card, lv_color_hex(0x1396CE), 0);
    lv_obj_set_style_border_width(mode_card, 2, 0);
    lv_obj_set_style_radius(mode_card, 12, 0);
    lv_obj_clear_flag(mode_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *mode_title = lv_label_create(mode_card);
    lv_label_set_text(mode_title, "Mode");
    lv_obj_align(mode_title, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_text_font(mode_title, &lv_font_montserrat_14, 0);

    lv_obj_t *mode_icon = lv_label_create(mode_card);
    lv_label_set_text(mode_icon, "HOME");
    lv_obj_align(mode_icon, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(mode_icon, lv_color_hex(0x32CD32), 0);

    lv_obj_t *mode_status = lv_label_create(mode_card);
    lv_label_set_text(mode_status, "Bedroom");
    lv_obj_align(mode_status, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_text_color(mode_status, lv_color_hex(0x32CD32), 0);
}