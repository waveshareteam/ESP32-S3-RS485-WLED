# 05_MAIN_WIFI_AP

ESP-IDF C 综合示例，参考 `04_MAIN_WIFI_AP` 的 Arduino 功能，并复用现有 IDF 示例里的 RS485、灯光和音频底层。

## 功能

- WiFi AP：`ESP32-S3-RS485-WLED` / `12345678`
- Web 控制页：浏览器打开 `http://192.168.4.1`
- RS485：网页发送、接收显示、波特率切换、char/hex 模式
- WS2812 灯带：GPIO 和数量配置，RGB 调色，关闭
- 音频：网页播放/暂停内置 `music.h` 音频，录音 5 秒后播放
- BOOT 按键：短按播放/暂停，长按录音 5 秒并播放

## 默认引脚

- RS485 TX：GPIO17
- RS485 RX：GPIO18
- RS485 TXDEN/RTS：GPIO21
- WS2812 DATA：GPIO2
- LED 电源使能：GPIO6
- BOOT：GPIO0
- 音频 I2C：SDA GPIO15，SCL GPIO14
- 音频 I2S：MCLK GPIO7，BCLK GPIO9，WS GPIO45，DOUT GPIO8，DIN GPIO10
- PA 使能：GPIO46

## Build

```powershell
idf.py -B build build
```

## Flash

```powershell
idf.py -B build flash monitor
```
