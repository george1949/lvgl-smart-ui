#include "ui_pages.h"
#include "ui_add_device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// 日志宏定义
#define LOG_TAG "[SmartHome]"

// 如果不需要详细过程，可以将这个设为 0
#define DEBUG_MODE 1

// 颜色代码
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define LOGI(fmt, ...) printf(ANSI_COLOR_GREEN LOG_TAG " [INFO] " fmt ANSI_COLOR_RESET "\n", ##__VA_ARGS__)
#define LOGE(fmt, ...) printf(ANSI_COLOR_RED LOG_TAG " [ERROR] " fmt ANSI_COLOR_RESET "\n", ##__VA_ARGS__)
#define LOGD(fmt, ...) if(DEBUG_MODE) printf(LOG_TAG " [DEBUG] " fmt "\n", ##__VA_ARGS__)

// 函数前向声明
int check_wired_network_status();

// 互斥锁定义
pthread_mutex_t ui_mutex;

// 修正后的宏：每次调用都会创建一个独立的局部作用域，互不干扰
#define MQTT_REPORT(topic, msg) \
    do { \
        char cmd_temp[512]; \
        memset(cmd_temp, 0, sizeof(cmd_temp)); \
        /* 1. 移除了 /mnt/udisk/ 路径，直接使用 /usr/bin 下的命令 */ \
        /* 2. 增加到 512 字节防止长消息溢出 */ \
        snprintf(cmd_temp, sizeof(cmd_temp), "mosquitto_pub -h 47.118.21.238 -t \"%s\" -m \"%s\" -u \"home_admin\" -P \"321\" &", topic, msg); \
        system(cmd_temp); \
    } while(0)

/* 手势常量定义 */
#define GESTURE_UP 1
#define GESTURE_DOWN 2
#define GESTURE_LEFT 3
#define GESTURE_RIGHT 4
#define GESTURE_FORWARD 5
#define GESTURE_BACKWARD 6
#define GESTURE_CLOCKWISE 7
#define GESTURE_COUNT_CLOCKWISE 8

/* 图像声明 */
extern const lv_img_dsc_t background01;
extern const lv_img_dsc_t background02;

/* 函数前置声明 */
void sync_menu_summary(void);
void sync_robot_to_home(void);
void update_robot_summary(void);
void sync_home_state(void);
void clear_ui_selection(void);
void close_all_modals(void);
void cleanup_ui_elements(void);
static void msgbox_event_cb(lv_event_t * e);

/* 场景容器 */
lv_obj_t * living_room_cont = NULL; // 客厅根容器
lv_obj_t * bedroom_cont = NULL;    // 卧室根容器

/* 全局状态变量 */
page_layer_t current_layer = PAGE_LAYER_MAIN;
page_id_t current_page = PAGE_HOME;
bool page_selected = false;
bool light_status = false; // 全局灯光状态（ON/OFF）
device_status_t robot_status = DEVICE_OFF;
mode_id_t current_mode = MODE_LIVING_ROOM;       // 当前模式
robot_status_t robot_current_status = ROBOT_STATUS_STOP;
bool gesture_enabled = true; // 手势识别启用标志
bool gesture_active = false; // 手势识别活跃标志，用于屏蔽滑块滑动事件
bool about_author_open = false; // 关于作者弹窗是否打开

// 全局模式状态：0: Living Room, 1: Bedroom
int global_system_mode = 0;

/* 清除手势活跃标志的回调函数 */
static void clear_gesture_active_cb(lv_timer_t *timer) {
    gesture_active = false;
}

/* 解锁UI的回调函数 */
static void unlock_ui_cb(lv_timer_t *timer) {
    pthread_mutex_unlock(&ui_mutex);
}

/* 卧室模式相关变量 */
bool dnd_mode = false; // 勿扰模式
lv_timer_t *sleep_timer = NULL; // 助眠倒计时器

/* 灯光设备索引定义 */
#define BEDSIDE_LAMP_INDEX 0
#define HALLWAY_LIGHT_INDEX 1
#define BATHROOM_LIGHT_INDEX 2
#define BALCONY_LIGHT_INDEX 3

/* 全局UI对象 */
lv_obj_t *tabview = NULL;
lv_obj_t *mode_status_label = NULL;
lv_obj_t *ui_background = NULL;
lv_obj_t *ui_focus_frame = NULL;
lv_obj_t *current_msgbox = NULL;

lv_obj_t *home_time_label = NULL;
lv_obj_t *home_date_label = NULL;
lv_obj_t *home_light_label = NULL;
lv_obj_t *home_robot_label = NULL;

/* 智能灯光汇总标签 */
menu_smart_light_summary_t menu_smart_light_summary = { NULL, NULL };

/* 子页面对象 */
lv_obj_t *light_sub_page = NULL;
lv_obj_t *robot_sub_page = NULL;
lv_obj_t *setting_sub_page = NULL;

/* 三级页面对象 */
lv_obj_t *current_modal = NULL;
lv_obj_t *modal_mask = NULL;
int current_device_index = -1;

/* 焦点管理 */
lv_group_t *focus_group = NULL;
int current_focus_index = 0;

/* 设备数据 */
light_device_t light_devices[] = {
    {"Bedroom bedside lamp", false, NULL, NULL, NULL},
    {"Living room light", false, NULL, NULL, NULL},
    {"Bathroom light", false, NULL, NULL, NULL},
    {"Balcony light", false, NULL, NULL, NULL}
};

robot_device_t robot_devices[] = {
    {"Main Robot", ROBOT_STATUS_STOP, false, NULL, NULL}
};

/* 设置选项设备数据 */
setting_device_t setting_devices[] = {
    {"Mode Selection", "Bedroom", NULL},
    {"Add smart home", "", NULL},
    {"About the Author", "", NULL}
};

int light_device_count = sizeof(light_devices) / sizeof(light_device_t);
int robot_device_count = sizeof(robot_devices) / sizeof(robot_device_t);
int setting_device_count = sizeof(setting_devices) / sizeof(setting_device_t);

/* 手势防抖 */
static uint32_t last_gesture_time = 0;

/* 函数声明 */
static int page_to_tab_index(page_id_t p);
static lv_obj_t *make_tab_content(lv_obj_t *parent, const char *title);
static void tabbar_style(lv_obj_t *tv);
void update_home_time(void);
static void create_modal_mask(void);
static void update_focus_visual(void);

/* 页面到标签索引的转换 */
static int page_to_tab_index(page_id_t p)
{
    switch(p) {
        case PAGE_HOME:    return 0;
        case PAGE_LIGHT:   return 1;
        case PAGE_ROBOT:   return 2;
        case PAGE_SETTING: return 3;
        default: return 0;
    }
}

/* 创建标签内容 */
static lv_obj_t *make_tab_content(lv_obj_t *parent, const char *title)
{
    lv_obj_t *tab = lv_tabview_add_tab(parent, title);
    return tab;
}

/* 设置标签栏样式 */
static void tabbar_style(lv_obj_t *tv)
{
    lv_obj_t *tabbar = lv_obj_get_child(tv, 0);
    
    if (tabbar) {
        lv_obj_set_style_bg_color(tabbar, lv_color_hex(0xF2F2F7), 0);
        lv_obj_set_style_bg_opa(tabbar, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(tabbar, 0, 0);
        lv_obj_set_style_pad_all(tabbar, 0, 0);
        lv_obj_set_height(tabbar, 50);
    }
}

/* 时钟定时器回调函数 */
static void clock_timer_cb(lv_timer_t *timer)
{
    // 即使 UI 忙碌，时钟也应该继续更新
    update_home_time();
}

/* 更新首页时间和日期 */
void update_home_time(void)
{
    if (!home_time_label || !home_date_label) return;
    
    time_t now;
    struct tm *tm_info;
    char time_str[9];
    char date_str[11];
    
    time(&now);
    tm_info = localtime(&now);
    
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);
    strftime(date_str, sizeof(date_str), "%Y-%m-%d", tm_info);
    
    lv_label_set_text(home_time_label, time_str);
    lv_label_set_text(home_date_label, date_str);
    
}

/* 通过 MQTT 同步时间 */
void sync_time_via_mqtt(const char *payload)
{
    long remote_timestamp = atol(payload);
    
    // 注意：在 Linux 中，设置系统时间通常需要 root 权限
    // 这里我们只更新 UI 显示，实际的系统时间设置可能需要其他方式
    
    // 更新 UI 显示
    update_home_time();
    printf("[TIME] Synced via MQTT: %ld\n", remote_timestamp);
}

/* 从 MQTT 同步时间并更新系统时间 */
void sync_time_from_mqtt(const char *payload)
{
    if (!payload) {
        printf("[TIME] Error: Empty payload\n");
        return;
    }
    
    long remote_timestamp = atol(payload);
    if (remote_timestamp <= 0) {
        printf("[TIME] Error: Invalid timestamp\n");
        return;
    }
    
    // 更新系统时间
    struct timespec ts;
    ts.tv_sec = remote_timestamp;
    ts.tv_nsec = 0;
    
    if (clock_settime(CLOCK_REALTIME, &ts) != 0) {
        perror("[TIME] Failed to set system time");
        printf("[TIME] Note: Setting system time may require root privileges\n");
    } else {
        printf("[TIME] System time updated via MQTT: %ld\n", remote_timestamp);
    }
    
    // 更新 UI 显示
    update_home_time();
    printf("[TIME] UI time synchronized\n");
}



/* 页面管理函数 */
void ui_manager(page_id_t page)
{
    if (page >= PAGE_MAX) return;
    
    current_page = page;
    current_layer = PAGE_LAYER_MAIN; // Reset layer on tab switch
    
    if (tabview) {
        lv_tabview_set_act(tabview, current_page, LV_ANIM_ON);
        printf("[UI] P:%d\n", current_page);
    }
}

/* 启用触摸屏回调函数 */
static void enable_touch_cb(lv_timer_t *timer) {
    pthread_mutex_unlock(&ui_mutex);
}

/* 切换到下一个页面 */
void ui_next_page()
{
    // 设置UI忙碌标志，禁用触摸屏
    pthread_mutex_lock(&ui_mutex);
    
    current_layer = PAGE_LAYER_MAIN;
    current_page++;
    if(current_page >= PAGE_MAX) {
        current_page = 0;
    }
    int prev_page = (current_page == 0) ? PAGE_MAX - 1 : current_page - 1;
    lv_tabview_set_act(tabview, current_page, LV_ANIM_ON);
    printf("[UI] P:%d >> P:%d\n", prev_page, current_page);
    
    // 短暂延迟后重新启用触摸屏
    lv_timer_create(enable_touch_cb, 100, NULL);
}

/* 切换到上一个页面 */
void ui_prev_page()
{
    // 设置UI忙碌标志，禁用触摸屏
    pthread_mutex_lock(&ui_mutex);
    
    current_layer = PAGE_LAYER_MAIN;
    if(current_page == 0) {
        current_page = PAGE_MAX - 1;
    } else {
        current_page--;
    }
    int prev_page = (current_page == PAGE_MAX - 1) ? 0 : current_page + 1;
    lv_tabview_set_act(tabview, current_page, LV_ANIM_ON);
    printf("[UI] P:%d >> P:%d\n", prev_page, current_page);
    
    // 短暂延迟后重新启用触摸屏
    lv_timer_create(enable_touch_cb, 100, NULL);
}

/* 隐藏所有子页面 */
static void hide_all_sub_pages(void)
{
    if (light_sub_page) lv_obj_add_flag(light_sub_page, LV_OBJ_FLAG_HIDDEN);
    if (robot_sub_page) lv_obj_add_flag(robot_sub_page, LV_OBJ_FLAG_HIDDEN);
    if (setting_sub_page) lv_obj_add_flag(setting_sub_page, LV_OBJ_FLAG_HIDDEN);
}

/* 显示特定子页面 */
static void show_current_sub_page(void)
{
    hide_all_sub_pages();
    switch (current_page) {
        case PAGE_LIGHT:
            if (light_sub_page) {
                lv_obj_clear_flag(light_sub_page, LV_OBJ_FLAG_HIDDEN);
            }
            break;
        case PAGE_ROBOT:
            if (robot_sub_page) {
                lv_obj_clear_flag(robot_sub_page, LV_OBJ_FLAG_HIDDEN);
            }
            break;
        case PAGE_SETTING:
            if (setting_sub_page) {
                lv_obj_clear_flag(setting_sub_page, LV_OBJ_FLAG_HIDDEN);
            }
            break;
        case PAGE_HOME:
        case PAGE_MAX:
                break;
    }
}

/* 手势处理函数 */
void handle_gesture(int gesture)
{
    
    // 检查手势是否启用
    if (!gesture_enabled) {
        return;
    }
    
    // 检查UI是否忙碌（使用互斥锁尝试锁定）
    if (pthread_mutex_trylock(&ui_mutex) != 0) {
        return;
    }
    // 操作完成后解锁
    pthread_mutex_unlock(&ui_mutex);
    
    // 手势防抖
    if(lv_tick_elaps(last_gesture_time) < 300) {
        return;
    }
    last_gesture_time = lv_tick_get();
    
    // 设置手势活跃标志
    gesture_active = true;
    // 短暂延迟后清除手势活跃标志
    lv_timer_create(clear_gesture_active_cb, 500, NULL);
    
    // 处理逆时针手势（退出逻辑）
    if (gesture == GESTURE_COUNT_CLOCKWISE) {
        // 第一步：执行物理清理
        cleanup_ui_elements();

        // 第二步：模式跳转
        if (current_mode == MODE_BEDROOM) {
            return_to_living_room_mode(); // 切换背景
            
            // 核心修改：逻辑归零
            current_mode = MODE_LIVING_ROOM;
            current_page = PAGE_HOME;
            current_layer = PAGE_LAYER_MAIN;
            
            // 强制 UI 切换回主页 Tab
            lv_tabview_set_act(tabview, 0, LV_ANIM_OFF);
            
            // 必须清除之前的弹窗状态，防止“幽灵点击”
            close_all_modals();
            
            ui_manager(current_page); // 刷新 UI
            update_focus_visual();
            printf("[UI] Mode Reset: Return to Living Room, Layer Main\n");
            // 新增MQTT上报
            MQTT_REPORT("home/mode", "LIVING_ROOM_MODE");
            return; // 退出模式后直接返回，防止误触发当前页的其他手势
        } 
        // 第三步：普通层级回退（非卧室模式下）
        else if (current_layer > PAGE_LAYER_MAIN) {
            current_layer--;
            if (current_layer == PAGE_LAYER_MAIN) {
                update_focus_visual();
                ui_manager(current_page);
            }
        }
        
        pthread_mutex_unlock(&ui_mutex);
        return;
    }

    // 卧室模式特殊处理
    if (current_mode == MODE_BEDROOM) {
        switch(gesture) {
            case GESTURE_UP: { // 控制“起夜灯”
                // 切换起夜灯状态（床头灯、走廊灯、厕所灯）
                bool night_light_on = !light_devices[BEDSIDE_LAMP_INDEX].status;
                update_light_state(BEDSIDE_LAMP_INDEX, night_light_on);
                update_light_state(HALLWAY_LIGHT_INDEX, night_light_on);
                update_light_state(BATHROOM_LIGHT_INDEX, night_light_on);
                printf("[MODE] Night trip light %s\n", night_light_on ? "ON" : "OFF");
                // 新增MQTT上报
                MQTT_REPORT("home/gesture", night_light_on ? "NIGHT_LIGHT_ON" : "NIGHT_LIGHT_OFF");
                break;
            }
            case GESTURE_LEFT: { // 控制“灯 A”（卧室床头灯）
                bool light_a_on = !light_devices[BEDSIDE_LAMP_INDEX].status;
                update_light_state(BEDSIDE_LAMP_INDEX, light_a_on);
                printf("[MODE] Light A (Bedside) %s\n", light_a_on ? "ON" : "OFF");
                // 新增MQTT上报
                MQTT_REPORT("home/gesture", light_a_on ? "BEDSIDE_LIGHT_ON" : "BEDSIDE_LIGHT_OFF");
                break;
            }
            case GESTURE_RIGHT: { // 控制“灯 B”（客厅灯）
                bool light_b_on = !light_devices[1].status;
                update_light_state(1, light_b_on);
                printf("[MODE] Light B (Living room) %s\n", light_b_on ? "ON" : "OFF");
                // 新增MQTT上报
                MQTT_REPORT("home/gesture", light_b_on ? "LIVING_ROOM_LIGHT_ON" : "LIVING_ROOM_LIGHT_OFF");
                break;
            }
            // 其他手势在卧室模式下均被忽略
            default:
                break;
        }
        return;
    }

    // 一级页面（HOME / PAGE_LAYER_MAIN）
    if (current_layer == PAGE_LAYER_MAIN) {
        switch(gesture) {
            case GESTURE_LEFT: // 左：向左滑动页面
                // 强制结束当前触控事件
                lv_indev_wait_release(lv_indev_get_act());
                ui_next_page(); // 手势3调用 ui_next_page()：home -> light -> robot -> setting -> home...
                printf("[DEBUG] Actual Page changed to: %d, Layer: %d\n", current_page, current_layer);
                // 新增MQTT上报
                char page_msg1[32];
                sprintf(page_msg1, "PAGE_CHANGED_%d", current_page);
                MQTT_REPORT("home/status", page_msg1);
                break;
            case GESTURE_RIGHT: // 右：向右滑动页面
                // 强制结束当前触控事件
                lv_indev_wait_release(lv_indev_get_act());
                ui_prev_page(); // 手势4调用 ui_prev_page()：home -> setting -> robot -> light -> home...
                printf("[DEBUG] Actual Page changed to: %d, Layer: %d\n", current_page, current_layer);
                // 新增MQTT上报
                char page_msg2[32];
                sprintf(page_msg2, "PAGE_CHANGED_%d", current_page);
                MQTT_REPORT("home/status", page_msg2);
                break;
            case GESTURE_CLOCKWISE: // 顺时针：进入二级菜单
                if (current_page != PAGE_HOME) {
                    // 检查是否有合法的选中项
                    int dev_count = 0;
                    if (current_page == PAGE_LIGHT) {
                        dev_count = light_device_count;
                    } else if (current_page == PAGE_ROBOT) {
                        dev_count = robot_device_count;
                    } else if (current_page == PAGE_SETTING) {
                        dev_count = setting_device_count;
                    }
                    
                    if (dev_count > 0) {
                        current_layer = PAGE_LAYER_SUB;
                        show_current_sub_page();
                        // 初始化并更新焦点组
                        update_focus_group();
                        printf("[DEBUG] Actual Page changed to: %d, Layer: %d\n", current_page, current_layer);
                    }
                }
                break;
            // 其他手势在一级页面均被忽略
            default:
                break;
        }
        return;
    }

    // 二级页面（LAYER_1 / 选中框模式）
    if (current_layer == PAGE_LAYER_SUB) {
        switch(gesture) {
            case GESTURE_LEFT: { // 左：移动到下一个选中框
                int max_index;
                if (current_page == PAGE_LIGHT) {
                    max_index = light_device_count - 1;
                } else if (current_page == PAGE_ROBOT) {
                    max_index = robot_device_count - 1;
                } else if (current_page == PAGE_SETTING) {
                    max_index = setting_device_count - 1;
                } else {
                    max_index = 0;
                }
                if (current_focus_index < max_index) {
                    set_focus_to_index(current_focus_index + 1);
                }
                break;
            }
            case GESTURE_RIGHT: { // 右：移动到上一个选中框
                if (current_focus_index > 0) {
                    set_focus_to_index(current_focus_index - 1);
                }
                break;
            }
            case GESTURE_CLOCKWISE: { // 顺时针：确认选中项，进入三级操作页面
                if (current_page == PAGE_LIGHT) {
                    open_light_device_modal(current_focus_index);
                    printf("[DEBUG] Actual Page changed to: %d, Layer: %d\n", current_page, current_layer);
                } else if (current_page == PAGE_ROBOT) {
                    open_robot_device_modal(current_focus_index);
                    printf("[DEBUG] Actual Page changed to: %d, Layer: %d\n", current_page, current_layer);
                } else if (current_page == PAGE_SETTING) {
                    // 对于设置页面，当焦点在 Mode Selection 卡片上时，进入卧室模式
                    if (current_focus_index == 0) {
                        // 切换到卧室模式
                        switch_to_bedroom_mode();
                        // 切换到卧室布局
                        switch_to_bedroom_layout();
                        ui_show_gesture_toast("Entering bedroom mode");
                        printf("[DEBUG] Actual Page changed to: %d, Layer: %d\n", current_page, current_layer);
                        // 新增MQTT上报
                        MQTT_REPORT("home/mode", "BEDROOM_MODE");
                    } else {
                        ui_show_gesture_toast("Setting option clicked");
                    }
                }
                break;
            }
            // 其他手势在二级页面均被忽略
            default:
                break;
        }
        return;
    }

    // 三级页面（LAYER_2 / 详情操作）
    if (current_layer == PAGE_LAYER_MODAL) {
        switch(gesture) {
            case GESTURE_UP: // 上：在详情页内进行上下选择
            case GESTURE_DOWN: { // 下：在详情页内进行上下选择
                if (current_page == PAGE_LIGHT && current_device_index >= 0) {
                    // 翻转灯光状态
                    bool new_state = !light_devices[current_device_index].status;
                    update_light_state(current_device_index, new_state);
                    
                    // 更新模态窗口中的状态显示
                    if (current_modal) {
                        lv_obj_t *title = lv_obj_get_child(current_modal, 0);
                        if (title) {
                            lv_obj_t *status_label = lv_obj_get_child(current_modal, 1);
                            if (status_label) {
                                lv_label_set_text(status_label, new_state ? "ON" : "OFF");
                                lv_obj_set_style_text_color(status_label, 
                                    new_state ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF0000), 0);
                                // 强制重新绘制标签
                                lv_obj_invalidate(status_label);
                            }
                        }
                    }
                    // 新增MQTT上报
                    char light_msg[64];
                    sprintf(light_msg, "LIGHT_%d_%s", current_device_index, new_state ? "ON" : "OFF");
                    MQTT_REPORT("home/device", light_msg);
                } else if (current_page == PAGE_ROBOT && current_device_index >= 0) {
                    // 翻转机器人运行状态
                    bool new_state = !robot_devices[current_device_index].is_running;
                    robot_devices[current_device_index].is_running = new_state;
                    
                    // 更新机器人状态汇总
                    update_robot_summary();
                    
                    // 更新模态窗口中的状态显示
                    if (current_modal) {
                        lv_obj_t *title = lv_obj_get_child(current_modal, 0);
                        if (title) {
                            lv_obj_t *status_label = lv_obj_get_child(current_modal, 1);
                            if (status_label) {
                                const char *status_str;
                                lv_color_t color;
                                
                                if (new_state) {
                                    status_str = "Clearing";
                                    color = lv_color_hex(0x1396CE); // 亮蓝色
                                } else {
                                    status_str = "Docking / Charging";
                                    color = lv_color_hex(0xCCCCCC); // 灰色
                                }
                                
                                lv_label_set_text(status_label, status_str);
                                lv_obj_set_style_text_color(status_label, color, 0);
                                // 强制重新绘制标签
                                lv_obj_invalidate(status_label);
                            }
                        }
                    }
                    // 新增MQTT上报
                    MQTT_REPORT("home/device", new_state ? "ROBOT_CLEARING" : "ROBOT_DOCKING");
                }
                break;
            }
            case GESTURE_CLOCKWISE: { // 顺时针：完成并退出
            case GESTURE_COUNT_CLOCKWISE: // 逆时针：完成并退出
                close_current_modal();
                printf("[DEBUG] Actual Page changed to: %d, Layer: %d\n", current_page, current_layer);
                break;
            }
            // 其他手势在三级页面均被忽略
            default:
                break;
        }
        return;
    }
}

