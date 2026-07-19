# ESP32 天气时钟

它演示一条小型嵌入式原型链路：读取本地温湿度、光照和 RTC；在有网络和用户自备凭据时请求天气数据；将当前源码中的字段绘制到 OLED。它不是气象服务、计量仪表、网络安全产品或可直接长期部署的成品。

## 项目资料

这里整理了项目照片、界面截图和相关资料；文件处理说明见 [MEDIA_EVIDENCE](docs/MEDIA_EVIDENCE.md)。

![ESP32 天气时钟原型，2026-03-28](assets/photos/historical-prototype.jpg)

## 源码功能范围

```text
DHT22 ─────┐
BH1750 ────┼─ I2C / GPIO ── ESP32 ── SSD1306 OLED
DS3231 ────┘                  │
                               ├─ EEPROM：本地配置（非安全存储）
                               ├─ AP Captive Portal：短时本地配置
                               ├─ STA + NTP：时钟同步
                               └─ HTTPS 请求：和风天气当前天气接口
```

- 从 DHT22 读取温度、湿度；从 BH1750 读取光照；从 DS3231 读取时间；
- 使用 SSD1306 显示时钟、日期、当前天气字段、室内温湿度、光照和 Wi-Fi 标志；
- 首次没有可用 Wi-Fi，或连续重连失败后，创建本地 AP 配网页；
- 用户在本地页面输入 Wi-Fi、和风天气 Key 与城市 ID，固件写入 EEPROM 后重启；
- 网络可用时请求天气数据；源码当前用 gzip 解压与 JSON 解析处理响应。

上述均为**当前源码行为**，不是当前设备的实时或已复测状态。

## 接线与硬件说明

| 模块 | ESP32 接口 | 当前源码配置 | 注意事项 |
| :-- | :-- | :-- | :-- |
| DHT22 | GPIO4 | `DHT_PIN=4` | 传感器型号、上拉、供电、精度和实际读数均待复测。 |
| SSD1306 OLED | SDA=GPIO21、SCL=GPIO22 | I2C 地址 `0x3C` | 当前代码初始化失败会停止运行；实际屏幕地址和电压须以实物为准。 |
| BH1750 | SDA=GPIO21、SCL=GPIO22 | I2C 地址 `0x23` | 与 OLED、DS3231 共用 I2C；总线拉高与地址冲突待实物确认。 |
| DS3231 RTC | SDA=GPIO21、SCL=GPIO22 | I2C 地址 `0x68` | RTC 电池、时区、失电路径和准确度未按公开提交复测。 |
| USB 串口 | 取决于开发板 | `115200` | 开发板型号、Flash、USB 芯片与稳定供电未确认。 |

完整清单和边界见 [HARDWARE.md](HARDWARE.md)。`hardware/wiring-diagram.svg` 是按源码推导的接口图，**不是**原理图、PCB、实测接线或电气认证。

请先确认所有模块的电压、电平、共地、上拉/限流和开发板资料。不要将 5 V 信号直接接入 ESP32 GPIO，也不要把此教学原型连接到市电、大功率或安全关键设备。

## 本地配置与网络安全

没有可用 STA 配置时，固件会启动：

```text
SSID: WeatherClock-Setup
Password: none (open AP)
Config URL: http://192.168.4.1/
```

开放 AP 是为了避免把固定密码伪装为安全机制；它只适合短时间、隔离、可信的本地配置环境。页面和 `/scan`、`/save` 都是无认证、无 TLS 的 HTTP。`/scan` 会返回附近 Wi-Fi 名称和信号信息；`/save` 会接收 Wi-Fi 密码和天气 API Key 并写入 EEPROM。

天气请求虽使用 HTTPS，但当前源码调用 `WiFiClientSecure::setInsecure()`，**不校验服务器证书**。EEPROM 也不是安全凭据存储。不要把设备、开放 AP 或 HTTP 接口暴露到公网、不可信网络或包含真实敏感数据的环境。

配置页不会回显已保存的 Wi-Fi 或 API Key；串口日志也不输出这些字段或天气响应内容。但设备本地仍会保存凭据：不要公开 EEPROM 导出、串口记录、照片、视频、截图、SSID、密码、API Key、私网地址、MAC 或网络拓扑。

完整接口与边界见 [docs/PROTOCOL.md](docs/PROTOCOL.md) 和 [SECURITY.md](SECURITY.md)。

## 构建

### 1. 准备本地配置（可选）

仓库自带安全的 `firmware/include/weather_clock_config.example.h`，空值会让设备走本地配置流程。若想在本地预置自己的测试配置：

```bash
cd firmware/include
cp weather_clock_config.example.h weather_clock_config.h
# 仅在本机填写自己的测试值；不要提交 weather_clock_config.h。
```

### 2. 编译固件

```bash
cd firmware
pio run
```

### 一键门禁

```bash
bash scripts/verify.sh
```

## 许可证、第三方与学习使用

- Rongyi 原创的固件、文档和源码推导图以 [MIT License](LICENSE) 公开；
- 依赖清单及其各自许可责任见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)；
- 本项目用于学习、实验、课程参考和二次开发。请保留来源说明，不要将其直接包装为个人课程设计、毕业设计、竞赛或商业产品成果；
- 使用者自行承担硬件、电气、网络、天气数据、密钥和适用性验证责任。

## 更多资料

- [来源与权威副本裁决](docs/SOURCE_PROVENANCE.md)
- [配网与协议说明](docs/PROTOCOL.md)
- [验证说明](docs/VERIFICATION.md)
- [GitHub 元数据](docs/GITHUB_METADATA.md)
- [Hardware Lab 索引卡片](docs/HARDWARE_LAB_CARD.md)
- [Hardware Lab](https://github.com/rongyishuaige7/hardware-lab)
