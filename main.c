#include "ui_pages.h"

#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"

#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

/* 显示分辨率和缓冲区大小定义 */
#define HOR_RES 800
#define VER_RES 480
#define DISP_BUF_PIXELS (HOR_RES * VER_RES / 10)

/* --------- 全局状态定义 --------- */
volatile int g_current_gesture = 0;

/* --------- LV_TICK_CUSTOM：提供 custom_tick_get() ---------
   lv_conf.h: LV_TICK_CUSTOM_SYS_TIME_EXPR (custom_tick_get())
*/
uint32_t custom_tick_get(void)
{
    static uint64_t start_ms = 0;

    struct timeval tv;
    gettimeofday(&tv, NULL);

    uint64_t now_ms = (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)tv.tv_usec / 1000ULL;
    if (start_ms == 0) start_ms = now_ms;

    return (uint32_t)(now_ms - start_ms);
}

/* --------- 手势线程：读取 /dev/IIC_drv --------- */
static void *gesture_thread_func(void *arg)
{
    (void)arg;

    int fd = open("/dev/IIC_drv", O_RDWR);
    if (fd < 0) {
        perror("Failed to open /dev/IIC_drv");
        printf("[Gesture] ERROR: Failed to open /dev/IIC_drv\n");
        return NULL;
    }
    printf("[Gesture] /dev/IIC_drv opened successfully, fd=%d\n", fd);

    unsigned char buf = 0;
    unsigned char last_valid = 0;
    while (1) {
        int n = read(fd, &buf, 1);
        if (n > 0) {
            // 过滤无效值：255表示无数据，0表示无效
            if (buf >= 1 && buf <= 9 && buf != 255) {
                // 读取到有效手势时，只要和上一次不一样，就打印一次底层的识别，然后持续输出给消费端
                g_current_gesture = (int)buf;
                if (buf != last_valid) {
                    last_valid = buf;
                    printf("[Gesture] Valid gesture detected: %d\n", g_current_gesture);
                }
            } else {
                g_current_gesture = 0; // 手势离开/结束后清零
                last_valid = 0;
            }
        }
        usleep(30000); /* 30ms - 更快的响应 */
    }

    close(fd);
    return NULL;
}

/* --------- 消费手势：边沿触发 + 消费清零 --------- */
static int consume_gesture(void)
{
    static int last_handled = 0;
    int g = g_current_gesture;

    // 当手势变为0时，重置状态可以接受下一次手势
    if (g == 0) {
        last_handled = 0;
        return 0;
    }

    // 同一个连续手势只向 LVGL 通知一次（边沿触发）
    if (g == last_handled) {
        return 0;
    }

    printf("[consume_gesture] Gesture consumed: %d\n", g);
    last_handled = g;
    g_current_gesture = 0; // 消费后清零

    return g;
}

/* --------- 手势 -> 状态机 --------- */
int main(void)
{
    lv_init();

    /* Display */
    fbdev_init();
    static lv_color_t buf[DISP_BUF_PIXELS];
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_BUF_PIXELS);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &disp_buf;
    disp_drv.flush_cb = fbdev_flush;
    disp_drv.hor_res = HOR_RES;
    disp_drv.ver_res = VER_RES;
    lv_disp_drv_register(&disp_drv);

    /* Input */
    evdev_init();
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = evdev_read;
    lv_indev_drv_register(&indev_drv);

    /* UI init */
    ui_init_all();

    /* 开启手势线程 */
    pthread_t g_tid;
    printf("=== UI Init Done, Sensor Start ===\n");
    pthread_create(&g_tid, NULL, gesture_thread_func, NULL);
    pthread_detach(g_tid);

    while (1) {
        int g = consume_gesture();
        // 处理手势状态
        if (g != 0) {
            handle_gesture((gesture_t)g);
        }

        lv_timer_handler();
        usleep(5000);
    }

    return 0;
}
