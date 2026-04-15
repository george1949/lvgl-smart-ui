#pragma once

#include <stdint.h>
#include "lvgl/lvgl.h"

/* 外部资源声明 */
LV_IMG_DECLARE(background01);
LV_IMG_DECLARE(background02); // 卧室模式背景图

/* Z-index definitions */
#define LV_Z_INDEX_MODAL 100

typedef enum {
    PAGE_LAYER_MAIN = 0,    // 一级页面层（主导航层）
    PAGE_LAYER_SUB,         // 二级页面层（卡片选择层）
    PAGE_LAYER_MODAL        // 三级页面层（详细设置/控制弹窗层）
} page_layer_t;

/* 页面编号 */
typedef enum {
    PAGE_HOME = 0,    // 首页
    PAGE_LIGHT,       // 智能灯控制
    PAGE_ROBOT,       // 扫地机器人控制
    PAGE_SETTING,     // 设置界面
    PAGE_MAX
} page_id_t;

/* 手势定义 */
typedef enum {
    GESTURE_UP = 1,        // 上
    GESTURE_DOWN,          // 下
    GESTURE_LEFT,          // 左
    GESTURE_RIGHT,         // 右
    GESTURE_FORWARD,       // 前
    GESTURE_BACKWARD,      // 后
    GESTURE_CW,            // 顺时针
    GESTURE_CCW            // 逆时针
} gesture_t;

/* 设备状态 */
typedef enum {
    DEVICE_OFF = 0,
    DEVICE_ON
} device_status_t;

/* 机器人状态 */
typedef enum {
    ROBOT_STATUS_STOP = 0,
    ROBOT_STATUS_CLEANING,
    ROBOT_STATUS_CHARGING
} robot_status_t;

/* 灯光设备 */
typedef struct {
    const char *name;
    bool status; // ON/OFF 二元状态
    lv_obj_t *home_label; // 一级页面状态映射指针
    lv_obj_t *card_label; // 二级页面卡片状态映射指针
    lv_obj_t *card;
} light_device_t;

/* 机器人设备 */
typedef struct {
    const char *name;
    robot_status_t status;
    bool is_running; // 运行状态
    lv_obj_t *card;
    lv_obj_t *status_label;
} robot_device_t;

/* 设置选项设备 */
typedef struct {
    const char *name;
    const char *subtext;
    lv_obj_t *card;
} setting_device_t;

/* 模式定义 */
typedef enum {
    MODE_LIVING_ROOM = 0,
    MODE_BEDROOM
} mode_id_t;

/* 全局状态 */
extern page_layer_t current_layer;
extern page_id_t current_page;
extern bool page_selected;
extern bool light_status; // 全局灯光状态（ON/OFF）
extern device_status_t robot_status;
extern mode_id_t current_mode;       // 当前模式
extern robot_status_t robot_current_status;
extern bool gesture_enabled; // 手势识别启用标志

/* POSIX 互斥锁 */
#include <pthread.h>
extern pthread_mutex_t ui_mutex; // 用于UI操作的互斥锁

/* 互斥锁操作宏 */
#define UI_LOCK() pthread_mutex_lock(&ui_mutex)
#define UI_UNLOCK() pthread_mutex_unlock(&ui_mutex)

/* 全局UI对象 */
extern lv_obj_t *tabview;
extern lv_obj_t *mode_status_label;
extern lv_obj_t *home_time_label;
extern lv_obj_t *home_date_label;
extern lv_obj_t *home_light_label;
extern lv_obj_t *home_robot_label;

/* 容器声明 */
extern lv_obj_t * living_room_cont;
extern lv_obj_t * bedroom_cont;

/* 布局声明 */
extern lv_obj_t * living_room_layout;
extern lv_obj_t * bedroom_layout;

/* 智能灯光汇总标签 */
typedef struct {
    lv_obj_t *title; // 标题部分
    lv_obj_t *value; // 状态值部分
} menu_smart_light_summary_t;

extern menu_smart_light_summary_t menu_smart_light_summary;

/* Sub pages */
extern lv_obj_t *light_sub_page;
extern lv_obj_t *robot_sub_page;
extern lv_obj_t *setting_sub_page;

/* 焦点管理 */
extern lv_group_t *focus_group;
extern int current_focus_index;

/* 设备数据 */
extern light_device_t light_devices[];
extern robot_device_t robot_devices[];
extern setting_device_t setting_devices[];
extern int light_device_count;
extern int robot_device_count;
extern int setting_device_count;

/* 函数声明 */
void ui_init_all(void);
void handle_gesture(int gesture);
void ui_manager(page_id_t page);
void ui_next_page(void);
void ui_prev_page(void);
void ui_show_gesture_toast(const char *msg);

/* 页面初始化函数 */
void init_home_page(lv_obj_t *parent);
void init_light_page(lv_obj_t *parent);
void init_robot_page(lv_obj_t *parent);
void init_setting_page(lv_obj_t *parent);

/* 子页面初始化函数 */
void init_light_settings(lv_obj_t *parent);
void init_robot_settings(lv_obj_t *parent);
void init_setting_options(lv_obj_t *parent);

/* 三级页面函数 */
void open_light_device_modal(int device_index);
void open_robot_device_modal(int device_index);
void close_current_modal(void);

/* 焦点管理函数 */
void init_focus_group(void);
void update_focus_group(void);
void set_focus_to_index(int index);

/* UI更新函数 */
void update_light_ui(void);
void update_robot_ui(void);
void update_mode_ui(void);
void update_device_cards(void);
void update_home_time(void);

/* 状态更新函数 */
void update_light_state(int id, bool new_state);
void update_robot_summary(void);
void sync_menu_summary(void);
void sync_robot_to_home(void);

/* 卧室模式函数 */
void night_trip_mode(void);
void start_sleep_timer(void);
void toggle_dnd_mode(void);
void return_to_living_room_mode(void);
void switch_to_bedroom_mode(void);

/* 布局切换函数 */
void switch_to_bedroom_layout(void);
void switch_to_living_room_layout(void);

/* 新增函数声明 */
void sync_logic_state(mode_id_t mode);
void full_scene_switch(mode_id_t target_mode);
void sync_time_via_mqtt(const char *payload);
void sync_time_from_mqtt(const char *payload);
int check_wired_network_status();
void alert_icon_click_event(lv_event_t *e);
int sync_system_time_from_server(void);
