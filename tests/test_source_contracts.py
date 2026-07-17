from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


def read(rel: str) -> str:
    return (ROOT / rel).read_text(encoding='utf-8')


class SourceContracts(unittest.TestCase):
    def test_platform_and_hardware_contract(self):
        ini = read('firmware/platformio.ini')
        main = read('firmware/src/main.cpp')
        self.assertIn('platform = espressif32@6.13.0', ini)
        self.assertIn('board = esp32dev', ini)
        self.assertIn('DHT dht(DHT_PIN, DHT22)', main)
        self.assertIn('Wire.begin(I2C_SDA, I2C_SCL)', main)
        self.assertIn('display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)', main)
        self.assertIn('BH1750::CONTINUOUS_HIGH_RES_MODE', main)
        self.assertIn('RTC_DS3231 rtc', main)

    def test_public_config_is_empty_and_local_config_is_ignored(self):
        example = read('firmware/include/weather_clock_config.example.h')
        ignored = read('.gitignore') + read('firmware/.gitignore')
        self.assertIn('#define DEFAULT_WIFI_SSID ""', example)
        self.assertIn('#define DEFAULT_WIFI_PASSWORD ""', example)
        self.assertIn('#define QWEATHER_API_KEY ""', example)
        self.assertIn('firmware/include/weather_clock_config.h', ignored)
        self.assertIn('include/weather_clock_config.h', ignored)
        self.assertFalse((ROOT / 'firmware/include/weather_clock_config.h').exists())

    def test_no_credential_echo_or_fixed_ap_password(self):
        main = read('firmware/src/main.cpp')
        html = read('firmware/include/wifi_config_html.h')
        self.assertIn('WiFi.softAP(AP_SSID);', main)
        self.assertNotIn('AP_PASSWORD', main)
        self.assertNotIn('server.sendContent(apiKey)', main)
        self.assertNotIn('Serial.println("SSID: "', main)
        self.assertNotIn('Serial.println("API Key: "', main)
        self.assertNotIn('fonts.googleapis.com', html)
        self.assertNotIn('QWEATHER_DEFAULT_KEY', html)
        self.assertIn('value=""', html)

    def test_protocol_risk_is_explicit_in_code_and_docs(self):
        main = read('firmware/src/main.cpp')
        protocol = read('docs/PROTOCOL.md')
        security = read('SECURITY.md')
        self.assertIn('client.setInsecure();', main)
        self.assertIn('WiFi.softAP(AP_SSID);', main)
        self.assertIn('无认证、无 TLS', protocol)
        self.assertIn('不校验证书', protocol)
        self.assertIn('EEPROM 不是安全凭据存储', security)

    def test_save_rejects_missing_and_oversized_credential_fields(self):
        main = read('firmware/src/main.cpp')
        self.assertIn('missing_body', main)
        self.assertIn('invalid_json', main)
        self.assertIn('invalid_fields', main)
        self.assertIn('apikey.length() > EEPROM_STR_MAX', main)
        self.assertIn('password.length() > EEPROM_STR_MAX', main)


if __name__ == '__main__':
    unittest.main()
