# ESP32-S3 SPIFFS MP3 Player

Minimal ESP-IDF project that mounts SPIFFS and plays `spiffs/music/test.mp3`
through `espressif/esp_audio_simple_player`.

## What remains

- `main/`: SPIFFS mount, MP3 file check, simple player setup
- `components/i2c/`: I2C bus initialization for the audio codec
- `components/speaker_microphone/`: speaker-only ES8389 + I2S output support
- `spiffs/music/test.mp3`: audio file embedded into the `storage` SPIFFS partition

## Build

```powershell
idf.py -B build_codex_verify build
```

## Flash

```powershell
idf.py -B build_codex_verify flash monitor
```
