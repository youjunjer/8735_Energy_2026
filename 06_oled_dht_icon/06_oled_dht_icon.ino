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

// 繪製左側全高溫度計，液柱依 10～40°C 線性變化
void drawThermometer(int x, int y, float temperature) {
  const int liquidBottom = y + 40;
  const int liquidMaxHeight = 34;

  // 將液柱限制在 10～40°C；溫度數字仍顯示感測器實際讀值
  float boundedTemperature = temperature;
  if (boundedTemperature < 10.0f) boundedTemperature = 10.0f;
  if (boundedTemperature > 40.0f) boundedTemperature = 40.0f;
  int liquidHeight = (int)(((boundedTemperature - 10.0f) * liquidMaxHeight / 30.0f) + 0.5f);

  // 溫度計外框與底部球體，總高度約 55 像素
  u8g2.drawRFrame(x + 3, y, 10, 43, 5);
  u8g2.drawCircle(x + 8, y + 47, 8);

  // 固定填滿球體；直管液柱高度依實際溫度改變
  u8g2.drawDisc(x + 8, y + 47, 4);
  if (liquidHeight > 0) {
    u8g2.drawBox(x + 6, liquidBottom - liquidHeight, 4, liquidHeight);
  }
}

// 繪製右側全高水滴，內部填充依 0～100% 濕度由下往上變化
void drawDroplet(int x, int y, float humidity) {
  const int centerX = x + 11;
  const int tipY = y;
  const int shoulderY = y + 18;
  const int sideBottomY = y + 42;
  const int bottomY = y + 54;

  // 將水滴填充高度限制在 0～100%
  float boundedHumidity = humidity;
  if (boundedHumidity < 0.0f) boundedHumidity = 0.0f;
  if (boundedHumidity > 100.0f) boundedHumidity = 100.0f;
  // 尖端與最底端都納入高度計算，共 55 個像素列
  int fillHeight = (int)((boundedHumidity * (bottomY - tipY + 1) / 100.0f) + 0.5f);
  int fillStartY = bottomY + 1 - fillHeight;

  // 水滴外框：尖端、兩側與圓弧底部，整體高度約 55 像素
  u8g2.drawLine(centerX, tipY, x, shoulderY);
  u8g2.drawLine(centerX, tipY, x + 22, shoulderY);
  u8g2.drawVLine(x, shoulderY, sideBottomY - shoulderY + 1);
  u8g2.drawVLine(x + 22, shoulderY, sideBottomY - shoulderY + 1);

  // 以折線描繪圓弧底部，避免使用完整圓形產生內部橫線
  const int arcX[] = {0, 1, 4, 7, 11, 15, 18, 21, 22};
  const int arcY[] = {42, 46, 51, 53, 54, 53, 51, 46, 42};
  for (int i = 0; i < 8; i++) {
    u8g2.drawLine(x + arcX[i], y + arcY[i], x + arcX[i + 1], y + arcY[i + 1]);
  }

  // 依每一列水滴外形計算可填充寬度，讓液面輪廓留在水滴外框內
  // 圓弧由 y+42 收至最底端 y+54；各列半寬與外框輪廓對齊
  const int bottomHalfWidth[] = {0, 10, 10, 10, 10, 9, 8, 8, 7, 7, 6, 5, 0};
  for (int row = fillStartY; row <= bottomY; row++) {
    int halfWidth = 0;
    int offsetY = row - y;

    if (row == tipY) {
      halfWidth = 0;
    } else if (row < shoulderY) {
      halfWidth = (offsetY * 11 / 18) - 1;
    } else if (row <= sideBottomY) {
      halfWidth = 10;
    } else {
      int arcIndex = row - sideBottomY;
      if (arcIndex >= 0 && arcIndex < 13) {
        halfWidth = bottomHalfWidth[arcIndex];
      }
    }

    if (halfWidth >= 0) {
      u8g2.drawHLine(centerX - halfWidth, row, halfWidth * 2 + 1);
    }
  }
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
    // 左區：全高溫度計，10°C 最低、40°C 最高
    drawThermometer(5, 4, temperature);
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(23, 18, "TEMP");

    // 左區：顯示實際攝氏溫度
    u8g2.setFont(u8g2_font_ncenB12_tr);
    u8g2.setCursor(22, 37);
    u8g2.print(temperature, 0);
    u8g2.print("\xb0" "C");

    // 標示溫度計液柱的有效範圍
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(22, 53, "10-40 C");

    // 右區：全高水滴，內部填充依 0～100% 濕度變化
    drawDroplet(103, 4, humidity);

    // 濕度標籤與讀值放在水滴左側
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(68, 18, "HUMI");

    // 顯示實際相對濕度
    u8g2.setFont(u8g2_font_ncenB12_tr);
    u8g2.setCursor(68, 37);
    u8g2.print(humidity, 0);
    u8g2.print("%");

    // 標示水滴填充範圍
    u8g2.setFont(u8g2_font_5x8_tf);
    u8g2.drawStr(68, 53, "0-100%");
  }

  // 將畫面送到 OLED
  u8g2.sendBuffer();

  // DHT11 約每 2 秒讀取一次
  delay(2000);
}
