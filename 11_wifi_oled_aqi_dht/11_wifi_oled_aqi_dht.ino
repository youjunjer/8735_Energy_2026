/*
  HUB-8735 Ultra integrated energy/environment monitor

  DHT11 DATA -> IO20
  Green LED  -> IO24 (temperature and humidity normal)
  Yellow LED -> IO23 (humidity > 80%)
  Red LED    -> IO22 (temperature > 25 C)
  Buzzer     -> IO11 (PWM; fire-siren alarm)
  OLED       -> SSD1306 128x64, I2C address 0x3C

  The OLED alternates between the DHT11 page and the Zhongli AQI page.
  AQI data is fetched once per minute from the MOENV API.
*/

#include <WiFi.h>
#include <WiFiSSLClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <U8g2lib.h>

char WIFI_SSID[] = "a";
const char WIFI_PASSWORD[] = "12345678";
const char API_HOST[] = "data.moenv.gov.tw";
const char API_PATH[] = "/api/v2/AQX_P_432?api_key=6b143ef9-f251-43e4-add6-a5557c7ffa1c";

const uint8_t DHT_PIN = 20;
const uint8_t GREEN_LED_PIN = 24;
const uint8_t YELLOW_LED_PIN = 23;
const uint8_t RED_LED_PIN = 22;
const uint8_t BUZZER_PIN = 11;

const unsigned long SENSOR_INTERVAL_MS = 2000UL;
const unsigned long AQI_INTERVAL_MS = 60000UL;
const unsigned long PAGE_INTERVAL_MS = 5000UL;
const unsigned long WIFI_RETRY_INTERVAL_MS = 10000UL;
const int SIREN_LOW_HZ = 650;
const int SIREN_HIGH_HZ = 1250;
const int SIREN_STEP_HZ = 10;
const unsigned long SIREN_STEP_INTERVAL_MS = 15UL;

DHT dht(DHT_PIN, DHT11);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
WiFiSSLClient httpsClient;

unsigned long lastSensorReadMs = 0;
unsigned long lastAqiFetchMs = 0;
unsigned long lastPageMs = 0;
unsigned long lastWiFiRetryMs = 0;
unsigned long lastSirenStepMs = 0;

float temperature = NAN;
float humidity = NAN;
bool temperatureAbnormal = false;
bool humidityAbnormal = false;
bool alarmActive = false;
bool sirenStarted = false;
int sirenFrequency = SIREN_LOW_HZ;
int sirenDirection = 1;
uint8_t displayPage = 0;
bool hasAqiData = false;
String latestAqi;
String latestPm25;

void drawMessage(const char* line1, const char* line2 = nullptr) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 25, line1);
  if (line2 != nullptr) u8g2.drawStr(0, 43, line2);
  u8g2.sendBuffer();
}

void drawDhtPage() {
  if (isnan(temperature) || isnan(humidity)) {
    drawMessage("DHT11 READ ERROR", "Check sensor/wire");
    return;
  }

  u8g2.clearBuffer();
  u8g2.drawVLine(64, 5, 54);
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(8, 11, "TEMP");
  u8g2.setFont(u8g2_font_ncenB12_tr);
  u8g2.setCursor(5, 34);
  u8g2.print(temperature, 1);
  u8g2.print(" C");
  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.drawStr(5, 46, ">25C = AC");
  u8g2.drawStr(5, 59, temperatureAbnormal ? "AC: ON" : "AC: OFF");

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

void drawAqiPage() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "Zhongli, Taoyuan");
  if (!hasAqiData) {
    u8g2.drawStr(0, 35, "NO AQI DATA");
    u8g2.sendBuffer();
    return;
  }
  u8g2.setFont(u8g2_font_ncenB12_tr);
  u8g2.setCursor(0, 34);
  u8g2.print("AQI: ");
  u8g2.print(latestAqi);
  u8g2.setCursor(0, 55);
  u8g2.print("PM2.5: ");
  u8g2.print(latestPm25);
  u8g2.sendBuffer();
}

void drawCurrentPage() {
  if (displayPage == 0) drawDhtPage();
  else drawAqiPage();
}

void setStatusLeds() {
  bool allNormal = !temperatureAbnormal && !humidityAbnormal;
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
    lastSirenStepMs = now;
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
    lastSirenStepMs = now;
  }
}

void readDht() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  if (isnan(temperature) || isnan(humidity)) {
    temperatureAbnormal = false;
    humidityAbnormal = false;
    setStatusLeds();
    stopSiren();
    return;
  }
  temperatureAbnormal = temperature > 25.0f;
  humidityAbnormal = humidity > 80.0f;
  setStatusLeds();
  alarmActive = temperatureAbnormal || humidityAbnormal;
  if (!alarmActive) stopSiren();
  drawCurrentPage();
}

bool skipHttpHeaders(WiFiSSLClient& client, bool& isChunked) {
  String line;
  isChunked = false;
  unsigned long startMs = millis();
  while (millis() - startMs < 15000UL) {
    while (client.available()) {
      char c = static_cast<char>(client.read());
      if (c == '\n') {
        if (line.length() == 0) return true;
        if (line.indexOf("Transfer-Encoding: chunked") >= 0) isChunked = true;
        line = "";
      } else if (c != '\r') line += c;
    }
    if (!client.connected() && !client.available()) break;
    delay(1);
  }
  return false;
}

