#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include "ui_pages.h" // 引用 update_home_time 以便同步 UI

// 日志宏定义
#define LOG_TAG "[SmartHome]"

// 如果不需要详细过程，可以将下面这个设为 0
#define DEBUG_MODE 1

// 颜色代码
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define LOGI(fmt, ...) printf(ANSI_COLOR_GREEN LOG_TAG " [INFO] " fmt ANSI_COLOR_RESET "\n", ##__VA_ARGS__)
#define LOGE(fmt, ...) printf(ANSI_COLOR_RED LOG_TAG " [ERROR] " fmt ANSI_COLOR_RESET "\n", ##__VA_ARGS__)
#define LOGD(fmt, ...) if(DEBUG_MODE) printf(LOG_TAG " [DEBUG] " fmt "\n", ##__VA_ARGS__)

#define TIME_SERVER_IP   "47.118.21.238" // 云服务器的公网IP
#define TIME_SERVER_PORT 8080        // 确保端口与 server_time.c 一致

/**
 * 核心功能：通过 TCP 从服务器获取 JSON 时间戳并更新 Linux 系统时钟
 * 返回值：0 成功, -1 失败
 */
int sync_system_time_from_server(void) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[512] = {0};
    // 构建标准的 HTTP GET 请求，匹配你的 sever_time.c 逻辑
    const char *http_request = "GET /time HTTP/1.1\r\nHost: 47.118.21.238\r\nConnection: close\r\n\r\n";

    LOGI("Syncing time with Cloud Server...");

    // 1. 物理层检查
    if (check_wired_network_status() == 0) {
        LOGE("Eth0 carrier down. Check cable!");
        return -1;
    }

    // 2. 创建 Socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        LOGE("Socket creation failed: %s", strerror(errno));
        return -1;
    }

    // 设置接收超时（2秒），防止网络不通时卡死 LVGL 界面
    struct timeval timeout = {2, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(TIME_SERVER_PORT);

    if (inet_pton(AF_INET, TIME_SERVER_IP, &serv_addr.sin_addr) <= 0) {
        LOGE("Invalid address: %s", strerror(errno));
        close(sock);
        return -1;
    }

    // 1. 设置 socket 为非阻塞模式
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    LOGD("Attempting non-blocking connect to %s...", TIME_SERVER_IP);

    // 2. 发起连接
    int res = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    
    if (res < 0 && errno != EINPROGRESS) {
        LOGE("Connect immediate error: %s", strerror(errno));
        close(sock);
        return -1;
    }

    // 3. 使用 select 等待 2 秒超时
    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(sock, &fdset);
    struct timeval tv = {2, 0}; // 2秒超时

    res = select(sock + 1, NULL, &fdset, NULL, &tv);

    if (res <= 0) {
        LOGE("Connection timeout (Server unreachable)");
        close(sock);
        return -1; // 2秒到了还没连上，直接放弃，不卡死UI
    }

    // 4. 恢复为阻塞模式以便后续 recv
    fcntl(sock, F_SETFL, flags);

    // 4. 发送 HTTP 请求
    send(sock, http_request, strlen(http_request), 0);

    // 5. 读取服务器返回的 JSON 数据
    int valread = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (valread <= 0) {
        LOGE("No response from server");
        close(sock);
        return -1;
    }
    
    buffer[valread] = '\0';
    LOGD("Raw Response: %s", buffer); // 打印服务器返回的JSON

    // 6. 解析 JSON 字段 "epoch"
    char *epoch_ptr = strstr(buffer, "\"epoch\":");
    if (epoch_ptr) {
        long remote_time = atol(epoch_ptr + 8); // 提取时间戳数字
        
        // 关键点：校准为北京时间 (UTC+8)
        remote_time += 28800;

        // 7. 使用 clock_settime 设置系统级时间
        struct timespec ts;
        ts.tv_sec = remote_time;
        ts.tv_nsec = 0;
        
        if (clock_settime(CLOCK_REALTIME, &ts) == 0) {
            // 8. 成功后立即调用你已有的 UI 刷新函数
            update_home_time();
            LOGI("Time Sync Success. (CST: %ld)", remote_time);
            close(sock);
            return 0;
        } else {
            LOGE("Failed to set system time: %s", strerror(errno));
        }
    } else {
        LOGE("No epoch field in response");
    }

    close(sock);
    return -1;
}