/*
  HUB-8735 Ultra + DHT11 + SSD1306 OLED + status LEDs + passive buzzer

  DHT11 DATA -> IO20
  Green LED  -> IO24 (normal)
  Yellow LED -> IO23 (humidity > 80%, dehumidifier simulation)
  Red LED    -> IO22 (temperature > 25 C, air-conditioner simulation)
  Buzzer     -> IO11 (PWM; fire-siren alarm)
  OLED       -> SSD1306 128x64, I2C address 0x3C, U8g2 hardware I2C

  U8g2 initializes the OLED's hardware I2C; do not call Wire.begin().
*/

#include "DHT.h"
#include "U8g2lib.h"

const uint8_t DHT_PIN = 20;
const uint8_t GREEN_LED_PIN = 24;
const uint8_t YELLOW_LED_PIN = 23;
const uint8_t RED_LED_PIN = 22;
const uint8_t BUZZER_PIN = 11;

const unsigned long SENSOR_INTERVAL_MS = 2000UL;
const unsigned long SIREN_STEP_INTERVAL_MS = 15UL;
const int SIREN_LOW_HZ = 650;
const int SIREN_HIGH_HZ = 1250;
const int SIREN_STEP_HZ = 10;

#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,
  /* reset = */ U8X8_PIN_NONE
);

unsigned long lastSensorReadMs = 0;
unsigned long lastSirenStepMs = 0;
bool alarmActive = false;
bool sirenStarted = false;
int sirenFrequency = SIREN_LOW_HZ;
int sirenDirection = 1;

void setStatusLeds(bool humidityAbnormal, bool temperatureAbnormal) {
  bool allNormal = !humidityAbnormal && !temperatureAbnormal;
  digitalWrite(GREEN_LED_PIN, allNormal ? HIGH : LOW);
  digitalWrite(YELLOW_LED_PIN, humidityAbnormal ? HIGH : LOW);
  digitalWrite(RED_LED_PIN, temperatureAbnormal ? HIGH : LOW);
}

void stopSiren() {
  alarmActive = false;
  if (sirenStarted) {
    noTone(BUZZER_PIN);
    sirenStarted = false;
  }
  sirenFrequency = SIREN_LOW_HZ;
  sirenDirection = 1;
}

void updateSiren() {
  if (!alarmActive) return;

  unsigned long now = millis();
  if (!sirenStarted) {
    tone(BUZZER_PIN, sirenFrequency);
    sirenStarted = true;
    lastSirenStepMs = millis();
    return;
  }

  if ((unsigned long)(now - lastSirenStepMs) >= SIREN_STEP_INTERVAL_MS) {
    sirenFrequency += sirenDirection * SIREN_STEP_HZ;
    if (sirenFrequency >= SIREN_HIGH_HZ) {
      sirenFrequency = SIREN_HIGH_HZ;
      sirenDirection = -1;
    } else if (sirenFrequency <= SIREN_LOW_HZ) {
      sirenFrequency = SIREN_LOW_HZ;
      sirenDirection = 1;
    }

    tone(BUZZER_PIN, sirenFrequency);
    lastSirenStepMs = millis();
  }
}

void drawReadError() {
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

  // Give the DHT11 time to stabilize before its first reading.
  delay(SENSOR_INTERVAL_MS);
  lastSensorReadMs = millis() - SENSOR_INTERVAL_MS;
}

void loop() {
  unsigned long now = millis();
  if ((unsigned long)(now - lastSensorReadMs) >= SENSOR_INTERVAL_MS) {
    lastSensorReadMs = now;
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (isnan(temperature) || isnan(humidity)) {
      stopSiren();
      drawReadError();
    } else {
      // Strictly greater-than thresholds: 25 C and 80% are still normal.
      bool temperatureAbnormal = temperature > 25.0f;
      bool humidityAbnormal = humidity > 80.0f;

      setStatusLeds(humidityAbnormal, temperatureAbnormal);
      drawReadings(temperature, humidity, humidityAbnormal, temperatureAbnormal);

      // Sound the siren if either reading is abnormal; silence it when both are normal.
      alarmActive = temperatureAbnormal || humidityAbnormal;
      if (!alarmActive) stopSiren();
    }
  }

  // Keep the alarm sweep updating while sensor readings remain valid and abnormal.
  updateSiren();
  delay(10);
}
