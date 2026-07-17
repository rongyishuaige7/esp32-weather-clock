# 安全边界与漏洞报告

这是一个 ESP32 天气时钟教学原型，不是网络安全设备、可信时间源、气象服务、环境监测仪表或生产系统。

## 当前网络边界

- AP 配网、`/scan` 和 `/save` 都使用无认证、无 TLS 的 HTTP；没有会话、授权、请求签名或设备身份机制；
- 公开候选使用 `WeatherClock-Setup` 开放 AP，刻意不保留固定密码。开放 AP 仅用于短时、隔离、可信本地测试，不能暴露公网；
- `/scan` 会暴露设备附近的 SSID、信号强度和加密标记给能连上该 AP 的客户端；
- `/save` 会接收 Wi-Fi SSID、密码、天气 Key 和城市 ID，并以明文写入 EEPROM；**EEPROM 不是安全凭据存储**；
- 天气请求当前调用 `WiFiClientSecure::setInsecure()`，不校验 HTTPS 服务器证书，不能把它描述为安全连接；
- 当前配置页不回显已保存凭据，固件串口也不输出凭据或天气响应体，但任何本地 EEPROM 导出仍可能包含敏感数据。

仅在隔离、可信的本地网络中使用。不要在 Issue、日志、截图、视频、EEPROM 导出、Git 或媒体中发布 API Key、SSID、密码、私网 IP、MAC、个人位置、网络拓扑或设备唯一标识。

## 当前真机边界

当前候选尚未按公开 commit 重新烧录并验证 ESP32、DHT22、BH1750、DS3231、OLED、Wi-Fi、NTP、EEPROM、AP 配网或天气 API。请先阅读 [docs/PROJECT_STATUS.md](docs/PROJECT_STATUS.md) 与 [docs/VERIFICATION.md](docs/VERIFICATION.md)。

## 报告方式

请不要在公开 Issue 中贴出密钥、密码、私网信息或可复用攻击细节。请通过 GitHub 账户主页的公开联系方式进行最小化说明；在收到确认前，不要公开可能影响其他复现者的细节。
