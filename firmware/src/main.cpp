#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <BH1750.h>
#include <RTClib.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <miniz.h>

#if __has_include("weather_clock_config.h")
#include "weather_clock_config.h"
#else
#include "weather_clock_config.example.h"
#endif
#include "wifi_config_html.h"

// 硬件对象
Adafruit_SSD1306 display(128, 64, &Wire, -1);
DHT dht(DHT_PIN, DHT22);
BH1750 bh1750;
RTC_DS3231 rtc;

// Web服务器
WebServer server(CONFIG_PORT);
DNSServer dnsServer;

// 全局变量
String weatherTemp = "--";
String weatherCond = "--";
String wifiSSID = "";
String wifiPassword = "";
String apiKey = "";
String locationId = "";

unsigned long lastWeatherUpdate = 0;
unsigned long lastSensorUpdate = 0;
unsigned long lastNtpSync = 0;
unsigned long lastDisplayUpdate = 0;
float humidity = 0;
float temperature = 0;
float lightLevel = -1;   // -1 表示尚未有效读数
bool hasSensorReading = false;  // 为 true 时表示 DHT 至少成功读过一次
bool weatherValid = false;
bool pendingRestart = false;
unsigned long restartRequestTime = 0;

// ============ WiFi配网相关 ============

// 截断到安全长度，避免 EEPROM 越界
static String truncateForEeprom(const String& s) {
  if (s.length() <= EEPROM_STR_MAX) return s;
  return s.substring(0, EEPROM_STR_MAX);
}

void saveConfig(String ssid, String password, String key, String loc) {
  EEPROM.begin(512);
  EEPROM.writeString(0, truncateForEeprom(ssid));
  EEPROM.writeString(64, truncateForEeprom(password));
  EEPROM.writeString(128, truncateForEeprom(key));
  EEPROM.writeString(192, truncateForEeprom(loc));
  EEPROM.commit();
  EEPROM.end();
}

// 从 EEPROM 安全读取字符串，避免 readString 越界读导致内存耗尽
static String readEepromString(int offset) {
  String s = EEPROM.readString(offset);
  if (s.length() > EEPROM_STR_MAX) s.remove(EEPROM_STR_MAX);
  return s;
}

void loadConfig() {
  EEPROM.begin(512);
  wifiSSID = readEepromString(0);
  wifiPassword = readEepromString(64);
  apiKey = readEepromString(128);
  locationId = readEepromString(192);
  EEPROM.end();

  // 首次使用或 EEPROM 为空时使用 weather_clock_config.h 默认 API 配置
  if (apiKey.length() == 0) apiKey = QWEATHER_API_KEY;
  if (locationId.length() == 0) locationId = QWEATHER_LOCATION;

  Serial.println("配置已从本地 EEPROM 读取（敏感字段不输出）");
}

void handleRoot() {
  // 分块输出本地自包含页面；绝不把设备保存的 Wi-Fi 或 API Key 回显到浏览器。
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html; charset=utf-8", "");
  const char* src = CONFIG_HTML;
  const size_t total = strlen(src);
  size_t sent = 0;
  while (sent < total) {
    size_t chunk = min((size_t)1024, total - sent);
    server.sendContent_P(src + sent, chunk);
    sent += chunk;
  }
  server.sendContent("");
}

void handleScan() {
  int n = WiFi.scanNetworks();
  StaticJsonDocument<1536> doc;  // 约 20 个 SSID × ~60 字节
  JsonArray arr = doc.to<JsonArray>();
  for (int i = 0; i < n; i++) {
    JsonObject obj = arr.add<JsonObject>();
    obj["ssid"] = WiFi.SSID(i);
    obj["rssi"] = WiFi.RSSI(i);
    obj["enc"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
  }
  WiFi.scanDelete();
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json; charset=utf-8", json);
}

void handleSave() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"missing_body\"}");
    return;
  }

  String body = server.arg("plain");
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"invalid_json\"}");
    return;
  }

  String ssid = doc["ssid"] | "";
  String password = doc["password"] | "";
  String apikey = doc["apikey"] | "";
  String location = doc["location"] | "";
  if (ssid.length() == 0 || apikey.length() == 0 || location.length() == 0 ||
      ssid.length() > EEPROM_STR_MAX || password.length() > EEPROM_STR_MAX ||
      apikey.length() > EEPROM_STR_MAX || location.length() > EEPROM_STR_MAX) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"invalid_fields\"}");
    return;
  }

  saveConfig(ssid, password, apikey, location);
  server.send(200, "application/json", "{\"success\":true}");

  // 延迟重启：先让 loop() 把 HTTP 响应发出去，再重启。
  pendingRestart = true;
  restartRequestTime = millis();
}

