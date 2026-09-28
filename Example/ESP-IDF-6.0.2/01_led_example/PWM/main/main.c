// #include <stdio.h>
// #include "driver/gpio.h"
// #include "driver/ledc.h"   // PWM 专用库
// #include "unistd.h"

// // 配置
// #define PWM_GPIO 6        // VOUT- 控制引脚
// #define PWM_CH    LEDC_CHANNEL_0
// #define PWM_TIMER LEDC_TIMER_0

// void app_main(void)
// {
//     // 1. 配置 PWM 定时器
//     ledc_timer_config_t timer_conf = {
//         .speed_mode       = LEDC_LOW_SPEED_MODE,
//         .timer_num        = PWM_TIMER,
//         .duty_resolution  = LEDC_TIMER_8_BIT,  // 0~255
//         .freq_hz          = 25000,               // 25kHz
//         .clk_cfg          = LEDC_AUTO_CLK
//     };
//     ledc_timer_config(&timer_conf);

//     // 2. 配置 PWM 通道
//     ledc_channel_config_t chan_conf = {
//         .gpio_num       = PWM_GPIO,
//         .speed_mode     = LEDC_LOW_SPEED_MODE,
//         .channel        = PWM_CH,
//         .timer_sel      = PWM_TIMER,
//         .duty           = 0,  // 初始 0 亮度
//         .hpoint         = 0
//     };
//     ledc_channel_config(&chan_conf);

//     // 3. 呼吸灯循环
//     int duty = 0;
//     int dir = 1;

//     while (1) {
//         duty += dir;
//         if (duty >= 255) dir = -1;
//         if (duty <= 0)   dir = 1;

//         ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CH, duty);
//         ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CH);
        
//         usleep(8000);
//     }
// }

#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// 引脚定义
#define LED_CTRL_GPIO 6  

void app_main(void)
{
    // GPIO 配置结构体
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_CTRL_GPIO), 
        .mode = GPIO_MODE_OUTPUT,                
        .pull_up_en = GPIO_PULLUP_DISABLE,       
        .pull_down_en = GPIO_PULLDOWN_DISABLE,   
        .intr_type = GPIO_INTR_DISABLE         
    };
    // 初始化GPIO配置
    gpio_config(&io_conf);

    // 持续输出高电平
    gpio_set_level(LED_CTRL_GPIO, 1);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// #include <stdio.h>
// #include "driver/ledc.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"

// // 配置
// #define PWM_GPIO    6       
// #define PWM_CH      LEDC_CHANNEL_0
// #define PWM_TIMER   LEDC_TIMER_0

// // ==================== 调亮度 ====================
// #define BRIGHTNESS  128      // 亮度值：0 (灭) ~ 255 (最亮)
// // =======================================================

// void app_main(void)
// {
//     // 1. 配置 PWM 定时器
//     ledc_timer_config_t timer_conf = {
//         .speed_mode       = LEDC_LOW_SPEED_MODE,
//         .timer_num        = PWM_TIMER,
//         .duty_resolution  = LEDC_TIMER_8_BIT,  // 8位分辨率：0~255
//         .freq_hz          = 25000,               // 25kHz 频率（无噪音）
//         .clk_cfg          = LEDC_AUTO_CLK
//     };
//     ledc_timer_config(&timer_conf);

//     // 2. 配置 PWM 通道
//     ledc_channel_config_t chan_conf = {
//         .gpio_num       = PWM_GPIO,
//         .speed_mode     = LEDC_LOW_SPEED_MODE,
//         .channel        = PWM_CH,
//         .timer_sel      = PWM_TIMER,
//         .duty           = 0,
//         .hpoint         = 0
//     };
//     ledc_channel_config(&chan_conf);

//     // 3. 设置亮度并生效
//     ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CH, BRIGHTNESS);
//     ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CH);

//     // 4. 保持运行，灯带常亮
//     while (1) {
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }