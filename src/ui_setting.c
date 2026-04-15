#include "ui_pages.h"
#include "ui_add_device.h"
#include <time.h>

/* 外部变量声明 */
extern setting_device_t setting_devices[];
extern mode_id_t current_mode;
extern bool gesture_enabled;
extern lv_obj_t *setting_sub_page;
extern bool about_author_open;
extern lv_obj_t *living_room_cont;
extern lv_obj_t *bedroom_cont;
extern lv_obj_t *ui_background;
extern const lv_img_dsc_t background01;
extern const lv_img_dsc_t background02;

/* 全局UI对象 */
lv_obj_t *living_room_layout = NULL;
lv_obj_t *bedroom_layout = NULL;

/* 按钮点击事件处理函数 */
static void button_click_event(lv_event_t *e)
{
    // 获取按钮ID
    int btn_id = (int)lv_event_get_user_data(e);
    
    switch (btn_id) {
        case 0: // 起夜模式
            night_trip_mode();
            break;
        case 1: // 助眠倒计时
            start_sleep_timer();
            break;
        case 2: // 勿扰模式
            toggle_dnd_mode();
            break;
        case 3: // 切回客厅
            return_to_living_room_mode(); // 确保该函数内部执行了 lv_img_set_src(ui_background, &background01)
            switch_to_living_room_layout(); // 统一使用布局切换函数
            break;
    }
}

/* 模式选择卡片点击事件 */
static void mode_selection_click_event(lv_event_t *e)
{
    // 切换到卧室模式
    switch_to_bedroom_mode(); // 确保该函数内部执行了 lv_img_set_src(ui_background, &background02)
    switch_to_bedroom_layout(); // 统一使用布局切换函数
}

/* 消息框关闭事件回调函数 */
static void msgbox_close_event(lv_event_t *e) {
    gesture_enabled = true;
    // 清除UI忙碌标志
    pthread_mutex_unlock(&ui_mutex);
    
    // 恢复底层容器的可点击状态
    if (living_room_layout) {
        lv_obj_add_flag(living_room_layout, LV_OBJ_FLAG_CLICKABLE);
    }
    if (bedroom_layout) {
        lv_obj_add_flag(bedroom_layout, LV_OBJ_FLAG_CLICKABLE);
    }
}

/* 关于作者浮层关闭事件回调函数 */
static void about_panel_close_event(lv_event_t *e) {
    gesture_enabled = true;
    // 清除UI忙碌标志
    pthread_mutex_unlock(&ui_mutex);
    // 设置关于作者弹窗关闭标志
    about_author_open = false;
    
    // 恢复底层容器的可点击状态
    if (living_room_layout) {
        lv_obj_add_flag(living_room_layout, LV_OBJ_FLAG_CLICKABLE);
    }
    if (bedroom_layout) {
        lv_obj_add_flag(bedroom_layout, LV_OBJ_FLAG_CLICKABLE);
    }
}

/* 关于作者浮层点击事件处理函数 */
static void about_panel_click_event(lv_event_t *e) {
    lv_obj_t *panel = lv_event_get_target(e);
    lv_obj_del(panel);
}

