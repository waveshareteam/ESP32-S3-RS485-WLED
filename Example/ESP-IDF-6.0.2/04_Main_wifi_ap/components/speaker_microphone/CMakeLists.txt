idf_component_register(SRCS "speaker_microphone.c" "codec_dev.c" 
                        INCLUDE_DIRS "."
                        REQUIRES driver esp_driver_gpio esp_driver_i2s i2c esp_codec_dev
                    )
