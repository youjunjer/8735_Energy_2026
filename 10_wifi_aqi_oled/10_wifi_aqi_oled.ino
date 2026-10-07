/*
  HUB-8735 Ultra + Wi-Fi + Environmental Data API + SSD1306 OLED

  Wi-Fi connects to the configured network, retrieves the MOENV AQI API
  once per minute, finds the Taoyuan / Zhongli monitoring station, and
  displays AQI and PM2.5 on the OLED.

  OLED: SSD1306 128x64, I2C address 0x3C, U8g2 hardware I2C
  U8g2 initializes hardware I2C; do not call Wire.begin().
*/

#include <WiFi.h>
#include <WiFiSSLClient.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>

char WIFI_SSID[] = "a";
const char WIFI_PASSWORD[] = "12345678";

const char API_HOST[] = "data.moenv.gov.tw";
const char API_PATH[] = "/api/v2/AQX_P_432?api_key=6b143ef9-f251-43e4-add6-a5557c7ffa1c";

const unsigned long FETCH_INTERVAL_MS = 60000UL;
const unsigned long WIFI_RETRY_INTERVAL_MS = 10000UL;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,
  /* reset = */ U8X8_PIN_NONE
);

WiFiSSLClient httpsClient;

unsigned long lastFetchMs = 0;
unsigned long lastWiFiRetryMs = 0;
bool hasData = false;
String latestAqi;
String latestPm25;
String latestPublishTime;

void drawMessage(const char* line1, const char* line2 = nullptr) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 25, line1);
  if (line2 != nullptr) {
    u8g2.drawStr(0, 43, line2);
  }
  u8g2.sendBuffer();
}

void drawAirQuality() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "Zhongli, Taoyuan");

  u8g2.setFont(u8g2_font_ncenB12_tr);
  u8g2.setCursor(0, 34);
  u8g2.print("AQI: ");
  u8g2.print(latestAqi);

  u8g2.setCursor(0, 55);
  u8g2.print("PM2.5: ");
  u8g2.print(latestPm25);
  u8g2.print(" ug");

  u8g2.sendBuffer();
}

void drawDataError() {
  if (hasData) {
    drawAirQuality();
    return;
  }
  drawMessage("data error", "No AQI data");
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  drawMessage("WiFi connecting....");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  drawMessage("WiFi connected");
  delay(1500);
}

bool skipHttpHeaders(WiFiSSLClient& client, bool& isChunked) {
  String line;
  unsigned long startMs = millis();
  isChunked = false;

  while (millis() - startMs < 15000UL) {
    while (client.available()) {
      char c = static_cast<char>(client.read());
      if (c == '\n') {
        if (line.length() == 0 || line == "\r") {
          return true;
        }
        if (line.indexOf("Transfer-Encoding: chunked") >= 0) {
          isChunked = true;
        }
        line = "";
      } else if (c != '\r') {
        line += c;
      }
    }

    if (!client.connected() && !client.available()) {
      break;
    }
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

  // The API currently uses HTTP/1.1 Transfer-Encoding: chunked.
  // Remove each hexadecimal chunk-size line before JSON parsing.
  String sizeLine;
  while (true) {
    if (!readHttpLine(client, sizeLine)) return false;
    int separator = sizeLine.indexOf(';');
    if (separator >= 0) sizeLine = sizeLine.substring(0, separator);
    unsigned long chunkSize = strtoul(sizeLine.c_str(), nullptr, 16);

    if (chunkSize == 0) {
      // Consume optional trailing headers.
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
      size_t toRead = remaining > 256UL ? 256U : static_cast<size_t>(remaining);
      uint8_t buffer[256];
      int count = client.read(buffer, toRead);
      if (count <= 0) return false;
      for (int i = 0; i < count; ++i) {
        body += static_cast<char>(buffer[i]);
      }
      remaining -= static_cast<unsigned long>(count);
    }

    // Every chunk is followed by CRLF.
    if (!readHttpLine(client, sizeLine) || sizeLine.length() != 0) return false;
  }
}

bool fetchZhongliAirQuality() {
  httpsClient.stop();
  httpsClient.setRecvTimeout(15000);

  // The AmebaPro2 SSL client can establish the HTTPS connection without
  // embedding a CA certificate. The API key remains in the request URL.
  if (!httpsClient.connect(API_HOST, 443)) {
    return false;
  }

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

  // Keep only fields needed for the Zhongli record while parsing the array.
  JsonDocument filter;
  filter[0]["county"] = true;
  filter[0]["sitename"] = true;
  filter[0]["aqi"] = true;
  filter[0]["pm2.5"] = true;
  filter[0]["pm2.5_avg"] = true;
  filter[0]["publishtime"] = true;

  JsonDocument document;
  DeserializationError error = deserializeJson(
    document,
    responseBody,
    DeserializationOption::Filter(filter)
  );
  httpsClient.stop();

  if (error) {
    return false;
  }

  bool found = false;
  for (JsonObject record : document.as<JsonArray>()) {
    const char* county = record["county"] | "";
    const char* siteName = record["sitename"] | "";
    if (strcmp(county, "桃園市") != 0 || strcmp(siteName, "中壢") != 0) {
      continue;
    }

    const char* aqi = record["aqi"] | "";
    const char* pm25 = record["pm2.5"] | "";
    const char* pm25Average = record["pm2.5_avg"] | "";
    const char* publishTime = record["publishtime"] | "";

    if (aqi[0] == '\0') {
      return false;
    }

    latestAqi = aqi;
    latestPm25 = pm25[0] != '\0' ? pm25 : pm25Average;
    latestPublishTime = publishTime;
    found = latestPm25.length() > 0;
    break;
  }

  return found;
}

void updateAirQuality() {
  drawMessage("data updating....");

  if (fetchZhongliAirQuality()) {
    hasData = true;
    drawAirQuality();
  } else {
    drawDataError();
  }
}

void setup() {
  u8g2.setI2CAddress(0x3C * 2);
  u8g2.begin();

  connectWiFi();
  updateAirQuality();
  lastFetchMs = millis();
}

void loop() {
  unsigned long now = millis();

  if (WiFi.status() != WL_CONNECTED) {
    if ((unsigned long)(now - lastWiFiRetryMs) >= WIFI_RETRY_INTERVAL_MS) {
      lastWiFiRetryMs = now;
      connectWiFi();
    }
    return;
  }

  if ((unsigned long)(now - lastFetchMs) >= FETCH_INTERVAL_MS) {
    lastFetchMs = now;
    updateAirQuality();
  }

  delay(10);
}
