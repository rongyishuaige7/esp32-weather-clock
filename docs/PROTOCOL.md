# 配网与本地配置协议


## 阶段 A：开放 AP 配网

当 EEPROM 中没有 SSID，或 STA 自动连接连续失败时，固件创建开放 AP：

```text
SSID: WeatherClock-Setup
密码：无
配置页：http://192.168.4.1/
```

DNS 将常见 Captive Portal 探测路径重定向到配置页。开放 AP 和配置接口均为**无认证、无 TLS**，也没有会话、授权或请求签名，只能短时用于隔离、可信的本地配置。手机/电脑连接它后可：

| 方法 | 路径 | 当前源码行为 | 边界 |
| :-- | :-- | :-- | :-- |
| `GET` | `/` | 返回本地自包含配置页。 | 不回显 EEPROM 中已保存的 Wi-Fi 或 API Key。 |
| `GET` | `/scan` | 扫描附近 Wi-Fi，返回 SSID、RSSI、加密标记数组。 | 会向 AP 上客户端暴露附近 SSID/信号；不应在不可信场所运行。 |
| `POST` | `/save` | 接收 `ssid`、`password`、`apikey`、`location` JSON 并写入 EEPROM，随后延迟重启。 | 明文 HTTP；长度与必填字段检查不等于认证、加密或安全存储。 |

配置成功 HTTP `200` 仅表示 handler 已尝试保存并安排重启；它不证明配置真的可用、Wi-Fi 已加入、API Key 有效或天气可获得。浏览器超时也不能被当作保存成功。

## 阶段 B：STA、NTP 与天气请求

设备尝试使用 EEPROM 中的 Wi-Fi 配置加入 STA。成功后调用 NTP 同步 RTC，并向和风天气接口发起 HTTPS 请求。它没有对外声明稳定 REST 服务；未注册的路径由捕获处理器返回 JSON `404`。

天气请求当前调用：

```cpp
client.setInsecure();
```

这意味着 TLS 不校验证书。即使能收到响应，也不应把该链路称为安全连接、准确天气数据或持续在线服务。

## 配置与日志禁止项

不得公开或收集：Wi-Fi SSID、密码、天气 API Key、真实城市/位置偏好、私网 IP、MAC、EEPROM 导出、附近网络扫描结果、家庭/实验室拓扑、截图 EXIF/GPS、串口日志中的可识别信息。
