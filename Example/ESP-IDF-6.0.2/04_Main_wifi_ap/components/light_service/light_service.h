#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LIGHT_DEFAULT_GPIO  2
#define LIGHT_DEFAULT_COUNT 160

esp_err_t light_service_init(void);
esp_err_t light_service_set_config(uint8_t gpio, uint16_t count);
esp_err_t light_service_set_color(uint8_t r, uint8_t g, uint8_t b);
esp_err_t light_service_off(void);
void light_service_status_json(char *out, size_t out_size);

#ifdef __cplusplus
}
#endif