/* 更新灯光UI */
void update_light_ui(void)
{
    if (home_light_label) {
        lv_label_set_text(home_light_label, light_status ? "ON" : "OFF");
        // Keep yellow if ON, otherwise light grey
        lv_obj_set_style_text_color(home_light_label, 
                                  light_status ? lv_color_hex(0xFCE616) : lv_color_hex(0xCCCCCC), 0);
    }
}


/* 更新模式UI */
void update_mode_ui(void)
{
    if (mode_status_label) {
        const char *mode_str;
        if (current_mode == MODE_LIVING_ROOM) {
            mode_str = "Living Room";
        } else if (current_mode == MODE_BEDROOM) {
            mode_str = "Bedroom";
        } else {
            mode_str = "Unknown";
        }
        lv_label_set_text(mode_status_label, mode_str);
        lv_obj_set_style_text_color(mode_status_label, lv_color_hex(0x32CD32), 0); // Green mode text
    }
}

/* 更新机器人UI */
void update_robot_ui(void)
{
    const char *status_str;
    switch(robot_current_status) {
        case ROBOT_STATUS_STOP:
            status_str = "STOP";
            break;
        case ROBOT_STATUS_CLEANING:
            status_str = "CLEANING";
            break;
        case ROBOT_STATUS_CHARGING:
            status_str = "CHARGING";
            break;
        default:
            status_str = "UNKNOWN";
    }

    if (home_robot_label) {
        lv_label_set_text(home_robot_label, status_str);
        lv_obj_set_style_text_color(home_robot_label, 
            robot_current_status == ROBOT_STATUS_STOP ? lv_color_hex(0xFF0000) : lv_color_hex(0x00FF00), 0);
    }
}

