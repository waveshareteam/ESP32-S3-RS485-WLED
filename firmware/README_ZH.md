# ESP32-S3-RS485-WLED 出厂固件

[English](README.md)

| 文件 | 用途 |
| --- | --- |
| [`ESP32-S3-RS485-WLED_FactoryOnly_20261019.bin`](ESP32-S3-RS485-WLED_FactoryOnly_20261019.bin) | 恢复 ESP32-S3-RS485-WLED 的 WLED 固件。 |

使用 USB 数据线连接开发板和电脑。在本目录打开终端，使用 esptool 5 或更新版本，并将 `PORT` 替换为开发板的串口。

整片擦除 Flash 会清除固件、设置和用户数据，请提前备份需要保留的数据。

```sh
python -m esptool --chip esp32s3 --port PORT erase-flash
python -m esptool --chip esp32s3 --port PORT write-flash 0x0 ESP32-S3-RS485-WLED_FactoryOnly_20261019.bin
```

烧录完成后复位开发板，确认 WLED 正常启动。配置和使用方法请参考[产品文档](https://docs.waveshare.net/ESP32-S3-RS485-WLED/)。
