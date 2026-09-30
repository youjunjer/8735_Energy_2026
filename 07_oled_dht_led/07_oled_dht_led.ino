/*
  HUB-8735 Ultra + DHT11 + SSD1306 OLED + status LEDs

  DHT11 DATA -> IO20
  Green LED  -> IO24 (normal)
  Yellow LED -> IO23 (humidity > 80%, dehumidifier simulation)
  Red LED    -> IO22 (temperature > 25 C, air-conditioner simulation)
  OLED       -> SSD1306 128x64, I2C address 0x3C, U8g2 hardware I2C

  U8g2 initializes the OLED's hardware I2C; do not call Wire.begin().
*/

#include "DHT.h"
#include "U8g2lib.h"

const uint8_t DHT_PIN = 20;
const uint8_t GREEN_LED_PIN = 24;
const uint8_t YELLOW_LED_PIN = 23;
const uint8_t RED_LED_PIN = 22;

#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,
  /* reset = */ U8X8_PIN_NONE
);

void setStatusLeds(bool humidityAbnormal, bool temperatureAbnormal) {
  // DHT readings are valid here: green means both readings are within limits.
  bool allNormal = !humidityAbnormal && !temperatureAbnormal;
  digitalWrite(GREEN_LED_PIN, allNormal ? HIGH : LOW);
  digitalWrite(YELLOW_LED_PIN, humidityAbnormal ? HIGH : LOW);
  digitalWrite(RED_LED_PIN, temperatureAbnormal ? HIGH : LOW);
}

void drawReadError() {
  // Turn every LED off when sensor data is invalid; do not report a false normal state.
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(15, 25, "DHT11 READ ERROR");
  u8g2.drawStr(15, 43, "Check sensor/wire");
  u8g2.sendBuffer();
}

void drawReadings(float temperature, float humidity,
                  bool humidityAbnormal, bool temperatureAbnormal) {
  u8g2.clearBuffer();
  u8g2.drawVLine(64, 5, 54);

  // Left panel: temperature and simulated air-conditioner state.
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(8, 11, "TEMP");
  u8g2.setFont(u8g2_font_ncenB12_tr);
  u8g2.setCursor(5, 34);
  u8g2.print(temperature, 1);
  u8g2.print(" C");
  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.drawStr(5, 46, ">25C = AC");
  u8g2.drawStr(5, 59, temperatureAbnormal ? "AC: ON" : "AC: OFF");

  // Right panel: humidity and simulated dehumidifier state.
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(75, 11, "HUMI");
  u8g2.setFont(u8g2_font_ncenB12_tr);
  u8g2.setCursor(69, 34);
  u8g2.print(humidity, 0);
  u8g2.print(" %");
  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.drawStr(69, 46, ">80% = DH");
  u8g2.drawStr(69, 59, humidityAbnormal ? "DH: ON" : "DH: OFF");

  u8g2.sendBuffer();
}

void setup() {
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);

  dht.begin();
  u8g2.setI2CAddress(0x3C * 2);
  u8g2.begin();

  delay(2000);
}

void loop() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    drawReadError();
  } else {
    // Strictly greater-than thresholds: exactly 25 C / 80% is still normal.
    bool temperatureAbnormal = temperature > 25.0f;
    bool humidityAbnormal = humidity > 80.0f;

    // Independent alarms can be active together; green is on only if both are normal.
    setStatusLeds(humidityAbnormal, temperatureAbnormal);
    drawReadings(temperature, humidity, humidityAbnormal, temperatureAbnormal);
  }

  // DHT11 should be read no more often than about once every 2 seconds.
  delay(2000);
}
