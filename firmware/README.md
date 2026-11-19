# ESP32-S3-RS485-WLED Factory Firmware

[简体中文](README_ZH.md)

| File | Purpose |
| --- | --- |
| [`ESP32-S3-RS485-WLED_FactoryOnly_20261019.bin`](ESP32-S3-RS485-WLED_FactoryOnly_20261019.bin) | Restores the WLED firmware on the ESP32-S3-RS485-WLED. |

Connect the board to your computer using a USB data cable. Open a terminal in this directory, use esptool 5 or later, and replace `PORT` with the board's serial port.

Erasing the entire flash clears the firmware, settings, and user data. Back up anything you want to keep before proceeding.

```sh
python -m esptool --chip esp32s3 --port PORT erase-flash
python -m esptool --chip esp32s3 --port PORT write-flash 0x0 ESP32-S3-RS485-WLED_FactoryOnly_20261019.bin
```

Reset the board after flashing and check that WLED starts. For setup and usage, see the [product documentation](https://docs.waveshare.com/ESP32-S3-RS485-WLED).
