#ifndef __SENSORS_H_
#define __SENSORS_H_

#include "esp_err.h"

// 传感器数据结构体
typedef struct
{
    float temperature;
    float humidity;
    int light_intensity;
} sensor_data_t;

void sensors_init(void);
esp_err_t sensors_read(sensor_data_t *data);

#endif