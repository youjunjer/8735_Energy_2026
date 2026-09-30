/*
  HUB-8735 Ultra 光敏電阻 OLED 顯示測試

  光敏電阻：IO0（類比輸入）
  OLED：SSD1306 128x64 I2C，位址 0x3C
  OLED 由 U8G2 自動啟動硬體 I2C，使用 IO3/IO4

  本程式不呼叫 Wire.begin()，由 U8G2 自動初始化 OLED。
  因此光敏電阻使用 IO0，不會與 OLED 的 IO3/IO4 衝突。
*/

#include "U8g2lib.h"

// OLED：SSD1306，解析度 128x64，由 U8G2 自動啟動硬體 I2C
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,
  /* reset = */ U8X8_PIN_NONE
);

// 光敏電阻接在 IO0
const int LIGHT_PIN = 0;

void setup() {
  // 設定光敏電阻為類比輸入
  pinMode(LIGHT_PIN, INPUT);

  // 初始化 OLED，I2C 位址為 0x3C
  u8g2.setI2CAddress(0x3C * 2);
  u8g2.begin();
}

void loop() {
  // 讀取光敏電阻類比值
  int lightValue = analogRead(LIGHT_PIN);

  // ===== 亮度比例換算（改用明確公式） =====
  // 原始值 0 為最亮，顯示 100%
  // 原始值 1024 為最暗，顯示 0%
  // 避免 HUB-8735 核心的 map() 相容性問題
  int brightnessPercent = ((1024L - lightValue) * 100L) / 1024L;
  brightnessPercent = constrain(brightnessPercent, 0, 100);

  // 清除 OLED 緩衝區
  u8g2.clearBuffer();

  // 使用較小字型，讓原始值與百分比能同時顯示
  u8g2.setFont(u8g2_font_6x12_tf);

  // 同時顯示光敏電阻原始值
  u8g2.setCursor(0, 20);
  u8g2.print("Raw: ");
  u8g2.print(lightValue);

  // 同時顯示換算後的亮度百分比
  u8g2.setCursor(0, 45);
  u8g2.print("Light: ");
  u8g2.print(brightnessPercent);
  u8g2.print("%");

  // 更新 OLED 畫面
  u8g2.sendBuffer();

  // 每 500 毫秒更新一次
  delay(500);
}
