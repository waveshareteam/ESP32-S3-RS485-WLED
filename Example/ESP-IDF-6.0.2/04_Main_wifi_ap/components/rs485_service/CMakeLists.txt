idf_component_register(
    SRCS "rs485_service.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_driver_uart esp_driver_gpio esp_timer
)
