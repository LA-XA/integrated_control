#include "camera.h"
#include "esp_log.h"
#include "esp_camera.h" // 确保包含了官方驱动头文件
#include "esp_psram.h"
#include "lwip/sockets.h" // 必须包含，否则无法使用 socket
#include "arpa/inet.h"    // 必须包含，用于处理 IP 地址
static const char *TAG = "CAMERA_MODULE";

// --- 引脚定义 ---
#define CAM_PIN_PWDN 10
#define CAM_PIN_RESET 9
#define CAM_PIN_XCLK -1 // 如果报错 Camera probe failed, 请改为具体的引脚号
#define CAM_PIN_SIOD 39
#define CAM_PIN_SIOC 38

#define CAM_PIN_D7 18
#define CAM_PIN_D6 17
#define CAM_PIN_D5 16
#define CAM_PIN_D4 15
#define CAM_PIN_D3 7
#define CAM_PIN_D2 6
#define CAM_PIN_D1 5
#define CAM_PIN_D0 4

#define CAM_PIN_VSYNC 14
#define CAM_PIN_HREF 3
#define CAM_PIN_PCLK 45

// --- 初始化函数 ---
esp_err_t camera_init(void) // 既然去掉了 _v，函数名也建议统一
{
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_1;
    config.ledc_timer = LEDC_TIMER_0;

    config.pin_d0 = CAM_PIN_D0;
    config.pin_d1 = CAM_PIN_D1;
    config.pin_d2 = CAM_PIN_D2;
    config.pin_d3 = CAM_PIN_D3;
    config.pin_d4 = CAM_PIN_D4;
    config.pin_d5 = CAM_PIN_D5;
    config.pin_d6 = CAM_PIN_D6;
    config.pin_d7 = CAM_PIN_D7;
    config.pin_xclk = CAM_PIN_XCLK;
    config.pin_pclk = CAM_PIN_PCLK;
    config.pin_vsync = CAM_PIN_VSYNC;
    config.pin_href = CAM_PIN_HREF;
    config.pin_sccb_sda = CAM_PIN_SIOD;
    config.pin_sccb_scl = CAM_PIN_SIOC;
    config.pin_pwdn = CAM_PIN_PWDN;
    config.pin_reset = CAM_PIN_RESET;

    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

    // 图像分辨率与缓存配置
    // 老师提醒：ESP32-S3 必须在 menuconfig 中开启 PSRAM 支持，psramFound() 才会返回 true
#if CONFIG_SPIRAM
    // 检查 PSRAM 是否初始化成功并可以获取大小
    if (esp_psram_get_size() > 0)
    {
        config.frame_size = FRAMESIZE_VGA; // 640x480
        config.jpeg_quality = 12;
        config.fb_count = 2;
        ESP_LOGI(TAG, "检测到 PSRAM, 开启高分辨率双缓冲");
    }
    else
#endif
    {
        config.frame_size = FRAMESIZE_QVGA; // 320x240
        config.jpeg_quality = 12;
        config.fb_count = 1;
        ESP_LOGW(TAG, "未检测到 PSRAM，切换到低分辨率单缓冲模式");
    }

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "摄像头初始化失败: 0x%x", err);
        return err;
    }
    ESP_LOGI(TAG, "摄像头初始化成功！");
    return ESP_OK;
}

// 抓取图像
camera_fb_t *camera_take_picture(void)
{
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb)
    {
        ESP_LOGE(TAG, "获取帧缓冲失败");
        return NULL;
    }
    return fb;
}

// 释放内存
void camera_return_fb(camera_fb_t *fb)
{
    if (fb)
    {
        esp_camera_fb_return(fb);
    }
}

// 发送单帧图像的逻辑
static esp_err_t camera_send_frame(int socket_fd, camera_fb_t *fb)
{
    if (!fb)
        return ESP_FAIL;
    size_t fb_len = fb->len;
    // 先发长度 (4字节)
    if (write(socket_fd, &fb_len, sizeof(size_t)) < 0)
        return ESP_FAIL;
    // 再发数据
    size_t total_sent = 0;
    while (total_sent < fb->len)
    {
        int sent = send(socket_fd, fb->buf + total_sent, fb->len - total_sent, 0);
        if (sent < 0)
            return ESP_FAIL;
        total_sent += sent;
    }
    return ESP_OK;
}

// 这就是 main.c 在寻找的“失踪”函数！
void video_stream_task(void *pvParameters)
{
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000);                         // 必须和 Python 端一致
    inet_pton(AF_INET, "10.202.228.85", &serv_addr.sin_addr); // !!!这里一定要改写成你电脑的 IP!!!

    while (1)
    {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            ESP_LOGW(TAG, "视频服务器连接失败，等待重试...");
            close(sock);
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        ESP_LOGI(TAG, "视频传输通道已建立");
        while (1)
        {
            camera_fb_t *fb = camera_take_picture();
            if (fb)
            {
                if (camera_send_frame(sock, fb) != ESP_OK)
                {
                    camera_return_fb(fb);
                    break;
                }
                camera_return_fb(fb);
            }
            vTaskDelay(pdMS_TO_TICKS(40)); // 约 25 帧/秒
        }
        close(sock);
    }
}