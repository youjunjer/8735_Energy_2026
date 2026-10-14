/*
  HUB-8735 Ultra + DHT11 + OLED + MQTT

  DHT11 DATA -> IO20
  OLED       -> SSD1306 128x64, I2C address 0x3C
  MQTT       -> mqttgo.io:1883
  Topic      -> eric/class105/data
  Payload    -> {"temp":25,"humi":72}

  The OLED shows Wi-Fi/MQTT connection and publish status.
  One temperature/humidity payload is published every 15 seconds.
*/

#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <U8g2lib.h>

char WIFI_SSID[] = "a";
const char WIFI_PASSWORD[] = "12345678";

const char MQTT_SERVER[] = "mqttgo.io";
const uint16_t MQTT_PORT = 1883;
const char MQTT_TOPIC[] = "eric/class105/data";
const char MQTT_CLIENT_ID[] = "HUB8735_eric_class105";

const uint8_t DHT_PIN = 20;
const unsigned long SENSOR_INTERVAL_MS = 2000UL;
const unsigned long PUBLISH_INTERVAL_MS = 15000UL;
const unsigned long WIFI_RETRY_INTERVAL_MS = 5000UL;
const unsigned long MQTT_RETRY_INTERVAL_MS = 5000UL;

DHT dht(DHT_PIN, DHT11);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

unsigned long lastSensorReadMs = 0;
unsigned long lastPublishMs = 0;
unsigned long lastWiFiRetryMs = 0;
unsigned long lastMqttRetryMs = 0;
float temperature = NAN;
float humidity = NAN;
bool hasSensorData = false;
String lastStatus = "Starting...";

void drawStatus(const char* line1, const char* line2 = nullptr) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 25, line1);
  if (line2 != nullptr) u8g2.drawStr(0, 43, line2);
  u8g2.sendBuffer();
}

void drawDataPage() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "MQTT SENSOR DATA");
  u8g2.setFont(u8g2_font_ncenB12_tr);
  u8g2.setCursor(0, 34);
  u8g2.print("T: ");
  if (hasSensorData) u8g2.print(temperature, 0);
  else u8g2.print("--");
  u8g2.print(" C");
  u8g2.setCursor(0, 57);
  u8g2.print("H: ");
  if (hasSensorData) u8g2.print(humidity, 0);
  else u8g2.print("--");
  u8g2.print(" %");
  u8g2.sendBuffer();
}

void readSensor() {
  float newTemperature = dht.readTemperature();
  float newHumidity = dht.readHumidity();
  if (isnan(newTemperature) || isnan(newHumidity)) {
    hasSensorData = false;
    drawStatus("DHT11 read error", "Check sensor/wire");
    return;
  }
  temperature = newTemperature;
  humidity = newHumidity;
  hasSensorData = true;
  drawDataPage();
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  drawStatus("WiFi connecting...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastStatus = "WiFi connecting...";
}

bool connectMqtt() {
  if (mqttClient.connected()) return true;
  drawStatus("MQTT connecting...", MQTT_SERVER);
  bool connected = mqttClient.connect(MQTT_CLIENT_ID);
  if (connected) {
    drawStatus("MQTT connected", MQTT_TOPIC);
    lastStatus = "MQTT connected";
    delay(1000);
  } else {
    drawStatus("MQTT connect fail", "Retrying...");
    lastStatus = "MQTT connect fail";
  }
  return connected;
}

bool publishSensorData() {
  if (!hasSensorData || !mqttClient.connected()) return false;

  char payload[64];
  int temp = static_cast<int>(temperature + 0.5f);
  int humi = static_cast<int>(humidity + 0.5f);
  snprintf(payload, sizeof(payload), "{\"temp\":%d,\"humi\":%d}", temp, humi);

  drawStatus("data publishing...", payload);
  bool published = mqttClient.publish(MQTT_TOPIC, payload);
  if (published) {
    drawStatus("publish success", payload);
    lastStatus = "publish success";
  } else {
    drawStatus("publish failed", "MQTT retry");
    lastStatus = "publish failed";
  }
  delay(1000);
  drawDataPage();
  return published;
}

void setup() {
  dht.begin();
  u8g2.setI2CAddress(0x3C * 2);
  u8g2.begin();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  drawStatus("WiFi connecting...");
  connectWiFi();
}

void loop() {
  unsigned long now = millis();

  if (WiFi.status() != WL_CONNECTED) {
    if ((unsigned long)(now - lastWiFiRetryMs) >= WIFI_RETRY_INTERVAL_MS) {
      lastWiFiRetryMs = now;
      connectWiFi();
    }
    delay(10);
    return;
  }

  if (lastStatus == "WiFi connecting...") {
    drawStatus("WiFi connected");
    lastStatus = "WiFi connected";
    delay(1000);
  }

  if (!mqttClient.connected()) {
    if ((unsigned long)(now - lastMqttRetryMs) >= MQTT_RETRY_INTERVAL_MS) {
      lastMqttRetryMs = now;
      connectMqtt();
    }
  } else {
    mqttClient.loop();
  }

  if ((unsigned long)(now - lastSensorReadMs) >= SENSOR_INTERVAL_MS) {
    lastSensorReadMs = now;
    readSensor();
  }

  if (mqttClient.connected() &&
      (unsigned long)(now - lastPublishMs) >= PUBLISH_INTERVAL_MS) {
    lastPublishMs = now;
    publishSensorData();
  }

  delay(10);
}
