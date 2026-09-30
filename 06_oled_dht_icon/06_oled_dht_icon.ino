/*
  HUB-8735 Ultra + DHT11 + SSD1306 OLED

  DHT11 DATA -> IO20
  OLED：SSD1306 128x64，I2C 位址 0x3C，使用 U8g2 硬體 I2C
  OLED 由 U8g2 初始化，不呼叫 Wire.begin()。
  畫面分為左右兩區，分別顯示溫度與濕度及對應圖示。
*/

#include "DHT.h"
#include "U8g2lib.h"

// DHT11 感測器設定
const uint8_t DHT_PIN = 20;
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// 128x64 SSD1306 OLED，U8g2 硬體 I2C
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,
  /* reset = */ U8X8_PIN_NONE
);

// 繪製左側溫度計圖示
void drawThermometer(int x, int y) {
  u8g2.drawRFrame(x + 2, y, 5, 13, 2); // 縮小後的溫度計外框
  u8g2.drawCircle(x + 4, y + 13, 4);    // 底部圓球
  u8g2.drawVLine(x + 4, y + 5, 8);      // 內部液柱
  u8g2.drawDisc(x + 4, y + 13, 2);      // 液柱底部
}

// 繪製右側水滴圖示
void drawDroplet(int x, int y) {
  u8g2.drawTriangle(x + 5, y, x, y + 8, x + 10, y + 8); // 縮小後的水滴尖端
  u8g2.drawDisc(x + 5, y + 8, 5);                       // 水滴圓身
}

void setup() {
  // 初始化 DHT11
  dht.begin();

  // 設定 OLED 位址並初始化 U8g2
  u8g2.setI2CAddress(0x3C * 2);
  u8g2.begin();

  // 等待感測器上電穩定
  delay(2000);
}

void loop() {
  // 讀取攝氏溫度與相對濕度
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // 清空畫面緩衝區
  u8g2.clearBuffer();

  // 中間分隔線，形成左右兩個顯示區塊
  u8g2.drawVLine(64, 5, 54);

  if (isnan(temperature) || isnan(humidity)) {
    // 感測器讀取失敗時顯示提示
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(20, 32, "DHT11 read error");
  } else {
    // 左區：縮小的溫度計圖示與標籤，避免與右區重疊
    drawThermometer(8, 4);
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(20, 19, "TEMP");

    // 左區：縮小後的攝氏溫度數值
    u8g2.setFont(u8g2_font_ncenB12_tr);
    u8g2.setCursor(10, 53);
    u8g2.print(temperature, 0);
    u8g2.print("\xb0" "C");

    // 右區：縮小的水滴圖示與標籤
    drawDroplet(74, 5);
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(89, 19, "HUMI");

    // 右區：縮小後的相對濕度數值
    u8g2.setFont(u8g2_font_ncenB12_tr);
    u8g2.setCursor(82, 53);
    u8g2.print(humidity, 0);
    u8g2.print("%");
  }

  // 將畫面送到 OLED
  u8g2.sendBuffer();

  // DHT11 約每 2 秒讀取一次
  delay(2000);
}