/* 作者信息卡片点击事件 */
static void about_author_click_event(lv_event_t *e)
{
    // 创建2/3宽度的浮层
    lv_obj_t *about_panel = lv_obj_create(lv_scr_act());
    lv_obj_set_size(about_panel, 533, 350); // 宽度533px（屏幕的2/3），高度350px
    lv_obj_center(about_panel);
    
    // 设置样式
    lv_obj_set_style_bg_color(about_panel, lv_color_hex(0x000000), 0); // 黑色背景
    lv_obj_set_style_bg_opa(about_panel, LV_OPA_COVER, 0); // 不透明
    lv_obj_set_style_border_width(about_panel, 0, 0); // 无边框
    lv_obj_set_style_radius(about_panel, 15, 0); // 圆角15px
    
    // 创建内容
    lv_obj_t *content = lv_label_create(about_panel);
    lv_label_set_text(content, "Biography:\n\n" 
                         "Stay Hungry, Stay Foolish.\n" 
                         "I am a C developer, former college football player, specializing in thread pools and Socket programming.\n\n" 
                         "Technology stack:\n\n" 
                         "- Programming Languages: C/C++\n" 
                         "- Embedded Systems: Linux, RTOS\n" 
                         "- Graphics Library: LVGL\n" 
                         "- Hardware Platform: ARM, x86\n\n" 
                         "Instructions:\n\n" 
                         "1. Gesture Control: Swipe up, down, left and right, to switch pages.\n" 
                         "2. Mode Switching: Click Mode Selection to enter bedroom mode.\n" 
                         "3. Device Control: Click on the device card to go to the detailed settings.\n" 
                         "4. Night Mode: Click Night Trip to turn on in bedroom mode.\n" 
                         "5. Sleep Aid Mode: Click Sleep Timer to start the countdown in bedroom mode.\n");
    lv_obj_align(content, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(content, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(content, lv_color_white(), 0);
    lv_obj_set_width(content, 500); // 设置内容宽度，启用自动换行
    lv_label_set_long_mode(content, LV_LABEL_LONG_WRAP); // 启用自动换行
    
    // 禁用底层页面的手势识别，防止误触
    gesture_enabled = false;
    
    // 设置UI忙碌标志
    pthread_mutex_lock(&ui_mutex);
    
    // 设置关于作者弹窗打开标志
    about_author_open = true;
    
    // 将底层容器设置为不可点击
    if (living_room_layout) {
        lv_obj_clear_flag(living_room_layout, LV_OBJ_FLAG_CLICKABLE);
    }
    if (bedroom_layout) {
        lv_obj_clear_flag(bedroom_layout, LV_OBJ_FLAG_CLICKABLE);
    }
    
    // 为浮层添加点击事件，点击即销毁
    lv_obj_add_event_cb(about_panel, about_panel_click_event, LV_EVENT_CLICKED, NULL);
    
    // 为浮层添加删除事件，关闭时重新启用手势识别
    lv_obj_add_event_cb(about_panel, about_panel_close_event, LV_EVENT_DELETE, NULL);
}

/* Add smart home 按钮点击事件 */
static void add_smart_home_click_event(lv_event_t *e)
{
    // 显示添加设备页面
    show_add_device_page();
    
    // 禁用底层页面的手势识别，防止误触
    gesture_enabled = false;
    
    // 设置UI忙碌标志
    pthread_mutex_lock(&ui_mutex);
    
    // 将底层容器设置为不可点击
    if (living_room_layout) {
        lv_obj_clear_flag(living_room_layout, LV_OBJ_FLAG_CLICKABLE);
    }
    if (bedroom_layout) {
        lv_obj_clear_flag(bedroom_layout, LV_OBJ_FLAG_CLICKABLE);
    }
}

/* 更新数字时钟 */
static void update_digital_clock(lv_timer_t *timer)
{
    lv_obj_t *clock_label = (lv_obj_t *)timer->user_data;
    
    time_t now;
    struct tm *tm_info;
    char time_str[9];
    
    time(&now);
    tm_info = localtime(&now);
    
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);
    lv_label_set_text(clock_label, time_str);
}

/* 初始化卧室模式布局 */
static void init_bedroom_layout(lv_obj_t *parent)
{
    // 强制让卧室布局的父对象指向 ui_pages 里的容器
    // 不要直接用传进来的 parent (如果是屏幕的话)
    bedroom_layout = lv_obj_create(bedroom_cont);
    lv_obj_set_size(bedroom_layout, lv_pct(100), lv_pct(100));
    lv_obj_align(bedroom_layout, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(bedroom_layout, 0, 0); // 背景透明
    lv_obj_set_style_border_width(bedroom_layout, 0, 0); // 无边框
    lv_obj_set_style_shadow_width(bedroom_layout, 0, 0); // 无阴影
    
    // 设置Flex布局
    lv_obj_set_flex_flow(bedroom_layout, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bedroom_layout, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // 左侧功能操作区 (1/3)
    lv_obj_t *left_cont = lv_obj_create(bedroom_layout);
    lv_obj_set_size(left_cont, lv_pct(33), lv_pct(90));
    lv_obj_set_style_bg_opa(left_cont, 0, 0); // 背景透明
    lv_obj_set_style_border_width(left_cont, 0, 0);
    lv_obj_set_style_shadow_width(left_cont, 0, 0); // 无阴影
    
    // 设置垂直Flex布局
    lv_obj_set_flex_flow(left_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(left_cont, 20, 0);
    
    // 定义按钮样式
    static lv_style_t style_btn;
    static lv_style_t style_btn_pressed;
    if(style_btn.prop_cnt == 0) {
        // 正常状态样式
        lv_style_init(&style_btn);
        lv_style_set_border_color(&style_btn, lv_color_hex(0x1396CE)); // 蓝色边框
        lv_style_set_border_width(&style_btn, 3);
        lv_style_set_radius(&style_btn, 10);
        lv_style_set_bg_opa(&style_btn, 0); // 背景透明
        
        // 按下状态样式
        lv_style_init(&style_btn_pressed);
        lv_style_set_transform_zoom(&style_btn_pressed, 243); // 缩放0.95倍
        lv_style_set_bg_opa(&style_btn_pressed, 0); // 背景透明
    }
    
    // 起夜模式按钮
    lv_obj_t *night_trip_btn = lv_btn_create(left_cont);
    lv_obj_set_size(night_trip_btn, 200, 60);
    lv_obj_add_style(night_trip_btn, &style_btn, 0);
    lv_obj_add_style(night_trip_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_add_event_cb(night_trip_btn, button_click_event, LV_EVENT_CLICKED, (void *)0);
    
    lv_obj_t *night_trip_label = lv_label_create(night_trip_btn);
    lv_label_set_text(night_trip_label, "Night Trip");
    lv_obj_center(night_trip_label);
    
    // 助眠倒计时按钮
    lv_obj_t *sleep_timer_btn = lv_btn_create(left_cont);
    lv_obj_set_size(sleep_timer_btn, 200, 60);
    lv_obj_add_style(sleep_timer_btn, &style_btn, 0);
    lv_obj_add_style(sleep_timer_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_add_event_cb(sleep_timer_btn, button_click_event, LV_EVENT_CLICKED, (void *)1);
    
    lv_obj_t *sleep_timer_label = lv_label_create(sleep_timer_btn);
    lv_label_set_text(sleep_timer_label, "Sleep Timer");
    lv_obj_center(sleep_timer_label);
    
    // 勿扰模式按钮
    lv_obj_t *dnd_btn = lv_btn_create(left_cont);
    lv_obj_set_size(dnd_btn, 200, 60);
    lv_obj_add_style(dnd_btn, &style_btn, 0);
    lv_obj_add_style(dnd_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_add_event_cb(dnd_btn, button_click_event, LV_EVENT_CLICKED, (void *)2);
    
    lv_obj_t *dnd_label = lv_label_create(dnd_btn);
    lv_label_set_text(dnd_label, "DND Mode");
    lv_obj_center(dnd_label);
    
    // 切回客厅按钮
    lv_obj_t *return_btn = lv_btn_create(left_cont);
    lv_obj_set_size(return_btn, 200, 60);
    lv_obj_add_style(return_btn, &style_btn, 0);
    lv_obj_add_style(return_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_add_event_cb(return_btn, button_click_event, LV_EVENT_CLICKED, (void *)3);
    
    lv_obj_t *return_label = lv_label_create(return_btn);
    lv_label_set_text(return_label, "Return");
    lv_obj_center(return_label);
    
    // 右侧数字床头钟 (2/3)
    lv_obj_t *right_cont = lv_obj_create(bedroom_layout);
    lv_obj_set_size(right_cont, lv_pct(67), lv_pct(90));
    lv_obj_set_style_bg_opa(right_cont, 0, 0); // 背景透明
    lv_obj_set_style_border_width(right_cont, 0, 0);
    lv_obj_set_style_shadow_width(right_cont, 0, 0); // 无阴影
    
    // 数字时钟
    lv_obj_t *clock_label = lv_label_create(right_cont);
    lv_label_set_text(clock_label, "12:00:00");
    lv_obj_align(clock_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(clock_label, &lv_font_montserrat_48, 0);
    // 字体颜色设为白色
    lv_obj_set_style_text_color(clock_label, lv_color_white(), 0);
    
    // 启动时钟更新定时器
    lv_timer_create(update_digital_clock, 1000, clock_label);
    
    // 默认隐藏卧室布局
    lv_obj_add_flag(bedroom_layout, LV_OBJ_FLAG_HIDDEN);
}

/* 初始化设置页 */
void init_setting_page(lv_obj_t *parent)
{
    // 1. 基础页面样式：透明背景，禁用滚动
    lv_obj_set_style_bg_opa(parent, 0, 0); // 背景透明
    lv_obj_set_style_pad_all(parent, 0, 0);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    // 2. 创建客厅模式布局容器
    living_room_layout = lv_obj_create(parent);
    lv_obj_set_size(living_room_layout, lv_pct(100), lv_pct(100));
    lv_obj_align(living_room_layout, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(living_room_layout, 0, 0); // 背景透明

    // 3. 创建水平排列的按钮容器
    lv_obj_t *btn_cont = lv_obj_create(living_room_layout);
    lv_obj_set_size(btn_cont, lv_pct(90), 200);
    lv_obj_align(btn_cont, LV_ALIGN_TOP_MID, 0, 50); // 顶部偏移50px
    lv_obj_set_style_bg_opa(btn_cont, 0, 0); // 背景透明
    lv_obj_set_style_border_width(btn_cont, 0, 0);
    
    // 设置水平布局
    lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // 4. 定义通用按钮样式
    static lv_style_t style_btn;
    static lv_style_t style_btn_pressed;
    if(style_btn.prop_cnt == 0) { // 静态初始化，防止重复分配内存
        // 正常状态样式
        lv_style_init(&style_btn);
        lv_style_set_border_color(&style_btn, lv_color_hex(0x32CD32)); // 绿色边框
        lv_style_set_border_width(&style_btn, 2);
        lv_style_set_radius(&style_btn, 5); // 圆角
        lv_style_set_bg_opa(&style_btn, LV_OPA_COVER); // 背景不透明
        lv_style_set_bg_color(&style_btn, lv_color_hex(0xFFFFFF)); // 白色背景
        
        // 按下状态样式
        lv_style_init(&style_btn_pressed);
        lv_style_set_transform_zoom(&style_btn_pressed, 243); // 缩放0.95倍（256 * 0.95 ≈ 243）
        lv_style_set_bg_opa(&style_btn_pressed, LV_OPA_COVER); // 背景不透明
        lv_style_set_bg_color(&style_btn_pressed, lv_color_hex(0xFFFFFF)); // 白色背景
    }

    // 5. 创建功能按钮
    // Mode Selection 按钮
    lv_obj_t *mode_btn = lv_btn_create(btn_cont);
    lv_obj_set_size(mode_btn, 200, 150);
    lv_obj_add_style(mode_btn, &style_btn, 0);
    lv_obj_add_style(mode_btn, &style_btn_pressed, LV_STATE_PRESSED);
    
    // 按钮标题
    lv_obj_t *mode_title = lv_label_create(mode_btn);
    lv_label_set_text(mode_title, "Mode Selection");
    lv_obj_align(mode_title, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_font(mode_title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(mode_title, lv_color_hex(0x000000), 0);
    
    // 按钮子文本
    lv_obj_t *mode_subtext = lv_label_create(mode_btn);
    lv_label_set_text(mode_subtext, "Bedroom");
    lv_obj_align(mode_subtext, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_text_font(mode_subtext, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(mode_subtext, lv_color_hex(0x888888), 0);
    
    // Add smart home 按钮
    lv_obj_t *add_btn = lv_btn_create(btn_cont);
    lv_obj_set_size(add_btn, 200, 150);
    lv_obj_add_style(add_btn, &style_btn, 0);
    lv_obj_add_style(add_btn, &style_btn_pressed, LV_STATE_PRESSED);
    
    // 按钮标题
    lv_obj_t *add_title = lv_label_create(add_btn);
    lv_label_set_text(add_title, "Add smart home");
    lv_obj_align(add_title, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(add_title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(add_title, lv_color_hex(0x000000), 0);
    
    // About the Author 按钮
    lv_obj_t *about_btn = lv_btn_create(btn_cont);
    lv_obj_set_size(about_btn, 200, 150);
    lv_obj_add_style(about_btn, &style_btn, 0);
    lv_obj_add_style(about_btn, &style_btn_pressed, LV_STATE_PRESSED);
    
    // 按钮标题
    lv_obj_t *about_title = lv_label_create(about_btn);
    lv_label_set_text(about_title, "About the Author");
    lv_obj_align(about_title, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(about_title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(about_title, lv_color_hex(0x000000), 0);

    // 6. 保存按钮引用到设置设备数据结构中
    setting_devices[0].card = mode_btn;
    setting_devices[1].card = add_btn;
    setting_devices[2].card = about_btn;
    
    // 7. 添加点击事件
    lv_obj_add_event_cb(mode_btn, mode_selection_click_event, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(add_btn, add_smart_home_click_event, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(about_btn, about_author_click_event, LV_EVENT_CLICKED, NULL);
    
    // 9. 初始化卧室模式布局
    init_bedroom_layout(parent);
}

/* 初始化设置选项子页面 (覆盖层) */
void init_setting_options(lv_obj_t *parent)
{
    // 不再创建提示画面，直接创建一个空的子页面
    setting_sub_page = lv_obj_create(parent);
    lv_obj_set_size(setting_sub_page, lv_pct(100), lv_pct(100));
    lv_obj_align(setting_sub_page, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(setting_sub_page, 0, 0); // 透明背景
    lv_obj_set_style_border_width(setting_sub_page, 0, 0);
    
    /* 默认隐藏此覆盖层 */
    lv_obj_add_flag(setting_sub_page, LV_OBJ_FLAG_HIDDEN);
}

/* 切换到卧室模式布局 */
void switch_to_bedroom_layout(void)
{
    lv_obj_add_flag(living_room_cont, LV_OBJ_FLAG_HIDDEN);    // 藏起客厅盒子
    lv_obj_clear_flag(bedroom_cont, LV_OBJ_FLAG_HIDDEN);     // 露出卧室盒子
    if (bedroom_layout) {
        lv_obj_clear_flag(bedroom_layout, LV_OBJ_FLAG_HIDDEN); // 露出卧室布局
    }
    lv_img_set_src(ui_background, &background02);            // 换卧室背景
}

/* 切换到客厅模式布局 */
void switch_to_living_room_layout(void)
{
    lv_obj_add_flag(bedroom_cont, LV_OBJ_FLAG_HIDDEN);       // 藏起卧室
    if (bedroom_layout) {
        lv_obj_add_flag(bedroom_layout, LV_OBJ_FLAG_HIDDEN); // 藏起卧室布局
    }
    lv_obj_clear_flag(living_room_cont, LV_OBJ_FLAG_HIDDEN);  // 露出客厅
    lv_img_set_src(ui_background, &background01);            // 换客厅背景
}
