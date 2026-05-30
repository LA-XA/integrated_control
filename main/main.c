#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_cpu.h" // 用于获取核心 ID

// 组件头文件
#include "movement.h"
#include "network.h"
#include "camera.h"
#include "sensors.h"

static const char *TAG = "MAIN_APP";

/**
 * 传感器任务：监测环境并上报
 * 运行核心：Core 0 (与网络协议栈同核，因为数据量小，压力不大)
 */
void sensor_report_task(void *pvParameters)
{
    sensor_data_t s_data;
    sensors_init();

    ESP_LOGI("SENSOR", "传感器任务运行在核心: %d", esp_cpu_get_core_id());

    while (1)
    {
        if (sensors_read(&s_data) == ESP_OK)
        {
            mqtt_publish_sensor_data(&s_data);
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/**
 * 视频任务声明
 * 我们将把它“钉”在 Core 1 上
 */
extern void video_stream_task(void *pvParameters);

void app_main(void)
{
    // 1. 初始化存储
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "===== ESP32-S3 双核系统启动 =====");

    // 2. 初始化硬件（电机、舵机、摄像头）
    movement_init();
    camera_init();

    // 3. 启动联网（WiFi 和 MQTT 会默认跑在 Core 0）
    static network_handle_t net_handle;
    network_init(&net_handle);

    // 4. 创建双核并发任务

    // 【核心 1】：全力负责视频采集与传输（计算密集型）
    xTaskCreatePinnedToCore(
        video_stream_task,
        "video_task",
        8192,
        NULL,
        5,
        NULL,
        1 // 绑定到 Core 1
    );

    // 【核心 0】：负责传感器读取与上报（与 WiFi 同核心，低频任务）
    xTaskCreatePinnedToCore(
        sensor_report_task,
        "sensor_task",
        4096,
        NULL,
        3,
        NULL,
        0 // 绑定到 Core 0
    );

    ESP_LOGI(TAG, "任务分配完成：视频(Core 1)，通讯/感测(Core 0)");
}