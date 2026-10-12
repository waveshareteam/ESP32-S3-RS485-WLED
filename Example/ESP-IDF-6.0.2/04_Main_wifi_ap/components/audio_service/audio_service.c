#include "audio_service.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "codec_dev.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c.h"
#include "music.h"

static const char *TAG = "audio_service";

#define PLAY_FRAME_SIZE       2048
#define MIC_FRAME_SIZE        1024
#define RECORD_SECONDS        5
#define RECORD_PLAYBACK_GAIN  2
#define RECORD_BYTES          (CODEC_DEFAULT_SAMPLE_RATE * RECORD_SECONDS * 4 * (CODEC_DEFAULT_BIT_WIDTH / 8))

typedef enum {
    AUDIO_MODE_IDLE = 0,
    AUDIO_MODE_MUSIC = 1,
    AUDIO_MODE_RECORDING = 2,
    AUDIO_MODE_RECORD_PLAYBACK = 3,
} audio_mode_t;

static int16_t s_play_frame[PLAY_FRAME_SIZE * 2];
static int16_t s_mic_frame[MIC_FRAME_SIZE * 4];
static uint8_t *s_record_buffer;
static size_t s_record_bytes;
static TaskHandle_t s_task_handle;
static volatile bool s_ready;
static volatile bool s_busy;
static volatile bool s_stop;
static volatile bool s_play_recorded_after_record;
static volatile bool s_record_after_playback_stop;
static volatile audio_mode_t s_mode = AUDIO_MODE_IDLE;
static uint32_t s_music_index;
static char s_status[96] = "audio not initialized";

static void record_5s_task(void *arg);

static void set_status(const char *status)
{
    strlcpy(s_status, status, sizeof(s_status));
}

static bool start_audio_task(TaskFunction_t task, const char *name)
{
    if (!s_ready || s_busy || s_task_handle) {
        return false;
    }

    s_busy = true;
    s_stop = false;
    BaseType_t ok = xTaskCreatePinnedToCore(task, name, 8192, NULL, 3, &s_task_handle, 1);
    if (ok != pdPASS) {
        s_busy = false;
        s_task_handle = NULL;
        set_status("audio task create failed");
        return false;
    }
    return true;
}

static void finish_audio_task(const char *status)
{
    set_status(status);
    s_mode = AUDIO_MODE_IDLE;
    s_busy = false;
    s_task_handle = NULL;
    vTaskDelete(NULL);
}

static int32_t sample_abs(int16_t value)
{
    return value < 0 ? -(int32_t)value : (int32_t)value;
}

static int16_t select_recorded_sample(const int16_t *frame)
{
    int16_t sample = frame[3];
    int32_t peak = sample_abs(sample);

    for (size_t ch = 0; ch < 4; ch++) {
        int32_t level = sample_abs(frame[ch]);
        if (level > peak) {
            peak = level;
            sample = frame[ch];
        }
    }
    return sample;
}

static int16_t amplify_recorded_sample(int16_t sample)
{
    int32_t amplified = (int32_t)sample * RECORD_PLAYBACK_GAIN;
    if (amplified > INT16_MAX) {
        return INT16_MAX;
    }
    if (amplified < INT16_MIN) {
        return INT16_MIN;
    }
    return (int16_t)amplified;
}

static void play_recorded_buffer(void)
{
    if (!s_record_buffer || s_record_bytes == 0) {
        set_status("no recording");
        return;
    }

    ESP_LOGI(TAG, "play recorded buffer, bytes=%u", (unsigned)s_record_bytes);
    set_status("playing recording");
    s_mode = AUDIO_MODE_RECORD_PLAYBACK;
    speaker_codec_mute_set(false);

    int16_t *samples = (int16_t *)s_record_buffer;
    size_t total_samples = s_record_bytes / sizeof(int16_t);
    size_t total_frames = total_samples / 4;
    size_t frame_index = 0;

    while (frame_index < total_frames && !s_stop) {
        size_t frames = PLAY_FRAME_SIZE;
        if (frame_index + frames > total_frames) {
            frames = total_frames - frame_index;
        }

        for (size_t i = 0; i < frames; i++) {
            int16_t sample = amplify_recorded_sample(select_recorded_sample(&samples[(frame_index + i) * 4]));
            s_play_frame[i * 2 + 0] = sample;
            s_play_frame[i * 2 + 1] = sample;
        }

        size_t bytes_written = 0;
        speaker_i2s_write(s_play_frame, frames * 2 * sizeof(int16_t), &bytes_written, portMAX_DELAY);
        frame_index += frames;
    }

    speaker_codec_mute_set(true);
}

