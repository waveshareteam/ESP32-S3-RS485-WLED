#include <Arduino.h>
#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "driver/gpio.h"

// ------------------- Configuration -------------------
#define WS2812_GPIO_PIN  (gpio_num_t)2       // LED strip control pin
#define WS2812_PHYSICAL_NUM 60                 // Number of physical LED segments：1428*1m -> 12，1428*5m -> 60
#define GPIO6_CTRL_PIN   (gpio_num_t)6
#define WS2812_LOGICAL_NUM  (WS2812_PHYSICAL_NUM * 2) // Number of logical pixels

#define RMT_RESOLUTION_HZ 10000000

static const char *TAG = "WS2812";

// ------------------- RMT Handles -------------------
static rmt_channel_handle_t tx_channel = NULL;
static rmt_encoder_handle_t led_encoder = NULL;

// ------------------- Timing Parameters -------------------
#define WS2812_T0H_NS  300
#define WS2812_T0L_NS  800
#define WS2812_T1H_NS  800
#define WS2812_T1L_NS  800
#define WS2812_RESET_US 300  

// ------------------- Driver Initialization -------------------
void ws2812_init(void) {

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << WS2812_GPIO_PIN) | (1ULL << GPIO6_CTRL_PIN);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    ESP_ERROR_CHECK(gpio_set_drive_capability(WS2812_GPIO_PIN, GPIO_DRIVE_CAP_3));
    ESP_ERROR_CHECK(gpio_set_drive_capability(GPIO6_CTRL_PIN, GPIO_DRIVE_CAP_3));

    gpio_set_level(GPIO6_CTRL_PIN, 1);
    ESP_LOGI(TAG, "GPIO6 set HIGH");

    // 2. Configure RMT channel
    rmt_tx_channel_config_t tx_chan_config = {}; // Initialize to zero
    tx_chan_config.gpio_num = WS2812_GPIO_PIN;
    tx_chan_config.clk_src = RMT_CLK_SRC_DEFAULT;
    tx_chan_config.resolution_hz = RMT_RESOLUTION_HZ;
    tx_chan_config.mem_block_symbols = 128;
    tx_chan_config.trans_queue_depth = 4;
    
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &tx_channel));

    // 3. Configure byte encoder
    rmt_bytes_encoder_config_t bytes_encoder_config = {};
    
    bytes_encoder_config.bit0.duration0 = WS2812_T0H_NS / (1000000000 / RMT_RESOLUTION_HZ);
    bytes_encoder_config.bit0.level0 = 1;
    bytes_encoder_config.bit0.duration1 = WS2812_T0L_NS / (1000000000 / RMT_RESOLUTION_HZ);
    bytes_encoder_config.bit0.level1 = 0;
    
    bytes_encoder_config.bit1.duration0 = WS2812_T1H_NS / (1000000000 / RMT_RESOLUTION_HZ);
    bytes_encoder_config.bit1.level0 = 1;
    bytes_encoder_config.bit1.duration1 = WS2812_T1L_NS / (1000000000 / RMT_RESOLUTION_HZ);
    bytes_encoder_config.bit1.level1 = 0;

    bytes_encoder_config.flags.msb_first = 1; 
    
    ESP_ERROR_CHECK(rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder));

    ESP_ERROR_CHECK(rmt_enable(tx_channel));
    ESP_LOGI(TAG, "WS2812 initialized (Dual-chip mode)");
}

// ------------------- Core Control Function: RGB + W1W2 -------------------
void WS2812_show(uint8_t r, uint8_t g, uint8_t b, uint8_t w1, uint8_t w2) {
    uint8_t led_data[WS2812_LOGICAL_NUM * 3];

    for (int i = 0; i < WS2812_PHYSICAL_NUM; i++) {
        int idx_chip1 = i * 2 * 3;
        int idx_chip2 = idx_chip1 + 3;

        led_data[idx_chip1 + 0] = r;
        led_data[idx_chip1 + 1] = g;
        led_data[idx_chip1 + 2] = b;

        led_data[idx_chip2 + 0] = w1;
        led_data[idx_chip2 + 1] = w2;
        led_data[idx_chip2 + 2] = 0;
    }

    rmt_transmit_config_t tx_config = {};
    tx_config.loop_count = 0;

    ESP_ERROR_CHECK(rmt_transmit(tx_channel, led_encoder, led_data, sizeof(led_data), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(tx_channel, portMAX_DELAY));
    esp_rom_delay_us(WS2812_RESET_US);
}

// ------------------- Arduino Setup -------------------
void setup() {
    Serial.begin(115200);
    delay(1000);
    ws2812_init();
}

// ------------------- Arduino Loop -------------------
void loop() {
    ESP_LOGI(TAG, "Test 1: Only Red");
    WS2812_show(255, 0, 0, 0, 0);
    delay(2000);

    ESP_LOGI(TAG, "Test 2: Only Green");
    WS2812_show(0, 255, 0, 0, 0);
    delay(2000);

    ESP_LOGI(TAG, "Test 3: Only Blue");
    WS2812_show(0, 0, 255, 0, 0);
    delay(2000);

    ESP_LOGI(TAG, "Test 4: Only W1");
    WS2812_show(0, 0, 0, 255, 0);
    delay(2000);

    ESP_LOGI(TAG, "Test 5: Only W2");
    WS2812_show(0, 0, 0, 0, 255);
    delay(2000);

    ESP_LOGI(TAG, "Test 6: All On");
    WS2812_show(255, 255, 255, 255, 255);
    delay(2000);
}