void showConfigPortalScreen() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("-- Config Mode --");

  display.setCursor(0, 14);
  display.println("WiFi: " AP_SSID);

  display.setCursor(0, 24);
  display.println("Open AP - local setup");

  display.setCursor(0, 38);
  display.println("Open browser:");
  display.setCursor(0, 48);
  display.println("192.168.4.1");

  display.display();
}

// Captive Portal 请求处理器 - 捕获所有未知请求并重定向到首页
class CaptiveRequestHandler : public RequestHandler {
  bool handle(WebServer* server, HTTPMethod method, String uri) {
    if (WiFi.getMode() == WIFI_AP) {
      server->sendHeader("Location", String("http://") + WiFi.softAPIP().toString());
      server->send(302, "text/plain", "");
    } else {
      server->send(404, "application/json", "{\"error\":\"not_found\"}");
    }
    return true;
  }
};

void initWebRoutes() {
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/scan", HTTP_GET, handleScan);
}

void startWebServer() {
  server.begin();
  Serial.println("本地配置 Web Server 已启动");
}

void startConfigPortal() {
  Serial.println("=== 启动配网模式 ===");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID);

  Serial.println("开放 AP 配网模式已启动；仅限隔离可信本地测试");

  // DNS重定向 - 捕获所有域名
  dnsServer.start(53, "*", WiFi.softAPIP());

  // Captive Portal 常见检测路径
  server.on("/generate_204", handleRoot);
  server.on("/fwlink", handleRoot);
  server.on("/connecttest.txt", handleRoot);
  server.on("/hotspot-detect.html", handleRoot);
  server.on("/library/test/success.html", handleRoot);
  server.addHandler(new CaptiveRequestHandler());

  // OLED 提示配网信息
  showConfigPortalScreen();

  Serial.println("配网页面已启动");
}

// ============ WiFi连接 ============

bool connectWiFi() {
  if (wifiSSID.length() == 0) return false;

  Serial.println("尝试连接已保存的 Wi-Fi（不输出 SSID 或密码）");
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi 连接成功");
    return true;
  }

  Serial.println("WiFi连接失败");
  return false;
}

// ============ NTP时间同步 ============

void syncNTP() {
  Serial.println("同步NTP时间...");
  configTime(GMT_OFFSET * 3600, DAYLIGHT_OFFSET, NTP_SERVER);

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    Serial.println("NTP时间获取成功");
    Serial.printf("%d-%02d-%02d %02d:%02d:%02d\n",
      timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    // 同步到RTC
    DateTime now = DateTime(
      timeinfo.tm_year + 1900,
      timeinfo.tm_mon + 1,
      timeinfo.tm_mday,
      timeinfo.tm_hour,
      timeinfo.tm_min,
      timeinfo.tm_sec
    );
    rtc.adjust(now);
    Serial.println("时间已同步到RTC");
  } else {
    Serial.println("NTP时间获取失败");
  }
}

// ============ 天气API ============