static void play_music_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "play built-in audio, samples=%lu", (unsigned long)AUDIO_SAMPLES);
    set_status("playing music");
    s_mode = AUDIO_MODE_MUSIC;
    speaker_codec_mute_set(false);

    while (s_music_index < AUDIO_SAMPLES && !s_stop) {
        size_t samples = PLAY_FRAME_SIZE;
        if (s_music_index + samples > AUDIO_SAMPLES) {
            samples = AUDIO_SAMPLES - s_music_index;
        }

        for (size_t i = 0; i < samples; i++) {
            int16_t sample = (int16_t)audio_data[s_music_index + i];
            s_play_frame[i * 2 + 0] = sample;
            s_play_frame[i * 2 + 1] = sample;
        }

        size_t bytes_written = 0;
        speaker_i2s_write(s_play_frame, samples * 2 * sizeof(int16_t), &bytes_written, portMAX_DELAY);
        s_music_index += samples;
    }

    speaker_codec_mute_set(true);
    if (s_stop && s_music_index < AUDIO_SAMPLES) {
        if (s_record_after_playback_stop) {
            s_record_after_playback_stop = false;
            s_stop = false;
            s_play_recorded_after_record = true;
            record_5s_task(NULL);
            return;
        }
        finish_audio_task("music paused");
        return;
    }

    s_music_index = 0;
    finish_audio_task("music done");
}

static void record_5s_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "record microphone for %d seconds", RECORD_SECONDS);
    set_status("recording 5s");
    s_mode = AUDIO_MODE_RECORDING;
    speaker_codec_mute_set(true);

    if (!s_record_buffer) {
        s_record_buffer = heap_caps_malloc(RECORD_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!s_record_buffer) {
            s_record_buffer = heap_caps_malloc(RECORD_BYTES, MALLOC_CAP_8BIT);
        }
    }
    if (!s_record_buffer) {
        s_record_bytes = 0;
        s_play_recorded_after_record = false;
        s_record_after_playback_stop = false;
        ESP_LOGE(TAG, "record buffer alloc failed, bytes=%u", (unsigned)RECORD_BYTES);
        finish_audio_task("record buffer alloc failed");
        return;
    }

    size_t total_bytes = 0;
    while (total_bytes < RECORD_BYTES) {
        size_t request_bytes = sizeof(s_mic_frame);
        if (total_bytes + request_bytes > RECORD_BYTES) {
            request_bytes = RECORD_BYTES - total_bytes;
        }

        size_t bytes_read = 0;
        mic_i2s_read(s_mic_frame, request_bytes, &bytes_read, portMAX_DELAY);
        if (bytes_read == 0) {
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }

        memcpy(s_record_buffer + total_bytes, s_mic_frame, bytes_read);
        total_bytes += bytes_read;
    }

    s_record_bytes = total_bytes;
    ESP_LOGI(TAG, "record done, bytes=%u", (unsigned)s_record_bytes);
    if (s_play_recorded_after_record) {
        play_recorded_buffer();
        s_play_recorded_after_record = false;
        finish_audio_task(s_stop ? "record playback stopped" : "record playback done");
        return;
    }
    finish_audio_task("record done");
}

esp_err_t audio_service_init(void)
{
    DEV_I2C_Init();
    vTaskDelay(pdMS_TO_TICKS(100));

    esp_err_t ret = codec_init();
    if (ret == ESP_OK) {
        speaker_codec_volume_set(80, NULL);
        microphone_codec_gain_set(80, NULL);
        speaker_codec_mute_set(true);
        s_ready = true;
        set_status("audio ready");
    } else {
        s_ready = false;
        set_status("audio init failed");
    }
    return ret;
}

bool audio_service_toggle_music(void)
{
    if (!s_ready) {
        return false;
    }

    if (s_busy && s_mode == AUDIO_MODE_MUSIC) {
        s_stop = true;
        set_status("pausing music");
        return true;
    }

    if (s_busy && s_mode == AUDIO_MODE_RECORD_PLAYBACK) {
        s_stop = true;
        set_status("stopping record playback");
        return true;
    }

    if (s_busy) {
        return false;
    }

    if (s_music_index >= AUDIO_SAMPLES) {
        s_music_index = 0;
    }
    return start_audio_task(play_music_task, "audio_music");
}

bool audio_service_record_5s_and_play(void)
{
    if (s_busy && (s_mode == AUDIO_MODE_MUSIC || s_mode == AUDIO_MODE_RECORD_PLAYBACK)) {
        s_record_after_playback_stop = true;
        s_stop = true;
        set_status("stopping playback for record");
        return true;
    }

    if (s_busy) {
        return false;
    }
    s_record_after_playback_stop = false;
    s_play_recorded_after_record = true;
    return start_audio_task(record_5s_task, "audio_record");
}

bool audio_service_is_busy(void)
{
    return s_busy;
}

void audio_service_status_json(char *out, size_t out_size)
{
    snprintf(out, out_size,
             "{\"ready\":%s,\"busy\":%s,\"mode\":%d,\"musicIndex\":%lu,\"recordBytes\":%u,\"status\":\"%s\"}",
             s_ready ? "true" : "false",
             s_busy ? "true" : "false",
             (int)s_mode,
             (unsigned long)s_music_index,
             (unsigned)s_record_bytes,
             s_status);
}
