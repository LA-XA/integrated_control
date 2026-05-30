#include <string.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "network.h"
#include "movement.h"

static const char *TAG = "NETWORK_MODULE";

// 老师的技术细节：使用静态全局变量，方便在任何地方调用发布函数
static esp_mqtt_client_handle_t global_mqtt_client = NULL;
static network_handle_t *global_handle = NULL;

// 1. WiFi 事件处理回调
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGW(TAG, "WiFi 掉线，尝试重连...");
        global_handle->state = NETWORK_STATE_WIFI_CONNECTING;
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "获取到 IP 地址: " IPSTR, IP2STR(&event->ip_info.ip));
        global_handle->state = NETWORK_STATE_WIFI_CONNECTED;
    }
}

// 2. MQTT 事件处理回调
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT 服务器连接成功");
        global_handle->state = NETWORK_STATE_MQTT_CONNECTED;
        // 连上后立即订阅控制主题
        esp_mqtt_client_subscribe(event->client, "/topic/control", 0);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT 服务器已断开");
        global_handle->state = NETWORK_STATE_WIFI_CONNECTED;
        break;

    case MQTT_EVENT_DATA:
        // 收到指令，直接根据第一个字符判断（更高效）
        if (event->data_len > 0)
        {
            char cmd = event->data[0];
            ESP_LOGI(TAG, "收到 MQTT 指令: %c", cmd);
            if (cmd == 'F')
                movement_forward();
            else if (cmd == 'B')
                movement_backward();
            else if (cmd == 'S')
                movement_stop();
        }
        break;

    default:
        break;
    }
}

// 3. 公开函数：发布传感器数据
void mqtt_publish_sensor_data(sensor_data_t *data)
{
    // 只有当 MQTT 真正连接成功时才发布
    if (global_mqtt_client != NULL && global_handle->state == NETWORK_STATE_MQTT_CONNECTED)
    {
        char payload[128];
        // 格式化为标准 JSON 字符串
        sprintf(payload, "{\"temp\":%.1f, \"humi\":%.1f, \"light\":%d}",
                data->temperature, data->humidity, data->light_intensity);

        esp_mqtt_client_publish(global_mqtt_client, "/car/sensors", payload, 0, 1, 0);
    }
}

// 4. 网络初始化总入口
esp_err_t network_init(network_handle_t *handle)
{
    global_handle = handle;
    handle->state = NETWORK_STATE_INIT;

    // A. 基础协议栈初始化
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // B. WiFi 初始化
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 注册 WiFi 和 IP 事件
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "jiabao666",
            .password = "88888tjtj",
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // C. MQTT 初始化
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://10.202.228.85:1883",
    };

    handle->mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    global_mqtt_client = handle->mqtt_client; // 赋值给全局变量

    esp_mqtt_client_register_event(handle->mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(handle->mqtt_client);

    return ESP_OK;
}