bool readHttpLine(WiFiSSLClient& client, String& line) {
  line = "";
  unsigned long startMs = millis();
  while (millis() - startMs < 15000UL) {
    while (client.available()) {
      char c = static_cast<char>(client.read());
      if (c == '\n') return true;
      if (c != '\r') line += c;
    }
    if (!client.connected() && !client.available()) return line.length() > 0;
    delay(1);
  }
  return false;
}

bool readResponseBody(WiFiSSLClient& client, bool isChunked, String& body) {
  body = "";
  if (!isChunked) {
    unsigned long startMs = millis();
    while (millis() - startMs < 20000UL) {
      while (client.available()) {
        body += static_cast<char>(client.read());
        startMs = millis();
      }
      if (!client.connected() && !client.available()) return true;
      delay(1);
    }
    return false;
  }

  String sizeLine;
  while (true) {
    if (!readHttpLine(client, sizeLine)) return false;
    int separator = sizeLine.indexOf(';');
    if (separator >= 0) sizeLine = sizeLine.substring(0, separator);
    unsigned long chunkSize = strtoul(sizeLine.c_str(), nullptr, 16);
    if (chunkSize == 0) {
      do {
        if (!readHttpLine(client, sizeLine)) return false;
      } while (sizeLine.length() > 0);
      return true;
    }

    unsigned long remaining = chunkSize;
    while (remaining > 0) {
      while (!client.available()) {
        if (!client.connected()) return false;
        delay(1);
      }
      uint8_t buffer[256];
      size_t toRead = remaining > 256UL ? 256U : static_cast<size_t>(remaining);
      int count = client.read(buffer, toRead);
      if (count <= 0) return false;
      for (int i = 0; i < count; ++i) body += static_cast<char>(buffer[i]);
      remaining -= static_cast<unsigned long>(count);
    }
    if (!readHttpLine(client, sizeLine) || sizeLine.length() != 0) return false;
  }
}

bool fetchZhongliAirQuality() {
  httpsClient.stop();
  httpsClient.setRecvTimeout(15000);
  if (!httpsClient.connect(API_HOST, 443)) return false;
  httpsClient.print("GET ");
  httpsClient.print(API_PATH);
  httpsClient.println(" HTTP/1.1");
  httpsClient.print("Host: ");
  httpsClient.println(API_HOST);
  httpsClient.println("Connection: close");
  httpsClient.println();

  bool isChunked = false;
  if (!skipHttpHeaders(httpsClient, isChunked)) {
    httpsClient.stop();
    return false;
  }
  String responseBody;
  if (!readResponseBody(httpsClient, isChunked, responseBody)) {
    httpsClient.stop();
    return false;
  }

  JsonDocument filter;
  filter[0]["county"] = true;
  filter[0]["sitename"] = true;
  filter[0]["aqi"] = true;
  filter[0]["pm2.5"] = true;
  filter[0]["pm2.5_avg"] = true;
  JsonDocument document;
  DeserializationError error = deserializeJson(
    document, responseBody, DeserializationOption::Filter(filter));
  httpsClient.stop();
  if (error) return false;

  for (JsonObject record : document.as<JsonArray>()) {
    const char* county = record["county"] | "";
    const char* siteName = record["sitename"] | "";
    if (strcmp(county, "桃園市") != 0 || strcmp(siteName, "中壢") != 0) continue;
    const char* aqi = record["aqi"] | "";
    const char* pm25 = record["pm2.5"] | "";
    const char* pm25Average = record["pm2.5_avg"] | "";
    if (aqi[0] == '\0') return false;
    latestAqi = aqi;
    latestPm25 = pm25[0] != '\0' ? pm25 : pm25Average;
    return latestPm25.length() > 0;
  }
  return false;
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  drawMessage("WiFi connecting....");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  drawMessage("WiFi connected");
  delay(1500);
}

void updateAqi() {
  drawMessage("data updating....");
  if (fetchZhongliAirQuality()) hasAqiData = true;
  drawCurrentPage();
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
  connectWiFi();
  updateAqi();
  lastSensorReadMs = millis() - SENSOR_INTERVAL_MS;
  lastAqiFetchMs = millis();
  lastPageMs = millis();
}

void loop() {
  unsigned long now = millis();
  if (WiFi.status() != WL_CONNECTED) {
    if ((unsigned long)(now - lastWiFiRetryMs) >= WIFI_RETRY_INTERVAL_MS) {
      lastWiFiRetryMs = now;
      connectWiFi();
    }
  } else if ((unsigned long)(now - lastAqiFetchMs) >= AQI_INTERVAL_MS) {
    lastAqiFetchMs = now;
    updateAqi();
  }

  if ((unsigned long)(now - lastSensorReadMs) >= SENSOR_INTERVAL_MS) {
    lastSensorReadMs = now;
    readDht();
  }
  if ((unsigned long)(now - lastPageMs) >= PAGE_INTERVAL_MS) {
    lastPageMs = now;
    displayPage = displayPage == 0 ? 1 : 0;
    drawCurrentPage();
  }
  updateSiren();
  delay(10);
}
