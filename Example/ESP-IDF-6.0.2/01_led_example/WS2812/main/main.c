/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h" 

// Set to 1 to use DMA for driving the LED strip, 0 otherwise
// Please note the RMT DMA feature is only available on chips e.g. ESP32-S3/P4
#define LED_STRIP_USE_DMA  0

#if LED_STRIP_USE_DMA
// Numbers of the LED in the strip
#define LED_STRIP_LED_COUNT 256
#define LED_STRIP_MEMORY_BLOCK_WORDS 1024 // this determines the DMA block size
#else
// Numbers of the LED in the strip
#define LED_STRIP_LED_COUNT 160
#define LED_STRIP_MEMORY_BLOCK_WORDS 0 // let the driver choose a proper memory block size automatically
#endif // LED_STRIP_USE_DMA

// GPIO assignment
#define LED_STRIP_GPIO_PIN  2

// 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define LED_STRIP_RMT_RES_HZ  (10 * 1000 * 1000)

static const char *TAG = "example";

led_strip_handle_t configure_led(void)
{
    // LED strip general initialization, according to your led board design
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_GPIO_PIN, // The GPIO that connected to the LED strip's data line
        .max_leds = LED_STRIP_LED_COUNT,      // The number of LEDs in the strip,
        .led_model = LED_MODEL_WS2812,        // LED strip model
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB, // The color order of the strip: GRB
        .flags = {
            .invert_out = false, // don't invert the output signal
        }
    };

    // LED strip backend configuration: RMT
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,        // different clock source can lead to different power consumption
        .resolution_hz = LED_STRIP_RMT_RES_HZ, // RMT counter clock frequency
        .mem_block_symbols = LED_STRIP_MEMORY_BLOCK_WORDS, // the memory block size used by the RMT channel
        .flags = {
            .with_dma = LED_STRIP_USE_DMA,     // Using DMA can improve performance when driving more LEDs
        }
    };

    // LED Strip object handle
    led_strip_handle_t led_strip;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    ESP_LOGI(TAG, "Created LED strip object with RMT backend");
    return led_strip;
}

void set_led_color(led_strip_handle_t led, uint8_t r, uint8_t g, uint8_t b)
{
    // Turn on all the lights in a loop
    for (int i = 0; i < LED_STRIP_LED_COUNT; i++) {
        led_strip_set_pixel(led, i, r, g, b);
    }
    led_strip_refresh(led);
}

void app_main(void)
{

    gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << GPIO_NUM_6), 
            .mode = GPIO_MODE_OUTPUT,               
            .pull_up_en = GPIO_PULLUP_DISABLE,      
            .pull_down_en = GPIO_PULLDOWN_DISABLE,  
            .intr_type = GPIO_INTR_DISABLE           
        };
    gpio_config(&io_conf);

    gpio_set_level(GPIO_NUM_6, 1);

    led_strip_handle_t led_strip = configure_led();
    bool led_on_off = false;

    ESP_LOGI(TAG, "Start blinking LED strip");
#if 0
    while (1) {
        if (led_on_off) {
            /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
            for (int i = 0; i < LED_STRIP_LED_COUNT; i++) {
                ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 255,255,255));
            }
            /* Refresh the strip to send data */
            ESP_ERROR_CHECK(led_strip_refresh(led_strip));
            ESP_LOGI(TAG, "LED ON!");
        } else {
            /* Set all LED off to clear all pixels */
            ESP_ERROR_CHECK(led_strip_clear(led_strip));
            ESP_LOGI(TAG, "LED OFF!");
        }

        led_on_off = !led_on_off;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
#endif

#if 1
while (1) {
        // RED
        ESP_LOGI(TAG, "RED");
        set_led_color(led_strip, 255, 0, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));

        // GREEN
        ESP_LOGI(TAG, "GREEN");
        set_led_color(led_strip, 0, 255, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));

        // BLUE
        ESP_LOGI(TAG, "BLUE");
        set_led_color(led_strip, 0, 0, 255);
        vTaskDelay(pdMS_TO_TICKS(1000));

        // YELLOW
        ESP_LOGI(TAG, "YELLOW");
        set_led_color(led_strip, 255, 255, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));

        // PURPLE
        ESP_LOGI(TAG, "PURPLE");
        set_led_color(led_strip, 255, 0, 255);
        vTaskDelay(pdMS_TO_TICKS(1000));

        // CYAN
        ESP_LOGI(TAG, "CYAN");
        set_led_color(led_strip, 0, 255, 255);
        vTaskDelay(pdMS_TO_TICKS(1000));

        // WHITE
        ESP_LOGI(TAG, "WHITE");
        set_led_color(led_strip, 255, 255, 255);
        vTaskDelay(pdMS_TO_TICKS(1000));

        // OFF
        ESP_LOGI(TAG, "OFF");
        set_led_color(led_strip, 0, 0, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
#endif
}
