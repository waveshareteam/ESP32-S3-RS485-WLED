#include "codec_dev.h"

#include <assert.h>

#include "esp_log.h"
#include "speaker_microphone.h"

static const char *TAG = "codec_dev";

static esp_codec_dev_handle_t play_dev_handle;
static bool is_audio_init;
static int volume_intensity = CODEC_DEFAULT_VOLUME;

esp_err_t speaker_i2s_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    (void)timeout_ms;

    esp_err_t ret = esp_codec_dev_write(play_dev_handle, audio_buffer, len);
    if (bytes_written) {
        *bytes_written = (ret == ESP_OK) ? len : 0;
    }
    return ret;
}

esp_err_t speaker_codec_set_fs(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
{
    esp_codec_dev_sample_info_t fs = {
        .sample_rate = rate,
        .channel = ch,
        .bits_per_sample = bits_cfg,
    };

    esp_err_t ret = ESP_OK;
    if (play_dev_handle) {
        ret = esp_codec_dev_close(play_dev_handle);
        ret |= esp_codec_dev_open(play_dev_handle, &fs);
    }
    return ret;
}

esp_err_t speaker_codec_volume_set(int volume, int *volume_set)
{
    (void)volume_set;

    ESP_RETURN_ON_ERROR(esp_codec_dev_set_out_vol(play_dev_handle, volume), TAG, "Set codec volume failed");
    volume_intensity = volume;
    ESP_LOGI(TAG, "Setting volume: %d", volume);
    return ESP_OK;
}

int speaker_codec_volume_get(void)
{
    return volume_intensity;
}

esp_err_t speaker_codec_mute_set(bool enable)
{
    return esp_codec_dev_set_out_mute(play_dev_handle, enable);
}

esp_err_t speaker_codec_dev_stop(void)
{
    if (play_dev_handle) {
        return esp_codec_dev_close(play_dev_handle);
    }
    return ESP_OK;
}

esp_err_t speaker_codec_dev_resume(void)
{
    return speaker_codec_set_fs(CODEC_DEFAULT_SAMPLE_RATE, CODEC_DEFAULT_BIT_WIDTH, CODEC_DEFAULT_CHANNEL);
}

esp_err_t codec_init(void)
{
    if (is_audio_init) {
        return ESP_OK;
    }

    play_dev_handle = speaker_init();
    assert(play_dev_handle && "play_dev_handle not initialized");

    ESP_RETURN_ON_ERROR(speaker_codec_set_fs(CODEC_DEFAULT_SAMPLE_RATE,
                                             CODEC_DEFAULT_BIT_WIDTH,
                                             CODEC_DEFAULT_CHANNEL),
                        TAG, "Open speaker codec failed");

    is_audio_init = true;
    return ESP_OK;
}
