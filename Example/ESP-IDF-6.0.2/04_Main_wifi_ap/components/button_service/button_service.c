#include "button_service.h"

#include <stdbool.h>

#include "audio_service.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BOOT_BUTTON_GPIO GPIO_NUM_0
#define LONG_PRESS_MS    800
#define POLL_MS          20

static TaskHandle_t s_task;

static void button_task(void *arg)
{
    (void)arg;
    bool was_pressed = false;
    bool long_sent = false;
    uint32_t press_ms = 0;

    while (1) {
        bool pressed = gpio_get_level(BOOT_BUTTON_GPIO) == 0;

        if (pressed) {
            press_ms += POLL_MS;
            if (!long_sent && press_ms >= LONG_PRESS_MS) {
                audio_service_record_5s_and_play();
                long_sent = true;
            }
        } else if (was_pressed) {
            if (!long_sent) {
                audio_service_toggle_music();
            }
            press_ms = 0;
            long_sent = false;
        }

        was_pressed = pressed;
        vTaskDelay(pdMS_TO_TICKS(POLL_MS));
    }
}

esp_err_t button_service_init(void)
{
    if (s_task) {
        return ESP_OK;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << BOOT_BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    return xTaskCreate(button_task, "button_task", 3072, NULL, 5, &s_task) == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}
