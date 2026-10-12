#include "audio_service.h"
#include "button_service.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "light_service.h"
#include "nvs_flash.h"
#include "rs485_service.h"
#include "wifi_web.h"

static const char *TAG = "main";

static esp_err_t init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), TAG, "nvs erase failed");
        ret = nvs_flash_init();
    }
    return ret;
}

void app_main(void)
{
    ESP_ERROR_CHECK(init_nvs());

    ESP_ERROR_CHECK(rs485_service_init());
    ESP_ERROR_CHECK(light_service_init());
    ESP_ERROR_CHECK(audio_service_init());
    ESP_ERROR_CHECK(button_service_init());
    ESP_ERROR_CHECK(wifi_web_start());

    ESP_LOGI(TAG, "05_MAIN_WIFI_AP ready. Connect WiFi SSID ESP32-S3-RS485-WLED, open http://192.168.4.1");
}