/* 显示手势提示 */
void ui_show_gesture_toast(const char *msg)
{
    LOGD("Toast: %s", msg);
    // 这里可以添加 LVGL 的 Toast 提示实现
    // 暂时使用控制台输出代替
}



/* 网络状态检测函数 */
int check_wired_network_status() {
    FILE *fp = fopen("/sys/class/net/eth0/carrier", "r");
    if(fp == NULL) return 0;

    int status = 0;
    fscanf(fp, "%d", &status);
    fclose(fp);
    
    return (status == 1); // 1表示物理连接已建立
}

/* 消息框删除事件回调函数 */
static void msgbox_delete_event_cb(lv_event_t *ev) {
    // 获取用户数据，这里传递 mbox 指针的地址
    lv_obj_t **mbox_ptr = (lv_obj_t **)lv_event_get_user_data(ev);
    if(mbox_ptr) {
        *mbox_ptr = NULL;
        LOGD("Message box deleted and pointer cleared");
    }
}

/* 状态图标点击事件处理函数 */
void alert_icon_click_event(lv_event_t *e) {
    // 使用之前修复的野指针保护逻辑
    static lv_obj_t * mbox = NULL;
    if(mbox != NULL) return;

    // 获取警告图标对象
    lv_obj_t *alert_icon = lv_event_get_target(e);

    LOGI("User clicked sync icon");

    // 1. 创建初始消息框 (Simple English)
    mbox = lv_msgbox_create(NULL, "System", "Syncing time...", NULL, true);
    lv_obj_center(mbox);

    // 2. 执行同步逻辑
    if (sync_system_time_from_server() == 0) {
        // 同步成功：更新消息内容
        lv_obj_t *text_obj = lv_msgbox_get_text(mbox);
        if(text_obj) {
            lv_label_set_text(text_obj, "Time Sync Success!\nSystem Status: OK");
        }
        
        // 3. 视觉反馈：将图标颜色改为绿色 (LV_PALETTE_GREEN)
        if(alert_icon) {
            lv_obj_set_style_text_color(alert_icon, lv_palette_main(LV_PALETTE_GREEN), 0);
            LOGD("UI Alert Icon changed to GREEN");
        }
    } else {
        // 同步失败：显示错误原因
        lv_obj_t *text_obj = lv_msgbox_get_text(mbox);
        if(text_obj) {
            lv_label_set_text(text_obj, "Sync Failed!\nCheck Network.");
        }
        
        // 保持红色反馈
        if(alert_icon) {
            lv_obj_set_style_text_color(alert_icon, lv_palette_main(LV_PALETTE_RED), 0);
            LOGD("UI Alert Icon changed to RED");
        }
    }

    // 绑定删除回调，确保 mbox 销毁后指针归零
    lv_obj_add_event_cb(mbox, msgbox_delete_event_cb, LV_EVENT_DELETE, &mbox);
}

