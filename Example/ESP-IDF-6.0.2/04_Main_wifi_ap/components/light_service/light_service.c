#include "light_service.h"

#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/rmt_tx.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "light_service";

#define RMT_RESOLUTION_HZ       10000000
#define LIGHT_POWER_ENABLE_GPIO GPIO_NUM_6
#define LIGHT_MAX_COUNT         2048
#define WS2812_RESET_US         300
#define WS2812_T0H_NS           350
#define WS2812_T0L_NS           900
#define WS2812_T1H_NS           900
#define WS2812_T1L_NS           350

static rmt_channel_handle_t s_tx_channel;
static rmt_encoder_handle_t s_led_encoder;
static SemaphoreHandle_t s_mutex;
static uint8_t s_gpio = LIGHT_DEFAULT_GPIO;
static uint16_t s_count = LIGHT_DEFAULT_COUNT;
static uint8_t s_r;
static uint8_t s_g;
static uint8_t s_b;
static bool s_ready;
static char s_status[80] = "light not initialized";

static void set_status(const char *status)
{
    strlcpy(s_status, status, sizeof(s_status));
}

static bool valid_gpio(uint8_t gpio)
{
    int gpio_num = gpio;
    if (!GPIO_IS_VALID_OUTPUT_GPIO(gpio_num)) {
        return false;
    }
    return gpio != 0 && gpio != 6 && gpio != 7 && gpio != 8 && gpio != 9 &&
           gpio != 10 && gpio != 14 && gpio != 15 && gpio != 17 && gpio != 18 &&
           gpio != 21 && !(gpio >= 22 && gpio <= 37) && gpio != 43 &&
           gpio != 44 && gpio != 45 && gpio != 46;
}

static void enable_power(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << LIGHT_POWER_ENABLE_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(LIGHT_POWER_ENABLE_GPIO, 1);
}

static void deinit_driver(void)
{
    if (s_tx_channel) {
        rmt_disable(s_tx_channel);
    }
    if (s_led_encoder) {
        rmt_del_encoder(s_led_encoder);
        s_led_encoder = NULL;
    }
    if (s_tx_channel) {
        rmt_del_channel(s_tx_channel);
        s_tx_channel = NULL;
    }
    s_ready = false;
}

static esp_err_t init_driver(uint8_t gpio)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << gpio,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&io_conf), TAG, "gpio config failed");
    gpio_set_drive_capability((gpio_num_t)gpio, GPIO_DRIVE_CAP_3);

    rmt_tx_channel_config_t tx_config = {
        .gpio_num = (gpio_num_t)gpio,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = RMT_RESOLUTION_HZ,
        .mem_block_symbols = 128,
        .trans_queue_depth = 4,
    };
    ESP_RETURN_ON_ERROR(rmt_new_tx_channel(&tx_config, &s_tx_channel), TAG, "rmt channel failed");

    rmt_bytes_encoder_config_t encoder_config = {
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
    ESP_RETURN_ON_ERROR(rmt_new_bytes_encoder(&encoder_config, &s_led_encoder), TAG, "rmt encoder failed");
    ESP_RETURN_ON_ERROR(rmt_enable(s_tx_channel), TAG, "rmt enable failed");

    s_ready = true;
    set_status("light ready");
    return ESP_OK;
}

static esp_err_t show_locked(uint8_t r, uint8_t g, uint8_t b)
{
    if (!s_ready) {
        return ESP_ERR_INVALID_STATE;
    }

    size_t data_len = (size_t)s_count * 3;
    uint8_t *data = heap_caps_malloc(data_len, MALLOC_CAP_8BIT);
    if (!data) {
        set_status("light buffer alloc failed");
        return ESP_ERR_NO_MEM;
    }

    for (uint16_t i = 0; i < s_count; i++) {
        data[i * 3 + 0] = g;
        data[i * 3 + 1] = r;
        data[i * 3 + 2] = b;
    }

    rmt_transmit_config_t tx_config = {
        .loop_count = 0,
        .flags.eot_level = 0,
    };
    esp_err_t ret = rmt_transmit(s_tx_channel, s_led_encoder, data, data_len, &tx_config);
    if (ret == ESP_OK) {
        ret = rmt_tx_wait_all_done(s_tx_channel, portMAX_DELAY);
    }
    free(data);
    esp_rom_delay_us(WS2812_RESET_US);

    if (ret == ESP_OK) {
        s_r = r;
        s_g = g;
        s_b = b;
        set_status("light updated");
    } else {
        set_status("light transmit failed");
    }
    return ret;
}

esp_err_t light_service_init(void)
{
    if (!s_mutex) {
        s_mutex = xSemaphoreCreateMutex();
    }
    enable_power();

    xSemaphoreTake(s_mutex, portMAX_DELAY);
    deinit_driver();
    esp_err_t ret = init_driver(s_gpio);
    if (ret == ESP_OK) {
        ret = show_locked(s_r, s_g, s_b);
    }
    xSemaphoreGive(s_mutex);
    ESP_LOGI(TAG, "light init: %s", esp_err_to_name(ret));
    return ret;
}

esp_err_t light_service_set_config(uint8_t gpio, uint16_t count)
{
    if (!valid_gpio(gpio) || count == 0 || count > LIGHT_MAX_COUNT) {
        set_status("invalid gpio or count");
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(s_mutex, portMAX_DELAY);
    if (s_ready) {
        show_locked(0, 0, 0);
    }
    deinit_driver();
    s_gpio = gpio;
    s_count = count;
    esp_err_t ret = init_driver(s_gpio);
    if (ret == ESP_OK) {
        ret = show_locked(s_r, s_g, s_b);
    }
    xSemaphoreGive(s_mutex);
    return ret;
}

esp_err_t light_service_set_color(uint8_t r, uint8_t g, uint8_t b)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    esp_err_t ret = show_locked(r, g, b);
    xSemaphoreGive(s_mutex);
    return ret;
}

esp_err_t light_service_off(void)
{
    return light_service_set_color(0, 0, 0);
}

void light_service_status_json(char *out, size_t out_size)
{
    snprintf(out, out_size,
             "{\"ready\":%s,\"gpio\":%u,\"count\":%u,\"r\":%u,\"g\":%u,\"b\":%u,\"status\":\"%s\"}",
             s_ready ? "true" : "false",
             s_gpio,
             s_count,
             s_r,
             s_g,
             s_b,
             s_status);
}
