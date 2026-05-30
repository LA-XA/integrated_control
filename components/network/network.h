#ifndef __NETWORK_H_
#define __NETWORK_H_

#include "esp_err.h"
#include "mqtt_client.h"
#include "sensors.h" // 必须包含，以便识别 sensor_data_t 结构体

// 定义网络状态枚举
typedef enum
{
    NETWORK_STATE_INIT,
    NETWORK_STATE_WIFI_CONNECTING,
    NETWORK_STATE_WIFI_CONNECTED,
    NETWORK_STATE_MQTT_CONNECTED
} network_state_t;

// 网络句柄结构体
typedef struct
{
    network_state_t state;
    esp_mqtt_client_handle_t mqtt_client;
} network_handle_t;

// --- 公开接口 ---

/**
 * @brief 初始化网络（WiFi + MQTT）
 * @param handle 网络句柄指针
 */
esp_err_t network_init(network_handle_t *handle);

/**
 * @brief 将传感器数据发布到 MQTT
 * @param data 传感器数据结构体指针
 */
void mqtt_publish_sensor_data(sensor_data_t *data);

#endif