/* 初始化焦点组 */
void init_focus_group(void)
{
    if (focus_group) {
        lv_group_del(focus_group);
    }
    focus_group = lv_group_create();
    current_focus_index = 0;
}

/* 更新焦点组 */
void update_focus_group(void)
{
    if (!focus_group) {
        init_focus_group();
    }
    
    // 清空现有焦点组
    lv_group_remove_all_objs(focus_group);
    
    // 根据当前页面添加设备卡片到焦点组
    if (current_page == PAGE_LIGHT) {
        for (int i = 0; i < light_device_count; i++) {
            if (light_devices[i].card) {
                lv_group_add_obj(focus_group, light_devices[i].card);
            }
        }
    } else if (current_page == PAGE_ROBOT) {
        for (int i = 0; i < robot_device_count; i++) {
            if (robot_devices[i].card) {
                lv_group_add_obj(focus_group, robot_devices[i].card);
            }
        }
    } else if (current_page == PAGE_SETTING) {
        for (int i = 0; i < setting_device_count; i++) {
            if (setting_devices[i].card) {
                lv_group_add_obj(focus_group, setting_devices[i].card);
            }
        }
    }
    
    // 设置初始焦点
    if (lv_group_get_obj_count(focus_group) > 0) {
        set_focus_to_index(0);
    }
}

