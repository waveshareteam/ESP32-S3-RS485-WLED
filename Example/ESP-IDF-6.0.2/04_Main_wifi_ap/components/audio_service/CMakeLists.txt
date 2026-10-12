idf_component_register(
    SRCS "audio_service.c"
    INCLUDE_DIRS "."
    REQUIRES speaker_microphone i2c
)
