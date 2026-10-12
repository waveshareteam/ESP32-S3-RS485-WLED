#include "rs485_service.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "rs485_service";

#define RS485_UART_PORT UART_NUM_2
#define RS485_TXD       GPIO_NUM_17
#define RS485_RXD       GPIO_NUM_18
#define RS485_RTS       GPIO_NUM_21
#define RS485_BUF_SIZE  512
#define RS485_TEXT_SIZE 4096

static SemaphoreHandle_t s_mutex;
static TaskHandle_t s_task;
static uint32_t s_baud = RS485_DEFAULT_BAUD_RATE;
static rs485_format_t s_rx_format = RS485_FORMAT_CHAR;
static char s_received_text[RS485_TEXT_SIZE];
static size_t s_received_len;

static void append_received_text(const uint8_t *data, size_t len)
{
    if (s_mutex) {
        xSemaphoreTake(s_mutex, portMAX_DELAY);
    }

    int64_t ms = esp_timer_get_time() / 1000;
    int written = snprintf(s_received_text + s_received_len,
                           sizeof(s_received_text) - s_received_len,
                           "%lld ms: ",
                           (long long)ms);
    if (written < 0 || (size_t)written >= sizeof(s_received_text) - s_received_len) {
        s_received_len = 0;
        s_received_text[0] = '\0';
    } else {
        s_received_len += written;
    }

    for (size_t i = 0; i < len && s_received_len + 8 < sizeof(s_received_text); i++) {
        if (s_rx_format == RS485_FORMAT_HEX) {
            written = snprintf(s_received_text + s_received_len,
                               sizeof(s_received_text) - s_received_len,
                               "0x%02X ",
                               data[i]);
            if (written > 0) {
                s_received_len += written;
            }
        } else {
            s_received_text[s_received_len++] = (data[i] >= 32 && data[i] <= 126) ? (char)data[i] : ' ';
            s_received_text[s_received_len] = '\0';
        }
    }

    if (s_received_len + 2 < sizeof(s_received_text)) {
        s_received_text[s_received_len++] = '\n';
        s_received_text[s_received_len] = '\0';
    }

    if (s_mutex) {
        xSemaphoreGive(s_mutex);
    }
}

static void rs485_task(void *arg)
{
    (void)arg;
    uint8_t *data = (uint8_t *)malloc(RS485_BUF_SIZE);
    if (!data) {
        ESP_LOGE(TAG, "rx buffer alloc failed");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        int len = uart_read_bytes(RS485_UART_PORT, data, RS485_BUF_SIZE, pdMS_TO_TICKS(100));
        if (len > 0) {
            ESP_LOGI(TAG, "RS485 received %d bytes", len);
            append_received_text(data, (size_t)len);
        }
    }
}

esp_err_t rs485_service_init(void)
{
    if (s_task) {
        return ESP_OK;
    }

    s_mutex = xSemaphoreCreateMutex();
    if (!s_mutex) {
        return ESP_ERR_NO_MEM;
    }

    uart_config_t uart_config = {
        .baud_rate = (int)s_baud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_RETURN_ON_ERROR(uart_driver_install(RS485_UART_PORT, RS485_BUF_SIZE * 2, RS485_BUF_SIZE, 0, NULL, 0),
                        TAG, "uart driver install failed");
    ESP_RETURN_ON_ERROR(uart_param_config(RS485_UART_PORT, &uart_config), TAG, "uart config failed");
    ESP_RETURN_ON_ERROR(uart_set_pin(RS485_UART_PORT, RS485_TXD, RS485_RXD, RS485_RTS, UART_PIN_NO_CHANGE),
                        TAG, "uart pins failed");
    ESP_RETURN_ON_ERROR(uart_set_mode(RS485_UART_PORT, UART_MODE_RS485_HALF_DUPLEX),
                        TAG, "rs485 mode failed");
    ESP_RETURN_ON_ERROR(uart_set_rx_timeout(RS485_UART_PORT, 3), TAG, "rx timeout failed");

    BaseType_t ok = xTaskCreate(rs485_task, "rs485_task", 4096, NULL, 10, &s_task);
    return ok == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}

esp_err_t rs485_service_set_baud(uint32_t baud)
{
    if (baud == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    ESP_RETURN_ON_ERROR(uart_set_baudrate(RS485_UART_PORT, baud), TAG, "set baud failed");
    s_baud = baud;
    ESP_LOGI(TAG, "baud updated: %lu", (unsigned long)s_baud);
    return ESP_OK;
}

uint32_t rs485_service_get_baud(void)
{
    return s_baud;
}

void rs485_service_set_rx_format(rs485_format_t format)
{
    s_rx_format = format == RS485_FORMAT_HEX ? RS485_FORMAT_HEX : RS485_FORMAT_CHAR;
}

rs485_format_t rs485_service_get_rx_format(void)
{
    return s_rx_format;
}

esp_err_t rs485_service_send(const uint8_t *data, size_t length)
{
    if (!data || length == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    int written = uart_write_bytes(RS485_UART_PORT, (const char *)data, length);
    ESP_RETURN_ON_FALSE(written == (int)length, ESP_FAIL, TAG, "rs485 send failed");
    return uart_wait_tx_done(RS485_UART_PORT, pdMS_TO_TICKS(100));
}

size_t rs485_service_take_received(char *out, size_t out_size)
{
    if (!out || out_size == 0) {
        return 0;
    }

    if (s_mutex) {
        xSemaphoreTake(s_mutex, portMAX_DELAY);
    }

    size_t copy_len = s_received_len;
    if (copy_len >= out_size) {
        copy_len = out_size - 1;
    }
    memcpy(out, s_received_text, copy_len);
    out[copy_len] = '\0';
    s_received_len = 0;
    s_received_text[0] = '\0';

    if (s_mutex) {
        xSemaphoreGive(s_mutex);
    }
    return copy_len;
}