/* 设置焦点到指定索引 */
void set_focus_to_index(int index)
{
    if (!focus_group) return;
    
    int obj_count = lv_group_get_obj_count(focus_group);
    if (index < 0) index = 0;
    if (index >= obj_count) index = obj_count - 1;
    
    current_focus_index = index;
    
    // 先获取当前焦点对象
    lv_obj_t *current_focus = lv_group_get_focused(focus_group);
    
    // 如果没有焦点对象，先聚焦第一个
    if (!current_focus && obj_count > 0) {
        // 由于LVGL 8.2没有直接获取第一个对象的函数，我们可以先添加一个临时对象，聚焦它，然后移除
        lv_obj_t *temp_obj = lv_obj_create(lv_scr_act());
        lv_group_add_obj(focus_group, temp_obj);
        lv_group_focus_obj(temp_obj);
        lv_group_remove_obj(temp_obj);
        lv_obj_del(temp_obj);
    }
    
    // 然后通过focus_next/focus_prev来移动到指定索引
    if (obj_count > 0) {
        // 先聚焦到第一个对象
        for (int i = 0; i < obj_count; i++) {
            lv_group_focus_prev(focus_group);
        }
        
        // 然后移动到指定索引
        for (int i = 0; i < index; i++) {
            lv_group_focus_next(focus_group);
        }
        
        update_focus_visual();
    }
}

/* 更新焦点视觉效果 */
static void update_focus_visual(void)
{
    // 如果是一级页面，不显示选中框
    if (current_layer == PAGE_LAYER_MAIN) {
        // 重置所有卡片样式
        if (current_page == PAGE_LIGHT) {
            for (int i = 0; i < light_device_count; i++) {
                if (light_devices[i].card) {
                    lv_obj_set_style_border_color(light_devices[i].card, lv_color_hex(0x1396CE), 0);
                    lv_obj_set_style_border_width(light_devices[i].card, 3, 0);
                    lv_obj_set_style_shadow_width(light_devices[i].card, 0, 0);
                }
            }
        } else if (current_page == PAGE_ROBOT) {
            for (int i = 0; i < robot_device_count; i++) {
                if (robot_devices[i].card) {
                    lv_obj_set_style_border_color(robot_devices[i].card, lv_color_hex(0x1396CE), 0);
                    lv_obj_set_style_border_width(robot_devices[i].card, 3, 0);
                    lv_obj_set_style_shadow_width(robot_devices[i].card, 0, 0);
                }
            }
        } else if (current_page == PAGE_SETTING) {
            for (int i = 0; i < setting_device_count; i++) {
                if (setting_devices[i].card) {
                    lv_obj_set_style_border_color(setting_devices[i].card, lv_color_hex(0x32CD32), 0);
                    lv_obj_set_style_border_width(setting_devices[i].card, 3, 0);
                    lv_obj_set_style_shadow_width(setting_devices[i].card, 0, 0);
                }
            }
        }
        return;
    }
    
    if (!focus_group) return;
    
    lv_obj_t *focused = lv_group_get_focused(focus_group);
    
    // 重置所有卡片样式
    if (current_page == PAGE_LIGHT) {
        for (int i = 0; i < light_device_count; i++) {
            if (light_devices[i].card) {
                lv_obj_set_style_border_color(light_devices[i].card, lv_color_hex(0x1396CE), 0);
                lv_obj_set_style_border_width(light_devices[i].card, 3, 0);
                lv_obj_set_style_shadow_width(light_devices[i].card, 0, 0);
            }
        }
    } else if (current_page == PAGE_ROBOT) {
        for (int i = 0; i < robot_device_count; i++) {
            if (robot_devices[i].card) {
                lv_obj_set_style_border_color(robot_devices[i].card, lv_color_hex(0x1396CE), 0);
                lv_obj_set_style_border_width(robot_devices[i].card, 3, 0);
                lv_obj_set_style_shadow_width(robot_devices[i].card, 0, 0);
            }
        }
    } else if (current_page == PAGE_SETTING) {
        for (int i = 0; i < setting_device_count; i++) {
            if (setting_devices[i].card) {
                lv_obj_set_style_border_color(setting_devices[i].card, lv_color_hex(0x32CD32), 0);
                lv_obj_set_style_border_width(setting_devices[i].card, 3, 0);
                lv_obj_set_style_shadow_width(setting_devices[i].card, 0, 0);
            }
        }
    }
    
    // 设置焦点卡片样式
    if (focused) {
        lv_obj_set_style_border_color(focused, lv_color_hex(0xFF9500), 0);
        lv_obj_set_style_border_width(focused, 4, 0);
        lv_obj_set_style_shadow_width(focused, 10, 0);
        lv_obj_set_style_shadow_color(focused, lv_color_hex(0xFF9500), 0);
        lv_obj_set_style_shadow_opa(focused, LV_OPA_30, 0);
        lv_obj_set_style_shadow_ofs_y(focused, 5, 0);
    }
}

