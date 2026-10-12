idf_component_register(
    SRCS "light_service.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_driver_gpio esp_driver_rmt
)
