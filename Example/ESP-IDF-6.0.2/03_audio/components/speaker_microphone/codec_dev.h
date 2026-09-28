/*****************************************************************************
 * | File         :   codec_dev.h
 * | Author       :   Waveshare team
 * | Function     :   Hardware underlying interface
 * | Info         :
 * |
 * ----------------
 * | This version :   V1.0
 * | Date         :   2025-07-28
 * | Info         :   Basic version
 *
 ******************************************************************************/
#ifndef __CODEC_DEV_H
#define __CODEC_DEV_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/i2s_std.h"
#include "esp_err.h"

#define CODEC_DEFAULT_SAMPLE_RATE           (24000)
#define CODEC_DEFAULT_BIT_WIDTH             (16)
#define CODEC_DEFAULT_CHANNEL               (2)
#define CODEC_DEFAULT_VOLUME                (60)
#define CODEC_DEFAULT_TDM                   (0)

esp_err_t speaker_codec_mute_set(bool enable);
esp_err_t speaker_codec_volume_set(int volume, int *volume_set);
int speaker_codec_volume_get(void);
esp_err_t speaker_codec_dev_stop(void);
esp_err_t speaker_codec_dev_resume(void);
esp_err_t speaker_codec_set_fs(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch);
esp_err_t speaker_i2s_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms);
esp_err_t codec_init(void);

#endif
