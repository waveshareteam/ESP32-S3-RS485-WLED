idf_component_register(
    SRCS "wifi_web.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_netif esp_event esp_http_server rs485_service light_service audio_service
)
