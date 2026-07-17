# 源码来源与权威副本裁决

> 状态日期：2026-07-17

## 只读来源

```text
/home/rongyi/桌面/esp32_weather_clock
/mnt/shared/2026项目/esp32_weather_clock.zip
```

历史 ZIP SHA-256：

```text
09f3363acd82b8c1f82e6fede2047752621e57fc1fc67682b004cd8af6c0006f
```

按有效源码文件规则（忽略目录/缓存/构建产物），两个来源各有 4 个文件，逐文件比较没有差异：

```text
桌面来源 manifest SHA-256: 32b631e0b0b9c15e7cc06771e456b948fa033f05985d440f858408565c226236
ZIP 来源 manifest SHA-256:  32b631e0b0b9c15e7cc06771e456b948fa033f05985d440f858408565c226236
```

## 裁决

- 桌面原工程与历史 ZIP 当前选定源码完全一致；
- 桌面原工程是本轮公开整理的权威源码来源；
- 历史 ZIP 是只读封存基线；
- 公开候选目录是 `/home/rongyi/桌面/esp32-weather-clock`；
- 原工程和 ZIP 始终只读，公开候选不会反向覆盖、清理或删除它们。

## 可审计公开整理

原始 `weather_clock_config.h` 含非占位的天气 API Key 与固定 AP 密码，不能复制到公开候选。公开候选据此完成以下净化和可复现性整理：

1. 将原 `main.ino` 重组为 PlatformIO 的 `firmware/src/main.cpp`，并保留原配网页主体到 `firmware/include/wifi_config_html.h`；
2. 用 `firmware/include/weather_clock_config.example.h` 代替真实配置；`weather_clock_config.h` 被 Git 忽略，候选内没有真实 Wi-Fi、密码或 API Key；
3. 移除串口中的 SSID、密码、API Key、实际 IP、天气响应体和读数回显；
4. 取消把设备内存中的天气 Key 注入配网页，页面默认空白且不回显已保存凭据；
5. 移除配网页的 Google Fonts 外部导入，页面保持本地自包含；
6. 取消固定 AP 密码，改为开放 AP，并明确其只适合短时、隔离、可信本地配置；
7. 新增中文 README、BOM、源码推导接线边界图、安全/协议/验证文件、敏感信息门禁与 CI。

这些是公开前净化、文档和构建加固，不得被描述为当前真机、网络安全、传感器、天气准确性或长期稳定性验证。
