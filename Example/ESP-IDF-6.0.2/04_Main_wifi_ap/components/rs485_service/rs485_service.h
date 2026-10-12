#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RS485_DEFAULT_BAUD_RATE 9600

typedef enum {
    RS485_FORMAT_CHAR = 0,
    RS485_FORMAT_HEX = 1,
} rs485_format_t;

esp_err_t rs485_service_init(void);
esp_err_t rs485_service_set_baud(uint32_t baud);
uint32_t rs485_service_get_baud(void);
void rs485_service_set_rx_format(rs485_format_t format);
rs485_format_t rs485_service_get_rx_format(void);
esp_err_t rs485_service_send(const uint8_t *data, size_t length);
size_t rs485_service_take_received(char *out, size_t out_size);

#ifdef __cplusplus
}
#endif
