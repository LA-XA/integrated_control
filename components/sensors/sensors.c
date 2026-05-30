#include "sensors.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h" // 新版头文件
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

static const char *TAG = "SENSORS";

// 定义句柄（门禁卡）
static adc_oneshot_unit_handle_t adc1_handle;

void sensors_init(void)
{
    // 1. 配置 ADC 单元 (ADC_UNIT_1)
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    // 2. 配置通道 (GPIO 2 对应 ADC_CHANNEL_1)
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12, // 5.x 版本建议用 DB_12 替换 DB_11，测量范围更宽
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_1, &config));

    ESP_LOGI(TAG, "新版 ADC Oneshot 初始化完成 (GPIO 2)");
}

esp_err_t sensors_read(sensor_data_t *data)
{
    int raw_out;
    // 3. 读取原生数据
    esp_err_t ret = adc_oneshot_read(adc1_handle, ADC_CHANNEL_1, &raw_out);
    if (ret == ESP_OK)
    {
        data->light_intensity = raw_out;
    }

    // 模拟 DHT11 数据（等待你接入驱动）
    data->temperature = 26.0;
    data->humidity = 55.0;

    return ret;
}