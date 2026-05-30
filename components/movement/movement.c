#include "sdkconfig.h"
#include "network.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_log.h" // 新增这一行！解决 ESP_LOGI 未定义问题
#include "movement.h"

#define SERVO_GPIO 13
#define MOTOR_IN1_GPIO 11
#define MOTOR_IN2_GPIO 12

// 深度讲解：LEDC 的配置分为两步
esp_err_t movement_init(void)
{
    // 1. 定时器配置：设置频率为 50Hz（舵机标准频率）
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_13_BIT, // 2^13 = 8192 级精度
        .freq_hz = 50,                        // 50Hz
        .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&ledc_timer);

    // 2. 通道配置：把定时器绑定到 GPIO 13 上
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = SERVO_GPIO,
        .duty = 0,
        .hpoint = 0};
    ledc_channel_config(&ledc_channel);

    // 3. 普通 GPIO 配置（用于直流电机）
    gpio_reset_pin(MOTOR_IN1_GPIO);
    gpio_set_direction(MOTOR_IN1_GPIO, GPIO_MODE_OUTPUT);
    gpio_reset_pin(MOTOR_IN2_GPIO);
    gpio_set_direction(MOTOR_IN2_GPIO, GPIO_MODE_OUTPUT);

    return ESP_OK;
}
// --- 直流电机控制 ---

void movement_forward(void)
{
    gpio_set_level(MOTOR_IN1_GPIO, 1);
    gpio_set_level(MOTOR_IN2_GPIO, 0);
    ESP_LOGI("MOVE", "电机状态：前进");
}

void movement_backward(void)
{
    gpio_set_level(MOTOR_IN1_GPIO, 0);
    gpio_set_level(MOTOR_IN2_GPIO, 1);
    ESP_LOGI("MOVE", "电机状态：后退");
}

void movement_stop(void)
{
    gpio_set_level(MOTOR_IN1_GPIO, 0);
    gpio_set_level(MOTOR_IN2_GPIO, 0);
    ESP_LOGI("MOVE", "电机状态：停止");
}

// --- 舵机角度控制 ---

/**
 * @brief 设置舵机角度 (0-180度)
 * 计算逻辑复习：
 * 50Hz 周期 = 20ms
 * 13位精度 = 8192 份 (2^13)
 * 舵机 0.5ms(0度) -> (0.5/20)*8192 = 205
 * 舵机 2.5ms(180度) -> (2.5/20)*8192 = 1024
 */
void servo_set_angle(int angle)
{
    if (angle < 0)
        angle = 0;
    if (angle > 180)
        angle = 180;

    // 线性映射公式：Duty = MinDuty + (angle/180) * (MaxDuty - MinDuty)
    uint32_t duty = 205 + (angle * (1024 - 205) / 180);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    ESP_LOGI("SERVO", "舵机角度设置为: %d, 对应占空比: %lu", angle, duty);
}