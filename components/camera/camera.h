#ifndef __CAMERA_V_H_
#define __CAMERA_V_H_

#include "esp_camera.h"

// 初始化摄像头硬件
esp_err_t camera_init(void);

// 抓取一帧图像
camera_fb_t *camera_take_picture(void);

// 释放图像内存（非常重要，防止内存泄漏）
void camera_return_fb(camera_fb_t *fb);

void video_stream_task(void *pvParameters);
#endif