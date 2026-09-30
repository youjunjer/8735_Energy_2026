/*
  HUB-8735 Ultra + DHT11 + SSD1306 OLED

  DHT11 DATA -> IO20
  DHT11 VCC  -> 依感測器模組標示接電源
  DHT11 GND  -> GND

  OLED 使用 U8g2、SSD1306 128x64、I2C 位址 0x3C。
  由 U8g2 啟動硬體 I2C，不呼叫 Wire.begin()。
*/

#include "DHT.h"       // HUB-8735 AmebaPro2 核心內建 DHT 函式庫
#include "U8g2lib.h"   // OLED 顯示函式庫

// DHT11 感測器資料腳位與型號
const uint8_t DHT_PIN = 20;
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// SSD1306 128x64 OLED，使用 U8g2 硬體 I2C
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,
  /* reset = */ U8X8_PIN_NONE
);

void setup() {
  // 啟動 DHT11
  dht.begin();

  // 設定 OLED I2C 位址（U8g2 API 位址需左移一位）並初始化
  u8g2.setI2CAddress(0x3C * 2);
  u8g2.begin();

  // DHT11 上電後稍待穩定
  delay(2000);
}

void loop() {
  // 讀取攝氏溫度與相對濕度
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // 清除 OLED 畫面緩衝區
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);

  // DHT11 讀取失敗時顯示錯誤訊息
  if (isnan(temperature) || isnan(humidity)) {
    u8g2.drawStr(0, 20, "DHT11 read error");
  } else {
    // 顯示溫度，例如：TEMP: 25 C
    u8g2.setCursor(0, 20);
    u8g2.print("TEMP: ");
    u8g2.print(temperature, 0);
    u8g2.print(" C");

    // 顯示濕度，例如：HUMI: 58 %
    u8g2.setCursor(0, 45);
    u8g2.print("HUMI: ");
    u8g2.print(humidity, 0);
    u8g2.print(" %");
  }

  // 將畫面送到 OLED
  u8g2.sendBuffer();

  // DHT11 官方範例使用約 2 秒間隔，避免讀取過於頻繁
  delay(2000);
}
