#include <stdio.h>
#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"
#include "driver/gpio.h"

#define WS2812_GPIO_PIN  2 // Light strip control pins
#define WS2812_PHYSICAL_NUM 12 // Number of physical LED segments：1428*1m -> 12，1428*5m -> 60
#define WS2812_LOGICAL_NUM  (WS2812_PHYSICAL_NUM * 2) 

#define RMT_RESOLUTION_HZ 10000000

static const char *TAG = "WS2812";

// ------------------- RMT -------------------
static rmt_channel_handle_t tx_channel = NULL;
static rmt_encoder_handle_t led_encoder = NULL;

// WS2812
#define WS2812_T0H_NS  300
#define WS2812_T0L_NS  800
#define WS2812_T1H_NS  800
#define WS2812_T1L_NS  800
#define WS2812_RESET_US 300

// ------------------- Init -------------------
void ws2812_init(void) {

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << WS2812_GPIO_PIN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_ERROR_CHECK(gpio_set_drive_capability(WS2812_GPIO_PIN, GPIO_DRIVE_CAP_3));

    rmt_tx_channel_config_t tx_chan_config = {
        .gpio_num = WS2812_GPIO_PIN,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = RMT_RESOLUTION_HZ,
        .mem_block_symbols = 128,
        .trans_queue_depth = 4,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &tx_channel));

    rmt_bytes_encoder_config_t bytes_encoder_config = {
        .bit0 = {
            .duration0 = WS2812_T0H_NS / (1000000000 / RMT_RESOLUTION_HZ),
            .level0 = 1,
            .duration1 = WS2812_T0L_NS / (1000000000 / RMT_RESOLUTION_HZ),
            .level1 = 0,
        },
        .bit1 = {
            .duration0 = WS2812_T1H_NS / (1000000000 / RMT_RESOLUTION_HZ),
            .level0 = 1,
            .duration1 = WS2812_T1L_NS / (1000000000 / RMT_RESOLUTION_HZ),
            .level1 = 0,
        },
        .flags.msb_first = 1,
    };
    ESP_ERROR_CHECK(rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder));

    ESP_ERROR_CHECK(rmt_enable(tx_channel));
    ESP_LOGI(TAG, "WS2812 initialized (Dual-chip mode)");
}

// r, g, b, w1, w2
void WS2812_show(uint8_t r, uint8_t g, uint8_t b, uint8_t w1, uint8_t w2) {
    uint8_t led_data[WS2812_LOGICAL_NUM * 3]; 

    for (int i = 0; i < WS2812_PHYSICAL_NUM; i++) {
        int idx_chip1 = i * 2 * 3; // The starting position of the first chip (RGB)
        int idx_chip2 = idx_chip1 + 3; // The starting position of the second chip (W1W2)

        led_data[idx_chip1 + 0] = r;
        led_data[idx_chip1 + 1] = g;
        led_data[idx_chip1 + 2] = b;

        led_data[idx_chip2 + 0] = w1;
        led_data[idx_chip2 + 1] = w2;
        led_data[idx_chip2 + 2] = 0; // Fill in 0 for the idle channel
    }

    rmt_transmit_config_t tx_config = {
        .loop_count = 0,
    };

    // send data
    ESP_ERROR_CHECK(rmt_transmit(tx_channel, led_encoder, led_data, sizeof(led_data), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(tx_channel, portMAX_DELAY));
    esp_rom_delay_us(WS2812_RESET_US);
}

// ------------------- Test-------------------
void app_main(void) {
    ws2812_init();

    while (1) {
        ESP_LOGI(TAG, "Test 1: Only RGB (Red)");
        WS2812_show(255, 0, 0, 0, 0); // RED
        vTaskDelay(pdMS_TO_TICKS(2000));
        ESP_LOGI(TAG, "Test 2: Only RGB (Green)");
        WS2812_show(0, 255, 0, 0, 0); // GREEN
        vTaskDelay(pdMS_TO_TICKS(2000));
        ESP_LOGI(TAG, "Test 3: Only RGB (Blue)");
        WS2812_show(0, 0, 255, 0, 0); // BLUE
        vTaskDelay(pdMS_TO_TICKS(2000));

        ESP_LOGI(TAG, "Test 4: Only W1");
        WS2812_show(0, 0, 0, 255, 0); // W1
        vTaskDelay(pdMS_TO_TICKS(2000));

        ESP_LOGI(TAG, "Test 5: Only W2");
        WS2812_show(0, 0, 0, 0, 255); // W2
        vTaskDelay(pdMS_TO_TICKS(2000));

        ESP_LOGI(TAG, "Test 6: All On");
        WS2812_show(255, 255, 255, 255, 255); // ALL
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}