#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "lvgl/lvgl.h"

/* --------- 屏幕配置 --------- */
#define HOR_RES 800
#define VER_RES 480

/* 建议用部分行缓冲，省内存 */
#define DISP_BUF_LINES 60
#define DISP_BUF_PIXELS (HOR_RES * DISP_BUF_LINES)

/* 白天/夜间时间段（可改） */
#define DAY_START_HOUR   8
#define NIGHT_START_HOUR 22

/* 页面编号：替代魔法数字 */
typedef enum {
    PAGE_HOME = 0,    // 首页
    PAGE_DEVICES,     // 设备控制页（核心：根据模式动态显示）
    PAGE_SETTING,     // 设置页（在这里切换卧室/客厅模式）
    PAGE_NIGHT,       // 夜间模式页面
    PAGE_COUNT
} page_id_t;

/* 系统模式定义 */
typedef enum {
    MODE_BEDROOM = 0, // 卧室模式（默认）
    MODE_LIVING_ROOM  // 客厅模式
} system_mode_t;

/* UI 动作：替代 action=0/1/2/3/4 */
typedef enum {
    ACT_SWIPE_LEFT = 0,
    ACT_SWIPE_RIGHT,
    ACT_ENTER_SUB,     // 进入子页面（如音响控制详情）
    ACT_BACK,
    ACT_TO_NIGHT,
    ACT_WAKEUP
} ui_action_t;

/* 手势定义 */
typedef enum {
    GESTURE_UP = 1, GESTURE_DOWN, GESTURE_LEFT, GESTURE_RIGHT,
    GESTURE_FORWARD, GESTURE_BACKWARD, GESTURE_CW, GESTURE_CCW,
    GESTURE_ENTER
} gesture_t;

/* --------- 全局状态（main.c 定义） --------- */
extern volatile int g_current_gesture;      /* 传感器线程写，主线程读并消费 */
extern int manual_wake_up;                  /* 手动唤醒标志 */
extern page_id_t current_page;              /* 当前页面 */
extern system_mode_t g_active_mode;         /* 当前系统模式 */

/* --------- UI/业务接口 --------- */
void ui_init_all(void);
void ui_manager(page_id_t page);

/* 类似于LVGL心跳 */
uint32_t custom_tick_get(void);

// 新增UI组件接口
void ui_show_gesture_toast(const char *msg); // Toast 提示框
