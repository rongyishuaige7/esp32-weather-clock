# 第三方依赖与声明

本仓库未复制第三方库源码。固件通过 PlatformIO 拉取下列依赖；它们仍受各上游项目的许可证、NOTICE、商标和使用条款约束。使用或再分发时，请在你的精确依赖版本中核对完整许可证文本。

| 依赖 | 固定版本 | 用途 | 上游 |
| :-- | :-- | :-- | :-- |
| Arduino-ESP32 / espressif32 | `6.13.0` 平台 | ESP32 Arduino 框架与构建平台 | [PlatformIO espressif32](https://registry.platformio.org/platforms/platformio/espressif32) |
| Adafruit GFX Library | `1.12.5` | 显示绘制基础 | [GitHub](https://github.com/adafruit/Adafruit-GFX-Library) |
| Adafruit SSD1306 | `2.5.16` | OLED 驱动 | [GitHub](https://github.com/adafruit/Adafruit_SSD1306) |
| DHT sensor library | `1.4.6` | DHT22 读取 | [GitHub](https://github.com/adafruit/DHT-sensor-library) |
| BH1750 | `1.3.0` | BH1750 光照读取 | [GitHub](https://github.com/claws/BH1750) |
| RTClib | `2.1.4` | DS3231 RTC | [GitHub](https://github.com/adafruit/RTClib) |
| ArduinoJson | `7.4.3` | JSON 解析与配置处理 | [GitHub](https://github.com/bblanchon/ArduinoJson) |
| micro-miniz | `1.0.0` | gzip/deflate 解压 | [PlatformIO Registry](https://registry.platformio.org/libraries/rzeldent/micro-miniz) |

和风天气服务仅通过用户自行提供的 API Key 访问；其服务、数据、账户与商标不随本仓库授权。请阅读其当前 [开发者文档与条款](https://dev.qweather.com/)。

`docs/`、`hardware/`、`scripts/` 与 `firmware/src/main.cpp` 的本仓库原创整理部分适用 [MIT License](LICENSE)，但这不替代上游依赖和外部服务的许可证或条款。
