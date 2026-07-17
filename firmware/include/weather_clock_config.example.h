#ifndef WEATHER_CLOCK_CONFIG_H
#define WEATHER_CLOCK_CONFIG_H

// 本文件可用于本地构建。复制为 weather_clock_config.h 后再填写自己的值；weather_clock_config.h 不应提交。
// 留空表示启动后进入本地 AP 配网页面，不会预置真实 Wi-Fi 或天气凭据。
#define DEFAULT_WIFI_SSID ""
#define DEFAULT_WIFI_PASSWORD ""
#define QWEATHER_API_KEY ""
#define QWEATHER_LOCATION ""

// 更新间隔
#define WEATHER_UPDATE_INTERVAL 10  // 分钟
#define SENSOR_UPDATE_INTERVAL 5    // 秒

// 硬件接口
#define DHT_PIN 4
#define I2C_SDA 21
#define I2C_SCL 22
#define OLED_ADDR 0x3C
#define BH1750_ADDR 0x23
#define DS3231_ADDR 0x68

// 本地配置 AP。开放 AP 只用于短时、隔离、可信环境；不是安全机制。
#define AP_SSID "WeatherClock-Setup"
#define CONFIG_PORT 80

// NTP
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET 8
#define DAYLIGHT_OFFSET 0
#define NTP_RESYNC_INTERVAL 24

// EEPROM 配置
#define EEPROM_STR_MAX 62

#endif  // WEATHER_CLOCK_CONFIG_H
