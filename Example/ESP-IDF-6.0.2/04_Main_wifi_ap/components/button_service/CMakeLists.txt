idf_component_register(
    SRCS "button_service.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_driver_gpio audio_service
)
