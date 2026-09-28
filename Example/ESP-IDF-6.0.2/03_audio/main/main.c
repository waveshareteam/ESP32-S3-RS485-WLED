#include <stdio.h>
#include <string.h>

#include "codec_dev.h"
#include "esp_audio_simple_player.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c.h"

static const char *TAG = "main";

#define SPIFFS_BASE_PATH "/spiffs"
#define MP3_FILE_PATH    SPIFFS_BASE_PATH "/music/test.mp3"
#define MP3_FILE_URI     "file://spiffs/music/test.mp3"

static esp_asp_handle_t s_player;

static esp_err_t init_spiffs(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = SPIFFS_BASE_PATH,
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = false,
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS mount failed: %s", esp_err_to_name(ret));
        return ret;
    }

    size_t total = 0;
    size_t used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS mounted, total=%u, used=%u", (unsigned)total, (unsigned)used);
    }
    return ret;
}

static esp_err_t check_mp3_file(void)
{
    FILE *fp = fopen(MP3_FILE_PATH, "rb");
    if (fp == NULL) {
        ESP_LOGE(TAG, "MP3 file not found: %s", MP3_FILE_PATH);
        return ESP_FAIL;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fclose(fp);
    ESP_LOGI(TAG, "Found MP3: %s, size=%ld bytes", MP3_FILE_PATH, size);
    return ESP_OK;
}

static int player_data_cb(uint8_t *data, int data_size, void *ctx)
{
    (void)ctx;

    size_t bytes_written = 0;
    esp_err_t ret = speaker_i2s_write(data, data_size, &bytes_written, portMAX_DELAY);
    return ret == ESP_OK && bytes_written == (size_t)data_size ? data_size : ESP_FAIL;
}

static int player_event_cb(esp_asp_event_pkt_t *event, void *ctx)
{
    (void)ctx;

    switch (event->type) {
    case ESP_ASP_EVENT_TYPE_MUSIC_INFO: {
        esp_asp_music_info_t info = {0};
        memcpy(&info, event->payload, event->payload_size);
        ESP_LOGI(TAG, "Music info: rate=%u, bits=%u, channels=%u",
                 (unsigned)info.sample_rate, (unsigned)info.bits, (unsigned)info.channels);
        ESP_ERROR_CHECK(speaker_codec_set_fs(info.sample_rate, info.bits, info.channels));
        break;
    }
    case ESP_ASP_EVENT_TYPE_STATE: {
        esp_asp_state_t state = ESP_ASP_STATE_NONE;
        memcpy(&state, event->payload, event->payload_size);
        ESP_LOGI(TAG, "Player state: %s", esp_audio_simple_player_state_to_str(state));
        break;
    }
    default:
        ESP_LOGI(TAG, "Player event: %d", event->type);
        break;
    }

    return ESP_OK;
}

static void start_mp3_player(void)
{
    esp_asp_cfg_t cfg = {
        .out.cb = player_data_cb,
        .out.user_ctx = NULL,
        .task_prio = 5,
        .task_stack = 8192,
        .task_stack_in_ext = false,
    };

    ESP_ERROR_CHECK(esp_audio_simple_player_new(&cfg, &s_player));
    ESP_ERROR_CHECK(esp_audio_simple_player_set_event(s_player, player_event_cb, NULL));
    ESP_LOGI(TAG, "Start playing %s", MP3_FILE_URI);
    ESP_ERROR_CHECK(esp_audio_simple_player_run(s_player, MP3_FILE_URI, NULL));
}

void app_main(void)
{
    DEV_I2C_Init();
    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_LOGI(TAG, "Initializing audio codec");
    ESP_ERROR_CHECK(codec_init());
    ESP_ERROR_CHECK(speaker_codec_volume_set(100, NULL));

    ESP_ERROR_CHECK(init_spiffs());
    ESP_ERROR_CHECK(check_mp3_file());

    start_mp3_player();
}