// 和风天气 icon 代码 → 英文短名（适配 OLED ASCII 显示）
// 参考: https://dev.qweather.com/docs/resource/icons/
static const char* weatherIconToText(int icon) {
  switch (icon) {
    case 100: return "Sunny";
    case 101: return "Cloudy";
    case 102: return "FewCloud";
    case 103: return "PtCloud";
    case 104: return "Overcast";
    case 150: return "Clear";
    case 151: return "Cloudy";
    case 152: return "FewCloud";
    case 153: return "PtCloud";
    case 300: case 301: return "Shower";
    case 302: case 303: return "T-Storm";
    case 304: return "Hail";
    case 305: return "LtRain";
    case 306: return "ModRain";
    case 307: return "HvRain";
    case 308: return "XHvRain";
    case 309: return "Drizzle";
    case 310: return "Storm";
    case 311: case 312: return "BigStrm";
    case 313: return "Freezing";
    case 314: case 315: return "LtRain";
    case 316: case 317: case 318: return "HvRain";
    case 350: case 351: return "Shower";
    case 399: return "Rain";
    case 400: return "LtSnow";
    case 401: return "ModSnow";
    case 402: return "HvSnow";
    case 403: return "Blizzard";
    case 404: case 405: case 406: case 407: return "Sleet";
    case 408: case 409: case 410: return "HvSnow";
    case 456: case 457: return "Sleet";
    case 499: return "Snow";
    case 500: return "Mist";
    case 501: return "Fog";
    case 502: return "Haze";
    case 503: case 504: return "Dust";
    case 507: case 508: return "SndStrm";
    case 509: case 510: case 514: case 515: return "Fog";
    case 511: return "MidHaze";
    case 512: return "HvHaze";
    case 513: return "SvrHaze";
    case 900: return "Hot";
    case 901: return "Cold";
    default:  return "N/A";
  }
}

// 手动跳过 gzip 头，返回 raw deflate 数据的起始偏移；失败返回 0
static size_t skipGzipHeader(const uint8_t* data, size_t len) {
  if (len < 10 || data[0] != 0x1F || data[1] != 0x8B || data[2] != 0x08) {
    return 0;
  }
  uint8_t flags = data[3];
  size_t pos = 10;
  if (flags & 0x04) { // FEXTRA
    if (pos + 2 > len) return 0;
    uint16_t xlen = data[pos] | (data[pos + 1] << 8);
    pos += 2 + xlen;
  }
  if (flags & 0x08) { // FNAME
    while (pos < len && data[pos] != 0) pos++;
    pos++;
  }
  if (flags & 0x10) { // FCOMMENT
    while (pos < len && data[pos] != 0) pos++;
    pos++;
  }
  if (flags & 0x02) { // FHCRC
    pos += 2;
  }
  return (pos < len) ? pos : 0;
}

// 解压 gzip 数据，返回解压后的 JSON 字符串；失败返回空串
// tinfl_decompressor 约 11KB，必须分配在堆上，否则 ESP32 loopTask 栈溢出
static String gzipInflate(const uint8_t* data, size_t len) {
  size_t deflateStart = skipGzipHeader(data, len);
  if (deflateStart == 0) {
    Serial.println("gzip头解析失败");
    return String();
  }

  if (len < deflateStart + 8) {
    Serial.println("gzip数据不完整");
    return String();
  }

  const uint8_t* deflateData = data + deflateStart;
  size_t deflateLen = len - deflateStart - 8;

  uint32_t origSize = data[len - 4] | (data[len - 3] << 8) |
                      (data[len - 2] << 16) | (data[len - 1] << 24);
  if (origSize == 0 || origSize > 8192) {
    origSize = 4096;
  }

  // 在堆上分配解压器（~11KB）和输出缓冲区
  tinfl_decompressor* decomp = (tinfl_decompressor*)malloc(sizeof(tinfl_decompressor));
  uint8_t* outBuf = (uint8_t*)malloc(origSize + 1);
  if (!decomp || !outBuf) {
    Serial.println("解压内存分配失败");
    free(decomp);
    free(outBuf);
    return String();
  }

  tinfl_init(decomp);

  size_t inBytes = deflateLen;
  size_t outBytes = origSize;

  tinfl_status status = tinfl_decompress(
    decomp,
    deflateData, &inBytes,
    outBuf, outBuf, &outBytes,
    TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF
  );

  String jsonStr;
  if ((status == TINFL_STATUS_DONE || outBytes > 0) && outBytes <= origSize) {
    outBuf[outBytes] = '\0';
    jsonStr = String((char*)outBuf);
    Serial.printf("解压成功: %d -> %d 字节\n", (int)len, (int)outBytes);
  } else {
    Serial.printf("tinfl 解压失败, status=%d\n", status);
  }

  free(decomp);
  free(outBuf);
  return jsonStr;
}

