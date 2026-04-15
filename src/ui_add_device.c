#include "ui_add_device.h"
#include "ui_pages.h"

// 全局设备列表
device_t devices[MAX_DEVICES];
int device_count = 0;

// 全局UI对象
static lv_obj_t *add_device_page = NULL;
static lv_obj_t *name_input = NULL;
static lv_obj_t *id_input = NULL;
static lv_obj_t *keyboard = NULL;
static lv_obj_t *current_textarea = NULL;
static int selected_device_type = 0; // 0: 灯光, 1: 机器人, 2: 传感器

// 背景点击事件处理函数
static void bg_click_event_cb(lv_event_t *e)
{
    if(keyboard && !lv_obj_has_flag(keyboard, LV_OBJ_FLAG_HIDDEN)) {
        lv_keyboard_set_textarea(keyboard, NULL);      // 1. 断开绑定
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN); // 2. 隐藏键盘
        lv_obj_clear_state(lv_event_get_target(e), LV_STATE_FOCUSED); // 清除焦点
        current_textarea = NULL;
    }
}

// 输入框点击事件
static void textarea_click_event(lv_event_t *e)
{
    lv_obj_t *textarea = lv_event_get_target(e);
    
    if(keyboard) {
        lv_keyboard_set_textarea(keyboard, textarea);        // 绑定当前 TA
        lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN); // 显示键盘
        lv_obj_move_foreground(keyboard);               // 确保不被遮挡
        
        // 核心优化：自动聚焦到可视区域，防止被键盘遮挡
        lv_obj_scroll_to_view(textarea, LV_ANIM_ON);
        
        current_textarea = textarea;
    }
}

// 设备类型选择事件
static void device_type_click_event(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    int type = (int)lv_event_get_user_data(e);
    
    // 更新选中状态
    selected_device_type = type;
    
    // 重置所有按钮样式
    lv_obj_t *parent = lv_obj_get_parent(btn);
    lv_obj_t *child = lv_obj_get_child(parent, 0);
    while(child) {
        lv_obj_set_style_bg_color(child, lv_color_hex(0xF0F0F0), 0);
        lv_obj_set_style_text_color(child, lv_color_hex(0x333333), 0);
        child = lv_obj_get_child(parent, lv_obj_get_index(child) + 1);
    }
    
    // 设置选中按钮样式
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x1396CE), 0);
    lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFFFF), 0);
}

// 确认添加按钮点击事件
static void confirm_add_event(lv_event_t *e)
{
    if (device_count >= MAX_DEVICES) {
        // 显示设备数量上限提示
        lv_obj_t *msgbox = lv_msgbox_create(NULL, "Error", "Device limit reached (max 10)", NULL, true);
        lv_obj_center(msgbox);
        return;
    }
    
    // 获取输入框内容
    const char *name = lv_textarea_get_text(name_input);
    const char *id = lv_textarea_get_text(id_input);
    
    // 检查输入是否为空
    if (strlen(name) == 0 || strlen(id) == 0) {
        // 显示输入错误提示
        lv_obj_t *msgbox = lv_msgbox_create(NULL, "Error", "Please enter device name and ID", NULL, true);
        lv_obj_center(msgbox);
        return;
    }
    
    // 保存设备信息
    strncpy(devices[device_count].name, name, sizeof(devices[device_count].name) - 1);
    strncpy(devices[device_count].id, id, sizeof(devices[device_count].id) - 1);
    device_count++;
    
    // 显示添加成功提示
    lv_obj_t *msgbox = lv_msgbox_create(NULL, "Success", "Device added successfully", NULL, true);
    lv_obj_center(msgbox);
    
    // 清空输入框
    lv_textarea_set_text(name_input, "");
    lv_textarea_set_text(id_input, "");
    
    // 关闭键盘
    if (keyboard) {
        lv_keyboard_set_textarea(keyboard, NULL);
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    }
    current_textarea = NULL;
}

// 返回按钮点击事件
static void return_event(lv_event_t *e)
{
    // 关闭键盘
    if (keyboard) {
        lv_keyboard_set_textarea(keyboard, NULL);
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    }
    current_textarea = NULL;
    
    // 隐藏添加设备页面
    hide_add_device_page();
}

