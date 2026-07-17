# Hardware Lab 索引卡片草案

## 名称

```text
ESP32 天气时钟
```

## 摘要

```text
基于 ESP32、DHT22、BH1750、DS3231、SSD1306 OLED 与本地 AP 配网的天气时钟教学原型。
```

## 平台

```text
ESP32 · Arduino · PlatformIO · DHT22 · BH1750 · DS3231 · SSD1306 · Wi-Fi · NTP
```

## 真实状态口径

```text
源码来源已确认；公开候选已完成凭据净化与无硬件门禁准备。当前 ESP32、DHT22、BH1750、DS3231、OLED、Wi-Fi、NTP、EEPROM 与天气 API 端到端链路尚未重新真机复测。
```

## 公开范围与边界

```text
当前未公开实物照片、演示视频、原理图、PCB、Gerber 或制造文件；公开 BOM、源码推导接线边界图、来源、协议和验证说明。AP 配网与保存接口是无认证、无 TLS 的 HTTP；EEPROM 不是安全存储，天气请求当前不校验证书。
```

填入 Hardware Lab 前必须将本卡的 `head_sha`、固定 Actions URL 与最终公开仓 exact HEAD 同步；线上 CI 未完成前不得写成构建证据已通过。