void updateWeather() {
  if (apiKey.length() == 0 || locationId.length() == 0) {
    Serial.println("天气API配置无效");
    return;
  }

  Serial.println("获取天气数据...");

  WiFiClientSecure client;
  client.setInsecure();

  if (!client.connect("devapi.qweather.com", 443)) {
    Serial.println("API连接失败");
    weatherValid = false;
    return;
  }

  String url = "/v7/weather/now?location=" + locationId + "&key=" + apiKey;
  client.print(String("GET ") + url + " HTTP/1.0\r\n" +
               "Host: devapi.qweather.com\r\n" +
               "Accept-Encoding: gzip\r\n" +
               "Connection: close\r\n\r\n");

  unsigned long timeout = millis();
  while (!client.available()) {
    if (millis() - timeout > 10000) {
      Serial.println("API响应超时");
      client.stop();
      weatherValid = false;
      return;
    }
    delay(10);
  }

  // 读取全部响应到缓冲区
  const size_t bufMax = 4096;
  uint8_t* buf = (uint8_t*)malloc(bufMax);
  if (!buf) {
    Serial.println("内存分配失败");
    client.stop();
    weatherValid = false;
    return;
  }

  size_t totalRead = 0;
  timeout = millis();
  while ((client.connected() || client.available()) && totalRead < bufMax) {
    if (millis() - timeout > 10000) break;
    if (client.available()) {
      int n = client.read(buf + totalRead, bufMax - totalRead);
      if (n > 0) { totalRead += n; timeout = millis(); }
    } else {
      delay(1);
    }
  }
  client.stop();

  Serial.println("天气响应已接收（内容不输出）");

  // 在缓冲区中查找 HTTP 头结束标记 \r\n\r\n
  size_t bodyStart = 0;
  for (size_t i = 0; i + 3 < totalRead; i++) {
    if (buf[i] == '\r' && buf[i+1] == '\n' && buf[i+2] == '\r' && buf[i+3] == '\n') {
      bodyStart = i + 4;
      break;
    }
  }

  if (bodyStart == 0 || bodyStart >= totalRead) {
    Serial.println("未找到HTTP头结束标记");
    free(buf);
    weatherValid = false;
    return;
  }

  uint8_t* body = buf + bodyStart;
  size_t bodyLen = totalRead - bodyStart;

  Serial.println("天气响应体已定位（内容不输出）");

  String jsonStr;

  if (bodyLen >= 2 && body[0] == 0x1F && body[1] == 0x8B) {
    Serial.println("检测到gzip, 正在解压...");
    jsonStr = gzipInflate(body, bodyLen);
    if (jsonStr.length() == 0) {
      Serial.println("gzip解压失败");
      free(buf);
      weatherValid = false;
      return;
    }
    Serial.println("gzip 天气响应已解压");
  } else {
    jsonStr.reserve(bodyLen + 1);
    for (size_t i = 0; i < bodyLen; i++) jsonStr += (char)body[i];
  }

  free(buf);

  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, jsonStr);

  if (!error && doc.containsKey("now") && doc["now"].containsKey("temp")) {
    weatherTemp = doc["now"]["temp"].as<String>();
    int iconCode = doc["now"]["icon"].as<int>();
    weatherCond = weatherIconToText(iconCode);
    weatherValid = true;
    Serial.println("天气字段已解析");
  } else {
    Serial.println("天气 JSON 解析失败（响应内容不输出）");
    weatherValid = false;
  }
}

// ============ 传感器读取 ============