// 初始化添加设备页面
void init_add_device_page(lv_obj_t *parent)
{
    // 创建添加设备页面（作为遮罩层）
    add_device_page = lv_obj_create(parent);
    lv_obj_set_size(add_device_page, 800, 480);
    lv_obj_align(add_device_page, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(add_device_page, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(add_device_page, LV_OPA_70, 0);
    
    // 为页面添加背景点击事件
    lv_obj_add_event_cb(add_device_page, bg_click_event_cb, LV_EVENT_CLICKED, NULL);
    
    // 创建核心卡片容器
    lv_obj_t *card = lv_obj_create(add_device_page);
    lv_obj_set_size(card, 600, 400); // 调整高度以优化布局
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 25, 0);
    lv_obj_set_style_shadow_width(card, 20, 0);
    lv_obj_set_style_shadow_color(card, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_30, 0);
    lv_obj_set_style_shadow_ofs_y(card, 10, 0);
    
    // 彻底禁用卡片容器的滑动属性
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);
    
    // 创建顶部导航栏
    lv_obj_t *header = lv_obj_create(card);
    lv_obj_set_size(header, lv_pct(100), 60);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(header, 0, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    
    // 返回按钮
    lv_obj_t *back_btn = lv_btn_create(header);
    lv_obj_set_size(back_btn, 80, 30);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 15, 0);
    lv_obj_set_style_bg_opa(back_btn, 0, 0);
    lv_obj_set_style_border_width(back_btn, 0, 0);
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "BACK");
    lv_obj_center(back_label);
    lv_obj_set_style_text_color(back_label, lv_color_hex(0x333333), 0);
    lv_obj_add_event_cb(back_btn, return_event, LV_EVENT_CLICKED, NULL);
    
    // 标题
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Add New Device");
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x333333), 0);
    
    // 创建输入区域
    // 设备名称输入框
    lv_obj_t *name_group = lv_obj_create(card);
    lv_obj_set_size(name_group, lv_pct(90), 60);
    lv_obj_align(name_group, LV_ALIGN_TOP_MID, 0, 80); // 调整位置，紧凑型布局
    lv_obj_set_style_bg_opa(name_group, 0, 0);
    lv_obj_set_style_border_width(name_group, 0, 0);
    
    // 彻底禁用父容器的滑动属性
    lv_obj_clear_flag(name_group, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(name_group, LV_SCROLLBAR_MODE_OFF);
    
    // 水平布局
    lv_obj_set_flex_flow(name_group, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(name_group, LV_ALIGN_CENTER, LV_ALIGN_CENTER, LV_ALIGN_CENTER);
    
    // 图标
    lv_obj_t *name_icon = lv_label_create(name_group);
    lv_label_set_text(name_icon, LV_SYMBOL_EDIT);
    lv_obj_set_style_text_font(name_icon, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(name_icon, lv_color_hex(0x1396CE), 0);
    lv_obj_set_style_pad_right(name_icon, 15, 0);
    
    // 输入框
    name_input = lv_textarea_create(name_group);
    // 设置高度，宽度由 Flex 布局自动分配剩余空间
    lv_obj_set_height(name_input, 50);
    lv_obj_set_width(name_input, LV_SIZE_CONTENT); // 取消百分比宽度
    lv_obj_set_flex_grow(name_input, 1);           // 自动填充剩余空间
    
    // 设置设备名称输入框占位符
    lv_textarea_set_placeholder_text(name_input, "Device Name");
    // 强制单行模式，杜绝上下滑动条
    lv_textarea_set_one_line(name_input, true);
    
    lv_obj_set_style_radius(name_input, 10, 0);
    lv_obj_set_style_bg_color(name_input, lv_color_hex(0xF5F5F5), 0);
    lv_obj_set_style_border_width(name_input, 0, 0);
    // 移除文本框多余的内边距
    lv_obj_set_style_pad_all(name_input, 8, 0);    // 略微缩小内边距，确保文字不溢出
    // 禁用设备名称输入框的滑动
    lv_obj_clear_flag(name_input, LV_OBJ_FLAG_SCROLLABLE);
    // 彻底隐藏名称输入框滚动条
    lv_obj_set_scrollbar_mode(name_input, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(name_input, textarea_click_event, LV_EVENT_CLICKED, NULL);
    
    // 设备ID输入框
    lv_obj_t *id_group = lv_obj_create(card);
    lv_obj_set_size(id_group, lv_pct(90), 60);
    lv_obj_align(id_group, LV_ALIGN_TOP_MID, 0, 150); // 调整位置，紧凑型布局
    lv_obj_set_style_bg_opa(id_group, 0, 0);
    lv_obj_set_style_border_width(id_group, 0, 0);
    
    // 彻底禁用父容器的滑动属性
    lv_obj_clear_flag(id_group, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(id_group, LV_SCROLLBAR_MODE_OFF);
    
    // 水平布局
    lv_obj_set_flex_flow(id_group, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(id_group, LV_ALIGN_CENTER, LV_ALIGN_CENTER, LV_ALIGN_CENTER);
    
    // 图标
    lv_obj_t *id_icon = lv_label_create(id_group);
    lv_label_set_text(id_icon, LV_SYMBOL_USB);
    lv_obj_set_style_text_font(id_icon, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(id_icon, lv_color_hex(0x1396CE), 0);
    lv_obj_set_style_pad_right(id_icon, 15, 0);
    
    // 输入框
    id_input = lv_textarea_create(id_group);
    // 设置高度，宽度由 Flex 布局自动分配剩余空间
    lv_obj_set_height(id_input, 50);
    lv_obj_set_width(id_input, LV_SIZE_CONTENT); // 取消百分比宽度
    lv_obj_set_flex_grow(id_input, 1);           // 自动填充剩余空间
    
    // 设置设备ID输入框占位符
    lv_textarea_set_placeholder_text(id_input, "Device ID");
    // 强制单行模式，杜绝上下滑动条
    lv_textarea_set_one_line(id_input, true);
    
    lv_obj_set_style_radius(id_input, 10, 0);
    lv_obj_set_style_bg_color(id_input, lv_color_hex(0xF5F5F5), 0);
    lv_obj_set_style_border_width(id_input, 0, 0);
    // 移除文本框多余的内边距
    lv_obj_set_style_pad_all(id_input, 8, 0);    // 略微缩小内边距，确保文字不溢出
    // 禁用设备ID输入框的滑动
    lv_obj_clear_flag(id_input, LV_OBJ_FLAG_SCROLLABLE);
    // 彻底隐藏ID输入框滚动条
    lv_obj_set_scrollbar_mode(id_input, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(id_input, textarea_click_event, LV_EVENT_CLICKED, NULL);
    
    // 设备类型选择
    lv_obj_t *type_cont = lv_obj_create(card);
    lv_obj_set_size(type_cont, lv_pct(90), 60);
    lv_obj_align(type_cont, LV_ALIGN_TOP_MID, 0, 220); // 调整位置，紧凑型布局
    lv_obj_set_style_bg_opa(type_cont, 0, 0);
    lv_obj_set_style_border_width(type_cont, 0, 0);
    
    // 彻底禁用父容器的滑动属性
    lv_obj_clear_flag(type_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(type_cont, LV_SCROLLBAR_MODE_OFF);
    
    // 水平布局
    lv_obj_set_flex_flow(type_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(type_cont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // 灯光类型按钮
    lv_obj_t *light_btn = lv_btn_create(type_cont);
    lv_obj_set_size(light_btn, 120, 50);
    lv_obj_set_style_radius(light_btn, 10, 0);
    lv_obj_set_style_bg_color(light_btn, lv_color_hex(0x1396CE), 0);
    lv_obj_set_style_text_color(light_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_t *light_label = lv_label_create(light_btn);
    lv_label_set_text(light_label, "Light");
    lv_obj_center(light_label);
    lv_obj_add_event_cb(light_btn, device_type_click_event, LV_EVENT_CLICKED, (void *)0);
    
    // 机器人类型按钮
    lv_obj_t *robot_btn = lv_btn_create(type_cont);
    lv_obj_set_size(robot_btn, 120, 50);
    lv_obj_set_style_radius(robot_btn, 10, 0);
    lv_obj_set_style_bg_color(robot_btn, lv_color_hex(0xF0F0F0), 0);
    lv_obj_set_style_text_color(robot_btn, lv_color_hex(0x333333), 0);
    lv_obj_t *robot_label = lv_label_create(robot_btn);
    lv_label_set_text(robot_label, "Robot");
    lv_obj_center(robot_label);
    lv_obj_add_event_cb(robot_btn, device_type_click_event, LV_EVENT_CLICKED, (void *)1);
    
    // 传感器类型按钮
    lv_obj_t *sensor_btn = lv_btn_create(type_cont);
    lv_obj_set_size(sensor_btn, 120, 50);
    lv_obj_set_style_radius(sensor_btn, 10, 0); // 修复样式设置错误
    lv_obj_set_style_bg_color(sensor_btn, lv_color_hex(0xF0F0F0), 0);
    lv_obj_set_style_text_color(sensor_btn, lv_color_hex(0x333333), 0);
    lv_obj_t *sensor_label = lv_label_create(sensor_btn);
    lv_label_set_text(sensor_label, "Sensor");
    lv_obj_center(sensor_label);
    lv_obj_add_event_cb(sensor_btn, device_type_click_event, LV_EVENT_CLICKED, (void *)2);
    
    // 创建确认添加按钮
    lv_obj_t *confirm_btn = lv_btn_create(card);
    lv_obj_set_size(confirm_btn, lv_pct(80), 50);
    lv_obj_align(confirm_btn, LV_ALIGN_BOTTOM_MID, 0, -20); // 调整位置，增加与设备类型选择的间距
    lv_obj_set_style_radius(confirm_btn, 25, 0);
    lv_obj_set_style_bg_color(confirm_btn, lv_color_hex(0x1396CE), 0);
    lv_obj_set_style_text_color(confirm_btn, lv_color_hex(0xFFFFFF), 0);
    
    // 按下效果
    static lv_style_t style_btn_pressed;
    lv_style_init(&style_btn_pressed);
    lv_style_set_transform_zoom(&style_btn_pressed, 243); // 缩放0.95倍
    lv_obj_add_style(confirm_btn, &style_btn_pressed, LV_STATE_PRESSED);
    
    lv_obj_t *confirm_label = lv_label_create(confirm_btn);
    lv_label_set_text(confirm_label, "Add Device");
    lv_obj_center(confirm_label);
    lv_obj_add_event_cb(confirm_btn, confirm_add_event, LV_EVENT_CLICKED, NULL);
    
    // 创建键盘（单例模式）
    keyboard = lv_keyboard_create(lv_scr_act());
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_obj_set_size(keyboard, lv_pct(100), 200);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN); // 默认隐藏
    
    // 默认隐藏页面
    lv_obj_add_flag(add_device_page, LV_OBJ_FLAG_HIDDEN);
}

// 显示添加设备页面
void show_add_device_page(void)
{
    if (add_device_page) {
        lv_obj_clear_flag(add_device_page, LV_OBJ_FLAG_HIDDEN);
    }
}

// 隐藏添加设备页面
void hide_add_device_page(void)
{
    if (add_device_page) {
        lv_obj_add_flag(add_device_page, LV_OBJ_FLAG_HIDDEN);
    }
    
    // 关闭键盘
    if (keyboard) {
        lv_keyboard_set_textarea(keyboard, NULL);      // 强行脱离绑定
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN); // 强制隐藏
    }
    current_textarea = NULL;
    
    // 恢复底层页面的可点击状态
    if (living_room_layout) {
        lv_obj_add_flag(living_room_layout, LV_OBJ_FLAG_CLICKABLE);
    }
    if (bedroom_layout) {
        lv_obj_add_flag(bedroom_layout, LV_OBJ_FLAG_CLICKABLE);
    }
    
    // 重新启用手势识别
    gesture_enabled = true;
    
    // 清除UI忙碌标志
    pthread_mutex_unlock(&ui_mutex);
}
