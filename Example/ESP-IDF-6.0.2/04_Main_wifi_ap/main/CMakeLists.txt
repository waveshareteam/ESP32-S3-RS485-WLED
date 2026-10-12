idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash rs485_service light_service audio_service button_service wifi_web
    WHOLE_ARCHIVE
    )
    