void updateSensors() {
  // DHT22 温湿度
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (!isnan(h) && !isnan(t)) {
    humidity = h;
    temperature = t;
    hasSensorReading = true;
    Serial.println("DHT22 读数已更新");
  } else {
    Serial.println("DHT22读取失败");
  }

  // BH1750 光照（高分辨率模式约 120ms 一次有效）
  if (bh1750.measurementReady()) {
    float l = bh1750.readLightLevel();
    if (l >= 0 && !isnan(l)) {
      lightLevel = l;
      Serial.println("BH1750 读数已更新");
    }
  }
}

// ============ OLED显示 ============

// 在当前光标位置画一个 ° 符号（3x3 像素小圆圈）
static void drawDegreeSymbol(Adafruit_SSD1306& d, int16_t x, int16_t y) {
  d.drawPixel(x + 1, y,     SSD1306_WHITE);
  d.drawPixel(x,     y + 1, SSD1306_WHITE);
  d.drawPixel(x + 2, y + 1, SSD1306_WHITE);
  d.drawPixel(x + 1, y + 2, SSD1306_WHITE);
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  DateTime now = rtc.now();
  int sec = now.second();

  // 1. 时间 (大字体)  "HH MM" 冒号闪烁
  //    size3: 每字符 18x24, 冒号也是 18px 宽
  //    "HH:MM" 共 5 字符 = 90px, 居中 (128-90)/2 = 19
  display.setTextSize(3);
  char hh[3], mm[3];
  sprintf(hh, "%02d", now.hour());
  sprintf(mm, "%02d", now.minute());

  display.setCursor(19, 0);
  display.print(hh);
  if (sec % 2 == 0) {
    display.print(":");
  } else {
    display.print(" ");
  }
  display.print(mm);

  // 日期 (右对齐) 最长 "12/31" = 5字符 × 6px = 30px
  display.setTextSize(1);
  char dateStr[6];
  sprintf(dateStr, "%d/%d", now.month(), now.day());
  int dateWidth = strlen(dateStr) * 6;
  display.setCursor(128 - dateWidth, 25);
  display.print(dateStr);

  // 2. 天气状况 + 室外温度
  display.setCursor(0, 25);
  if (weatherValid) {
    display.print(weatherCond);
    display.print(" ");
    display.print(weatherTemp);
    int16_t cx = display.getCursorX();
    int16_t cy = display.getCursorY();
    drawDegreeSymbol(display, cx + 1, cy);
    display.setCursor(cx + 5, cy);
    display.print("C");
  } else {
    display.print("Weather: --");
  }

  // 3. 室内温湿度（hasSensorReading 避免未读到数据时误显示 0°C/0%）
  display.setCursor(0, 38);
  display.print("In:");
  if (hasSensorReading && !isnan(temperature)) {
    display.print((int)temperature);
    int16_t cx = display.getCursorX();
    int16_t cy = display.getCursorY();
    drawDegreeSymbol(display, cx + 1, cy);
    display.setCursor(cx + 5, cy);
    display.print("C");
  } else {
    display.print("--");
  }
  display.print(" ");
  if (hasSensorReading && !isnan(humidity) && humidity >= 0) {
    display.print((int)humidity);
    display.print("%");
  } else {
    display.print("--%");
  }

  // 光照
  display.setCursor(80, 38);
  if (lightLevel >= 0 && !isnan(lightLevel)) {
    display.print((int)lightLevel);
    display.print("Lx");
  } else {
    display.print("--Lx");
  }

  // 4. WiFi状态 (底部)
  display.setCursor(0, 54);
  if (WiFi.status() == WL_CONNECTED) {
    display.print("WiFi:OK");
  } else {
    display.print("WiFi:ERR");
  }

  display.display();
}

// ============ 初始化 ============

void initI2C() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.println("I2C初始化完成");
}

void initOLED() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED初始化失败!");
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(10, 20);
  display.println("Weather");
  display.setCursor(25, 45);
  display.setTextSize(1);
  display.println("Clock");
  display.display();
  delay(1500);
  Serial.println("OLED初始化完成");
}

