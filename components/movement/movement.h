#ifndef __MOVEMENT_H_
#define __MOVEMENT_H_

#include "esp_err.h"

// 初始化运动模块（GPIO 和 PWM）
esp_err_t movement_init(void);

// 控制逻辑
void movement_forward(void);     // 前进
void movement_backward(void);    // 后退
void movement_stop(void);        // 停止
void servo_set_angle(int angle); // 设置舵机角度 (0-180)

#endif