/* 创建模态窗口遮罩 */
static void create_modal_mask(void)
{
    if (!modal_mask) {
        modal_mask = lv_obj_create(lv_scr_act());
        lv_obj_set_size(modal_mask, lv_pct(100), lv_pct(100));
        lv_obj_set_style_bg_color(modal_mask, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(modal_mask, LV_OPA_50, 0);
        lv_obj_add_flag(modal_mask, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(modal_mask, LV_OBJ_FLAG_IGNORE_LAYOUT);
        // 在LVGL 8.2中，使用lv_obj_move_to_index来控制显示顺序
        // 移动到最后一个位置，使其显示在其他元素之上
        lv_obj_move_to_index(modal_mask, -1);
    }
}

/* 打开灯光设备模态窗口 */
void open_light_device_modal(int device_index)
{
    if (device_index < 0 || device_index >= light_device_count) return;
    
    // 关闭现有模态窗口
    close_current_modal();
    
    // 创建遮罩
    create_modal_mask();
    lv_obj_clear_flag(modal_mask, LV_OBJ_FLAG_HIDDEN);
    
    // 创建模态窗口
    current_modal = lv_obj_create(lv_scr_act());
    lv_obj_set_size(current_modal, 500, 200);
    lv_obj_align(current_modal, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(current_modal, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_color(current_modal, lv_color_hex(0x1396CE), 0);
    lv_obj_set_style_border_width(current_modal, 3, 0);
    lv_obj_set_style_radius(current_modal, 10, 0);
    // 在LVGL 8.2中，使用lv_obj_move_to_index来控制显示顺序
    // 移动到最后一个位置，使其显示在其他元素之上
    lv_obj_move_to_index(current_modal, -1);
    
    // 设置标题
    lv_obj_t *title = lv_label_create(current_modal);
    lv_label_set_text(title, light_devices[device_index].name);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    
    // 设置状态显示
    lv_obj_t *status_label = lv_label_create(current_modal);
    lv_label_set_text(status_label, light_devices[device_index].status ? "ON" : "OFF");
    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(status_label, light_devices[device_index].status ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF0000), 0);
    
    // 保存当前设备索引
    current_device_index = device_index;
    current_layer = PAGE_LAYER_MODAL;
    
    printf("[UI] Modal opened for light device: %s\n", light_devices[device_index].name);
}

/* 打开机器人设备模态窗口 */
void open_robot_device_modal(int device_index)
{
    if (device_index < 0 || device_index >= robot_device_count) return;
    
    // 关闭现有模态窗口
    close_current_modal();
    
    // 创建遮罩
    create_modal_mask();
    lv_obj_clear_flag(modal_mask, LV_OBJ_FLAG_HIDDEN);
    
    // 创建模态窗口
    current_modal = lv_obj_create(lv_scr_act());
    lv_obj_set_size(current_modal, 500, 350);
    lv_obj_align(current_modal, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(current_modal, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_color(current_modal, lv_color_hex(0x1396CE), 0);
    lv_obj_set_style_border_width(current_modal, 3, 0);
    lv_obj_set_style_radius(current_modal, 10, 0);
    // 在LVGL 8.2中，使用lv_obj_move_to_index来控制显示顺序
    // 移动到最后一个位置，使其显示在其他元素之上
    lv_obj_move_to_index(current_modal, -1);
    
    // 设置标题
    lv_obj_t *title = lv_label_create(current_modal);
    lv_label_set_text(title, robot_devices[device_index].name);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    
    // 设置状态图标
    lv_obj_t *status_icon = lv_label_create(current_modal);
    lv_label_set_text(status_icon, "ROBOT");
    lv_obj_align(status_icon, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_text_font(status_icon, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(status_icon, lv_color_hex(0x1396CE), 0);
    
    // 设置状态显示
    lv_obj_t *status_label = lv_label_create(current_modal);
    const char *status_str;
    lv_color_t color;
    
    if (robot_devices[device_index].is_running) {
        status_str = "Clearing";
        color = lv_color_hex(0x1396CE); // 亮蓝色
    } else {
        status_str = "Docking / Charging";
        color = lv_color_hex(0xCCCCCC); // 灰色
    }
    
    lv_label_set_text(status_label, status_str);
    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(status_label, color, 0);
    
    // 保存当前设备索引
    current_device_index = device_index;
    current_layer = PAGE_LAYER_MODAL;
    
    printf("[UI] Modal opened for robot device: %s\n", robot_devices[device_index].name);
}

/* 同步菜单汇总信息 */
void sync_menu_summary(void) {
    // 统计开启的灯光数量
    int on_count = 0;
    int on_index = -1;
    
    for (int i = 0; i < light_device_count; i++) {
        if (light_devices[i].status) {
            on_count++;
            on_index = i;
        }
    }
    
    // 更新一级菜单页的汇总标签
    if (menu_smart_light_summary.title && menu_smart_light_summary.value) {
        if (on_count == 0) {
            // 分支 A：全部熄灭
            lv_label_set_text(menu_smart_light_summary.title, "All Lights");
            lv_label_set_text(menu_smart_light_summary.value, "OFF");
            lv_obj_set_style_text_color(menu_smart_light_summary.value, lv_color_hex(0xCCCCCC), 0);
        } else if (on_count == 1) {
            // 分支 B：仅开一盏
            // 处理名称截断
            const char *name = light_devices[on_index].name;
            char truncated_name[20]; // 限制长度
            strncpy(truncated_name, name, sizeof(truncated_name) - 1);
            truncated_name[sizeof(truncated_name) - 1] = '\0';
            
            lv_label_set_text(menu_smart_light_summary.title, truncated_name);
            lv_label_set_text(menu_smart_light_summary.value, "ON");
            lv_obj_set_style_text_color(menu_smart_light_summary.value, lv_color_hex(0xFCE616), 0);
        } else {
            // 分支 C：开启多盏
            char title[20];
            sprintf(title, "%d Lights", on_count);
            lv_label_set_text(menu_smart_light_summary.title, title);
            lv_label_set_text(menu_smart_light_summary.value, "ON");
            lv_obj_set_style_text_color(menu_smart_light_summary.value, lv_color_hex(0xFCE616), 0);
        }
    }
    
    // 更新 home_light_label（保持兼容）
    bool all_off = (on_count == 0);
    light_status = !all_off;
    if (home_light_label) {
        lv_label_set_text(home_light_label, light_status ? "ON" : "OFF");
        lv_obj_set_style_text_color(home_light_label, 
            light_status ? lv_color_hex(0xFCE616) : lv_color_hex(0xCCCCCC), 0);
    }
}

/* 同步机器人状态到首页 */
void sync_robot_to_home(void) {
    if (home_robot_label) {
        const char *status_str;
        lv_color_t color;
        
        if (robot_devices[0].is_running) {
            status_str = "Clearing";
            color = lv_color_hex(0x1396CE); // 亮蓝色
        } else {
            status_str = "Standby / Charging";
            color = lv_color_hex(0xCCCCCC); // 灰色
        }
        
        lv_label_set_text(home_robot_label, status_str);
        lv_obj_set_style_text_color(home_robot_label, color, 0);
        // 强制重新绘制标签，确保状态及时更新
        lv_obj_invalidate(home_robot_label);
    }
}

/* 起夜模式 */
void night_trip_mode(void) {
    // 开启床头灯、走廊灯、厕所灯
    update_light_state(BEDSIDE_LAMP_INDEX, true);
    update_light_state(HALLWAY_LIGHT_INDEX, true);
    update_light_state(BATHROOM_LIGHT_INDEX, true);
    
    // 停止扫地机器人
    if (robot_devices[0].is_running) {
        robot_devices[0].is_running = false;
        update_robot_summary();
    }
    
    printf("[MODE] Night trip mode activated\n");
}

/* 助眠倒计时回调函数 */
static void sleep_timer_cb(lv_timer_t *timer) {
    // 关闭所有灯光
    for (int i = 0; i < light_device_count; i++) {
        update_light_state(i, false);
    }
    
    // 停止计时器
    lv_timer_del(timer);
    sleep_timer = NULL;
    
    printf("[MODE] Sleep timer triggered - all lights turned off\n");
}

/* 启动助眠倒计时 */
void start_sleep_timer(void) {
    // 停止之前的计时器
    if (sleep_timer) {
        lv_timer_del(sleep_timer);
    }
    
    // 创建新的计时器（10分钟）
    sleep_timer = lv_timer_create(sleep_timer_cb, 10 * 60 * 1000, NULL);
    
    printf("[MODE] Sleep timer started (10 minutes)\n");
}

/* 切换勿扰模式 */
void toggle_dnd_mode(void) {
    dnd_mode = !dnd_mode;
    
    if (dnd_mode) {
        // 开启勿扰模式，锁定扫地机器人状态
        if (robot_devices[0].is_running) {
            robot_devices[0].is_running = false;
            update_robot_summary();
        }
        printf("[MODE] DND mode activated\n");
    } else {
        printf("[MODE] DND mode deactivated\n");
    }
}

/* 同步逻辑状态 */
void sync_logic_state(mode_id_t mode) {
    current_mode = mode;
    if (mode == MODE_LIVING_ROOM) {
        current_layer = PAGE_LAYER_MAIN; // 回到客厅必是一级菜单
        current_page = PAGE_HOME;        // 回到客厅必是首页
    } else {
        current_layer = 0; // 卧室有自己的起始层
    }

    // 刷新 UI 状态机的聚焦逻辑
    ui_manager(current_page);
    update_focus_visual();
}

/* 全屏场景切换 */
void full_scene_switch(mode_id_t target_mode) {
    // 1. 物理拔掉所有悬浮的“零件”（消息框、模态窗口等）
    cleanup_ui_elements();

    // 2. 切换模式容器的显示状态
    if (target_mode == MODE_LIVING_ROOM) {
        lv_obj_clear_flag(living_room_cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(bedroom_cont, LV_OBJ_FLAG_HIDDEN);
        // 切换背景图
        if (ui_background) {
            lv_img_set_src(ui_background, &background01);
        }
    } else {
        lv_obj_add_flag(living_room_cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(bedroom_cont, LV_OBJ_FLAG_HIDDEN);
        if (ui_background) {
            lv_img_set_src(ui_background, &background02);
        }
    }

    // 3. 执行逻辑指针同步
    sync_logic_state(target_mode);
}

/* 切回客厅模式 */
void return_to_living_room_mode(void) {
    full_scene_switch(MODE_LIVING_ROOM);
    
    // 恢复底部导航栏
    lv_obj_t *tab_btns = lv_tabview_get_tab_btns(tabview);
    if (tab_btns) {
        lv_obj_clear_flag(tab_btns, LV_OBJ_FLAG_HIDDEN);
    }
    
    printf("[MODE] BED >> HOME\n");
}

/* 切换到卧室模式 */
void switch_to_bedroom_mode(void) {
    printf("[MODE] Switching to bedroom mode...\n");
    
    full_scene_switch(MODE_BEDROOM);
    
    // 隐藏底部导航栏
    lv_obj_t *tab_btns = lv_tabview_get_tab_btns(tabview);
    if (tab_btns) {
        lv_obj_add_flag(tab_btns, LV_OBJ_FLAG_HIDDEN);
    }
    
    // 清空焦点组，避免与客厅模式的焦点冲突
    if (focus_group) {
        lv_group_remove_all_objs(focus_group);
    }
    
    update_mode_ui();
    
    printf("[MODE] HOME >> BED\n");
    printf("[DEBUG] Actual Page changed to: %d, Layer: %d\n", current_page, current_layer);
}



// 同步首页背景与状态
void sync_home_state(void) {
    if (ui_background == NULL) return;

    if (current_mode == MODE_BEDROOM) {
        lv_img_set_src(ui_background, &background02); // 切换卧室背景
        global_system_mode = 1;
    } else {
        lv_img_set_src(ui_background, &background01); // 切换客厅背景
        global_system_mode = 0;
    }
}

/* 更新机器人状态汇总 */
void update_robot_summary(void) {
    // 更新机器人设备卡片
    for (int i = 0; i < robot_device_count; i++) {
        if (robot_devices[i].status_label) {
            const char *status_str;
            lv_color_t color;
            
            if (robot_devices[i].is_running) {
                status_str = "Clearing";
                color = lv_color_hex(0x1396CE); // 亮蓝色
            } else {
                status_str = "Standby";
                color = lv_color_hex(0xCCCCCC); // 中灰色
            }
            
            lv_label_set_text(robot_devices[i].status_label, status_str);
            lv_obj_set_style_text_color(robot_devices[i].status_label, color, 0);
            // 强制重新绘制标签，确保状态及时更新
            lv_obj_invalidate(robot_devices[i].status_label);
        }
    }
    
    // 更新首页机器人状态
    sync_robot_to_home();
}

/* 更新灯光状态 */
void update_light_state(int id, bool new_state) {
    if (id < 0 || id >= light_device_count) return;
    
    // 更新底层状态
    light_devices[id].status = new_state;
    
    // 更新二级页面对应卡片的状态标签
    if (light_devices[id].card_label) {
        lv_label_set_text(light_devices[id].card_label, new_state ? "ON" : "OFF");
        lv_obj_set_style_text_color(light_devices[id].card_label, 
            new_state ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF0000), 0);
        // 强制重新绘制标签，确保状态及时更新
        lv_obj_invalidate(light_devices[id].card_label);
    }
    
    // 同步菜单汇总信息
    sync_menu_summary();
    
    printf("[SYNC] LIGHT[%d] -> %s\n", id, new_state ? "ON" : "OFF");
}

/* 关闭当前模态窗口 */
void close_current_modal(void) {
    if (current_modal) {
        lv_obj_del(current_modal);
        current_modal = NULL;
    }
    
    if (modal_mask) {
        lv_obj_add_flag(modal_mask, LV_OBJ_FLAG_HIDDEN);
    }
    
    current_device_index = -1;
    current_layer = PAGE_LAYER_SUB;
    
    // 恢复焦点状态
    update_focus_visual();
    
    printf("[UI] Modal closed\n");
}

/* 消息框事件回调函数 */
static void msgbox_event_cb(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    if (obj == current_msgbox) {
        current_msgbox = NULL;
    }
}

/* 清理UI元素 */
void cleanup_ui_elements(void) {
    // 物理删除消息框
    if (current_msgbox != NULL ) {
        lv_obj_del(current_msgbox);
        current_msgbox = NULL ;
    }

    // 物理删除模态窗口
    if (current_modal != NULL ) {
        lv_obj_del(current_modal);
        current_modal = NULL ;
    }

    // 物理删除模态遮罩
    if (modal_mask != NULL ) {
        lv_obj_del(modal_mask);
        modal_mask = NULL ;
    }
    
    current_device_index = -1 ;
}

/* 关闭所有模态窗口 */
void close_all_modals(void) {
    // 1. 如果有弹窗对象（如提示框），在这里删除或隐藏
    // if(msgbox) lv_obj_del(msgbox);

    // 2. 关键：隐藏二级页面的选中/聚焦框
    if (ui_focus_frame != NULL) {
        lv_obj_add_flag(ui_focus_frame, LV_OBJ_FLAG_HIDDEN);
    }

    // 3. 清除 UI 忙碌标志
    pthread_mutex_unlock(&ui_mutex);
    gesture_enabled = true;
    
    printf("[UI] All modals closed, focus frame hidden.\n");
}

/* 清理UI选中状态 */
void clear_ui_selection(void) {
    // 如果你使用的是 LVGL 默认组（Group）
    lv_group_t *default_group = lv_group_get_default();
    if (default_group) {
        lv_group_focus_freeze(default_group, true); // 冻结焦点
        lv_group_set_editing(default_group, false); // 退出编辑模式
    }
    
    // 调用update_focus_visual来更新视觉效果
    update_focus_visual();
}

/* 更新设备卡片UI */
void update_device_cards(void)
{
    // 更新灯光设备卡片
    for (int i = 0; i < light_device_count; i++) {
        if (light_devices[i].card_label) {
            lv_label_set_text(light_devices[i].card_label, light_devices[i].status ? "ON" : "OFF");
            lv_obj_set_style_text_color(light_devices[i].card_label, 
                light_devices[i].status ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF0000), 0);
        }
    }
    
    // 更新机器人设备卡片
    for (int i = 0; i < robot_device_count; i++) {
        if (robot_devices[i].status_label) {
            const char *status_str;
            switch(robot_devices[i].status) {
                case ROBOT_STATUS_STOP:
                    status_str = "STOP";
                    break;
                case ROBOT_STATUS_CLEANING:
                    status_str = "CLEANING";
                    break;
                case ROBOT_STATUS_CHARGING:
                    status_str = "CHARGING";
                    break;
                default:
                    status_str = "UNKNOWN";
            }
            lv_label_set_text(robot_devices[i].status_label, status_str);
            lv_obj_set_style_text_color(robot_devices[i].status_label, 
                robot_devices[i].status == ROBOT_STATUS_STOP ? lv_color_hex(0xFF0000) : 
                robot_devices[i].status == ROBOT_STATUS_CLEANING ? lv_color_hex(0x00FF00) : 
                lv_color_hex(0xFF9500), 0);
        }
    }
    
    // 同步到首页
    update_light_ui();
    update_robot_ui();
}

/* 初始化所有UI */
void ui_init_all(void)
{
    // 初始化互斥锁
    if (pthread_mutex_init(&ui_mutex, NULL) != 0) {
        LOGE("Failed to initialize mutex");
        return;
    }
    
    lv_obj_t *main_scr = lv_scr_act();
    
    // 创建背景图对象
    ui_background = lv_img_create(main_scr);
    lv_img_set_src(ui_background, &background01);
    lv_obj_set_size(ui_background, 800, 480);
    lv_obj_align(ui_background, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(ui_background, 200, 0); // 设置透明度为200
    lv_obj_move_background(ui_background); // 移到背景层
    
    // 日志调试：确认图片指针是否有效
    printf("[UI] Background image address: %p\n", &background01);
    printf("[UI] Background image data address: %p\n", background01.data);
    
    // 初始化场景容器
    living_room_cont = lv_obj_create(main_scr);
    lv_obj_set_size(living_room_cont, 800, 480); // 设为全屏
    lv_obj_set_style_bg_opa(living_room_cont, LV_OPA_TRANSP, 0); // 透明
    lv_obj_set_style_border_width(living_room_cont, 0, 0);

    // 创建卧室大盒子
    bedroom_cont = lv_obj_create(main_scr);
    lv_obj_set_size(bedroom_cont, 800, 480);
    lv_obj_set_style_bg_opa(bedroom_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bedroom_cont, 0, 0);
    lv_obj_add_flag(bedroom_cont, LV_OBJ_FLAG_HIDDEN); // 初始隐藏卧室
    
    tabview = lv_tabview_create(living_room_cont, LV_DIR_BOTTOM, 50);
    lv_obj_set_style_bg_opa(tabview, LV_OPA_TRANSP, 0); // 背景透明
    
    // Customize tabview buttons to match user's mockup
    lv_obj_t *tab_btns = lv_tabview_get_tab_btns(tabview);
    lv_obj_set_style_bg_color(tab_btns, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_pad_all(tab_btns, 0, 0); // No padding
    lv_obj_set_style_text_color(tab_btns, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(tab_btns, &lv_font_montserrat_24, 0);
    
    // Style when a tab is selected: Light purple bg with blue bottom border
    lv_obj_set_style_bg_color(tab_btns, lv_color_hex(0xDCD0E9), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(tab_btns, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_border_side(tab_btns, LV_BORDER_SIDE_BOTTOM, LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_border_color(tab_btns, lv_color_hex(0x1396CE), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_border_width(tab_btns, 6, LV_PART_ITEMS | LV_STATE_CHECKED);
    
    // 创建时钟定时器，每秒更新一次
    lv_timer_create(clock_timer_cb, 1000, NULL);
    
    // 初始化时更新一次时间
    update_home_time();
    
    // Make sure we have enough padding at the top to clear the alert icon
    lv_obj_t *tab_cnt = lv_tabview_get_content(tabview);
    lv_obj_set_style_pad_top(tab_cnt, 0, 0);
    lv_obj_set_style_bg_opa(tab_cnt, LV_OPA_TRANSP, 0); // 内容区域背景透明
    
    // Use text as tab names
    lv_obj_t *tab_home = lv_tabview_add_tab(tabview, "HOME");
    lv_obj_t *tab_light = lv_tabview_add_tab(tabview, "LIGHT");
    lv_obj_t *tab_robot = lv_tabview_add_tab(tabview, "ROBOT");
    lv_obj_t *tab_setting = lv_tabview_add_tab(tabview, "SETTING");
    
    init_home_page(tab_home);
    init_light_page(tab_light);
    init_robot_page(tab_robot);
    init_setting_page(tab_setting);
    
    // Initialize add device page
    init_add_device_page(main_scr);
    
    // Initialize sub pages logic
    init_light_settings(tab_light);
    init_robot_settings(tab_robot);
    init_setting_options(tab_setting);
    
    // 初始化焦点组
    init_focus_group();
    
    // 初始化UI状态
    update_light_ui();
    update_mode_ui();
    update_robot_ui();
    update_device_cards();
    
    // 同步菜单汇总信息
    sync_menu_summary();
    // 同步机器人状态汇总
    update_robot_summary();
    
    // Update home time immediately and set a timer to run it every second
    update_home_time();
    lv_timer_create((lv_timer_cb_t)update_home_time, 1000, NULL);
}
