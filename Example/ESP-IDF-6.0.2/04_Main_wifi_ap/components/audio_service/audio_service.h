#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t audio_service_init(void);
bool audio_service_toggle_music(void);
bool audio_service_record_5s_and_play(void);
bool audio_service_is_busy(void);
void audio_service_status_json(char *out, size_t out_size);

#ifdef __cplusplus
}
#endif
