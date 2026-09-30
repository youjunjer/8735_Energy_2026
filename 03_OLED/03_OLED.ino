/*
  縮網址：twgo.io/oleds
  此為 OLED 範例程式，須使用 U8G2 程式庫。
  原始 U8G2 程式庫的中文字型有限，老師提供的版本約有 7000 個中文字。
  益師父版本下載：twgo.io/vygya

  OLED 接線：
    VCC -> 3.3V
    GND -> GND
    SDA -> IO0
    SCL -> IO1
*/

#include "Wire.h"
#include "U8g2lib.h"

// 0.96 吋 OLED：SSD1306，解析度 128*64
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,
  /* reset = */ U8X8_PIN_NONE
);

// 1.3 吋 OLED 若使用 SH1106，才使用下列宣告：
// U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(
//   U8G2_R0,
//   /* reset = */ U8X8_PIN_NONE
// );

int i = 0;

void setup() {
  // 啟動 HUB-8735 第一組硬體 I2C（SDA=IO0、SCL=IO1）
  //Wire.begin();

  // 初始化 OLED
  u8g2.begin();

  // 啟用 UTF-8 中文字串輸出
  u8g2.enableUTF8Print();

  // 設定中文字型
  u8g2.setFont(u8g2_font_unifont_t_chinese1);

  // 座標從文字區域上方開始計算
  u8g2.setFontPosTop();
}

void loop() {
  // 每次更新畫面前先清除繪圖緩衝區
  u8g2.clearBuffer();

  // 顯示第一行中文
  u8g2.setCursor(0, 5);
  u8g2.print("屏東科技大學");

  // 顯示第二行中文與測試數值
  u8g2.setCursor(0, 25);
  u8g2.print("溫濕度：123456789");

  // 顯示第三行中文與遞增計數值
  u8g2.setCursor(0, 45);
  u8g2.print("目前已啟動：");
  u8g2.print(i++);

  // 將繪圖緩衝區送到 OLED
  u8g2.sendBuffer();

  // 每秒更新一次
  delay(1000);
}