void initSensors() {
  // DHT22
  dht.begin();
  Serial.println("DHT22初始化完成");

  // BH1750
  if (!bh1750.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("BH1750初始化失败!");
  } else {
    Serial.println("BH1750初始化完成");
  }

  // DS3231 RTC
  if (!rtc.begin()) {
    Serial.println("RTC初始化失败!");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC丢失电源, 设置默认时间");
    rtc.adjust(DateTime(2026, 1, 1, 0, 0, 0));
  }
  Serial.println("RTC初始化完成");
}

// ============ 主循环处理 ============

void handleWebRequests() {
  dnsServer.processNextRequest();
  server.handleClient();
}

// ============ SETUP ============

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== ESP32 Weather Clock 启动 ===");

  // 初始化I2C
  initI2C();

  // 初始化OLED
  initOLED();

  // 初始化传感器
  initSensors();

  // 加载配置
  loadConfig();

  // 预注册 Web 路由（不依赖网络，安全）
  initWebRoutes();

  // 尝试连接WiFi
  if (!connectWiFi()) {
    Serial.println("WiFi连接失败, 启动配网模式");
    startConfigPortal();
  } else {
    syncNTP();
    lastNtpSync = millis();
    updateWeather();
    lastWeatherUpdate = millis();
  }

  // WiFi 模式已确定（STA 或 AP），现在启动 Web Server
  startWebServer();

  updateSensors();
  lastSensorUpdate = millis();
  lastDisplayUpdate = millis();

  Serial.println("=== 系统启动完成 ===");
}

// ============ LOOP ============

void loop() {
  unsigned long currentMillis = millis();

  // 配网模式：处理 Web/DNS 请求，定期刷新 OLED 引导页，跳过其余逻辑
  if (WiFi.getMode() == WIFI_AP) {
    handleWebRequests();

    // 延迟重启：等 1.5 秒让 HTTP 200 响应真正通过 TCP 发出去
    if (pendingRestart && (currentMillis - restartRequestTime >= 1500)) {
      Serial.println("配置已保存，重启中...");
      delay(100);
      ESP.restart();
    }

    if (currentMillis - lastDisplayUpdate >= 5000) {
      showConfigPortalScreen();
      lastDisplayUpdate = currentMillis;
    }
    delay(10);
    return;
  }

  // 传感器更新 (30秒)
  if (currentMillis - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL * 1000UL) {
    updateSensors();
    lastSensorUpdate = currentMillis;
  }

  // 天气更新 (10分钟)
  if (WiFi.status() == WL_CONNECTED &&
      currentMillis - lastWeatherUpdate >= WEATHER_UPDATE_INTERVAL * 60 * 1000UL) {
    updateWeather();
    lastWeatherUpdate = currentMillis;
  }

  // WiFi 断线重连；连续失败 3 次则切换到 AP 配网模式
  static int reconnectFails = 0;
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi断开, 尝试重连...");
    if (connectWiFi()) {
      reconnectFails = 0;
      syncNTP();
      lastNtpSync = currentMillis;
      updateWeather();
      lastWeatherUpdate = currentMillis;
    } else {
      reconnectFails++;
      Serial.printf("重连失败 (%d/3)\n", reconnectFails);
      if (reconnectFails >= 3) {
        Serial.println("多次重连失败，切换到配网模式");
        reconnectFails = 0;
        startConfigPortal();
        return;
      }
    }
  }

  // 定期 NTP 再同步（避免 RTC 长期漂移）
  if (WiFi.status() == WL_CONNECTED &&
      (currentMillis - lastNtpSync) >= (unsigned long)NTP_RESYNC_INTERVAL * 60 * 60 * 1000UL) {
    syncNTP();
    lastNtpSync = currentMillis;
  }

  // STA 模式下也处理 Web 请求（支持远程配置修改）
  server.handleClient();

  // 延迟重启（STA 模式下保存配置后也需要）
  if (pendingRestart && (currentMillis - restartRequestTime >= 1500)) {
    Serial.println("配置已保存，重启中...");
    delay(100);
    ESP.restart();
  }

  // 显示限频：约 1 秒刷新一次，降低 OLED 负载
  if (currentMillis - lastDisplayUpdate >= 1000) {
    updateDisplay();
    lastDisplayUpdate = currentMillis;
  }

  delay(100);
}
