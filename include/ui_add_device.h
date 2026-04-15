#ifndef UI_ADD_DEVICE_H
#define UI_ADD_DEVICE_H

#include "lvgl/lvgl.h"

#define MAX_DEVICES 10

// 设备结构体
typedef struct {
    char name[32];
    char id[32];
} device_t;

// 外部设备列表声明
extern device_t devices[MAX_DEVICES];
extern int device_count;

// 函数声明
void init_add_device_page(lv_obj_t *parent);
void show_add_device_page(void);
void hide_add_device_page(void);

#endif // UI_ADD_DEVICE_H
