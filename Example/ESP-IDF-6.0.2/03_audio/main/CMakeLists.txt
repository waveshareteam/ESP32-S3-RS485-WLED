idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    WHOLE_ARCHIVE
    )
    
spiffs_create_partition_image(storage ${CMAKE_SOURCE_DIR}/spiffs FLASH_IN_PROJECT)
