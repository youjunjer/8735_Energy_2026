/*
  HUB-8735 Ultra RTSP face recognition with Wi-Fi web controls.

  RTSP: HD H.264 video with OSD face boxes and names
  Web:  http://<board-ip>/ on port 80

  The web buttons replace the serial commands from the official example:
    REG=<name>    register the face currently in front of the camera
    DEL=<name>    remove a registered face
    RESET         clear all registered faces
    BACKUP        save registered faces to flash
    RESTORE       load registered faces from flash
*/

#include "WiFi.h"
#include "StreamIO.h"
#include "VideoStream.h"
#include "RTSP.h"
#include "NNFaceDetectionRecognition.h"
#include "VideoStreamOverlay.h"
#include "U8g2lib.h"

#define CHANNEL       0
#define CHANNEL_NN    3
#define NN_WIDTH      576
#define NN_HEIGHT     320

VideoSetting videoConfig(VIDEO_FHD, 30, VIDEO_H264, 0);
VideoSetting nnConfig(NN_WIDTH, NN_HEIGHT, 10, VIDEO_RGB, 0);
NNFaceDetectionRecognition faceRecognition;
RTSP rtsp;
StreamIO videoStreamer(1, 1);
StreamIO videoStreamerRGB(1, 1);
WiFiServer webServer(80);

char ssid[] = "a";
char pass[] = "12345678";
int wifiStatus = WL_IDLE_STATUS;
IPAddress boardIp;
int rtspPort = 0;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
String lastOperation = "Ready";
String lastFaceName = "none";
uint16_t lastFaceCount = 0;

void drawStatus(const char *line1, const char *line2 = nullptr,
                const char *line3 = nullptr) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 16, line1);
    if (line2 != nullptr) u8g2.drawStr(0, 34, line2);
    if (line3 != nullptr) u8g2.drawStr(0, 52, line3);
    u8g2.sendBuffer();
}

void drawNetworkInfo() {
    char ipText[24];
    char rtspText[24];
    snprintf(ipText, sizeof(ipText), "IP:%u.%u.%u.%u",
             boardIp[0], boardIp[1], boardIp[2], boardIp[3]);
    snprintf(rtspText, sizeof(rtspText), "WEB:80 RTSP:%d", rtspPort);
    drawStatus("WiFi connected", ipText, rtspText);
}

String urlDecode(String value) {
    String decoded;
    for (uint16_t i = 0; i < value.length(); i++) {
        char c = value[i];
        if (c == '+') {
            decoded += ' ';
        } else if (c == '%' && i + 2 < value.length()) {
            char hex[3] = {value[i + 1], value[i + 2], '\0'};
            decoded += static_cast<char>(strtol(hex, nullptr, 16));
            i += 2;
        } else {
            decoded += c;
        }
    }
    return decoded;
}

String queryValue(const String &request, const char *key) {
    String marker = String(key) + "=";
    int start = request.indexOf(marker);
    if (start < 0) return String();
    start += marker.length();
    int end = request.indexOf('&', start);
    if (end < 0) end = request.indexOf(' ', start);
    if (end < 0) end = request.length();
    return urlDecode(request.substring(start, end));
}

void setOperation(const char *message) {
    lastOperation = message;
    drawNetworkInfo();
}

void handleAction(const String &request) {
    if (request.indexOf("GET /register?") >= 0) {
        String name = queryValue(request, "name");
        if (name.length() == 0) {
            setOperation("Register failed: empty name");
        } else {
            faceRecognition.registerFace(name);
            lastOperation = "Registered: " + name;
            drawNetworkInfo();
        }
    } else if (request.indexOf("GET /delete?") >= 0) {
        String name = queryValue(request, "name");
        if (name.length() == 0) {
            setOperation("Delete failed: empty name");
        } else {
            faceRecognition.removeFace(name);
            lastOperation = "Deleted: " + name;
            drawNetworkInfo();
        }
    } else if (request.indexOf("GET /reset ") >= 0) {
        faceRecognition.resetRegisteredFace();
        setOperation("All faces cleared");
    } else if (request.indexOf("GET /backup ") >= 0) {
        faceRecognition.backupRegisteredFace();
        setOperation("Faces backed up");
    } else if (request.indexOf("GET /restore ") >= 0) {
        faceRecognition.restoreRegisteredFace();
        setOperation("Faces restored");
    }
}

void printWebPage(WiFiClient &client) {
    client.println(F("HTTP/1.1 200 OK"));
    client.println(F("Content-Type: text/html; charset=utf-8"));
    client.println(F("Connection: close"));
    client.println();
    client.println(F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"));
    client.println(F("<title>8735 Face Control</title><style>body{font-family:Arial;max-width:620px;margin:20px auto;padding:0 12px}button{padding:12px;margin:5px;font-size:16px}input{padding:12px;font-size:16px;width:55%}.card{border:1px solid #ccc;border-radius:8px;padding:12px;margin:12px 0}small{color:#555}</style></head><body>"));
    client.println(F("<h2>HUB-8735 人臉辨識控制</h2>"));
    client.print(F("<div class='card'><b>網頁：</b>http://"));
    client.print(boardIp);
    client.println(F("/<br><b>RTSP：</b>rtsp://"));
    client.print(boardIp);
    client.print(':');
    client.print(rtspPort);
    client.println(F("</div>"));
    client.println(F("<div class='card'><h3>登錄人臉</h3><form action='/register' method='get'><input name='name' placeholder='輸入姓名' maxlength='24' required><button type='submit'>登錄目前人臉</button></form><small>請確認畫面中只有一張臉。</small></div>"));
    client.println(F("<div class='card'><h3>刪除人臉</h3><form action='/delete' method='get'><input name='name' placeholder='輸入要刪除的姓名' maxlength='24' required><button type='submit'>刪除</button></form></div>"));
    client.println(F("<div class='card'><h3>資料管理</h3><a href='/backup'><button>備份至 Flash</button></a><a href='/restore'><button>從 Flash 還原</button></a><a href='/reset' onclick=\"return confirm('確定清除全部人臉？')\"><button>清除全部</button></a></div>"));
    client.print(F("<div class='card'><h3>辨識狀態</h3><b>目前偵測數量：</b>"));
    client.print(lastFaceCount);
    client.print(F("<br><b>最近辨識：</b>"));
    client.print(lastFaceName);
    client.print(F("<br><b>最近操作：</b>"));
    client.print(lastOperation);
    client.println(F("<br><br><a href='/'><button>重新整理</button></a></div></body></html>"));
}

void handleWebClient() {
    WiFiClient client = webServer.available();
    if (!client) return;

    client.setTimeout(500);
    String request = client.readStringUntil('\r');
    handleAction(request);
    while (client.connected() && client.available()) {
        client.read();
    }
    printWebPage(client);
    delay(2);
    client.stop();
}

void connectWiFi() {
    while (wifiStatus != WL_CONNECTED) {
        drawStatus("WiFi connecting...", "SSID: a");
        Serial.println("Attempting to connect to WPA SSID: a");
        wifiStatus = WiFi.begin(ssid, pass);
        delay(2000);
    }
    boardIp = WiFi.localIP();
    webServer.begin();
}

void FRPostProcess(std::vector<FaceRecognitionResult> results) {
    uint16_t imageHeight = videoConfig.height();
    uint16_t imageWidth = videoConfig.width();
    lastFaceCount = faceRecognition.getResultCount();
    lastFaceName = "none";
    OSD.createBitmap(CHANNEL);

    for (uint16_t i = 0; i < lastFaceCount; i++) {
        FaceRecognitionResult item = results[i];
        lastFaceName = item.name();
        int xmin = static_cast<int>(item.xMin() * imageWidth);
        int xmax = static_cast<int>(item.xMax() * imageWidth);
        int ymin = static_cast<int>(item.yMin() * imageHeight);
        int ymax = static_cast<int>(item.yMax() * imageHeight);
        uint32_t color = String(item.name()) == String("unknown")
                             ? OSD_COLOR_RED : OSD_COLOR_GREEN;
        OSD.drawRect(CHANNEL, xmin, ymin, xmax, ymax, 3, color);
        char text[40];
        snprintf(text, sizeof(text), "Face:%s", item.name());
        OSD.drawText(CHANNEL, xmin, ymin - OSD.getTextHeight(CHANNEL), text, color);
    }
    OSD.update(CHANNEL);
}

void setup() {
    Serial.begin(115200);
    u8g2.setI2CAddress(0x3C * 2);
    u8g2.begin();
    drawStatus("Face web control", "Starting...");
    connectWiFi();

    videoConfig.setBitrate(2 * 1024 * 1024);
    Camera.configVideoChannel(CHANNEL, videoConfig);
    Camera.configVideoChannel(CHANNEL_NN, nnConfig);
    Camera.videoInit();

    rtsp.configVideo(videoConfig);
    rtsp.begin();
    rtspPort = rtsp.getPort();

    faceRecognition.configVideo(nnConfig);
    faceRecognition.modelSelect(FACE_RECOGNITION, NA_MODEL, DEFAULT_SCRFD, DEFAULT_MOBILEFACENET);
    faceRecognition.begin();
    faceRecognition.setResultCallback(FRPostProcess);
    faceRecognition.restoreRegisteredFace();

    videoStreamer.registerInput(Camera.getStream(CHANNEL));
    videoStreamer.registerOutput(rtsp);
    videoStreamer.begin();
    Camera.channelBegin(CHANNEL);

    videoStreamerRGB.registerInput(Camera.getStream(CHANNEL_NN));
    videoStreamerRGB.setStackSize();
    videoStreamerRGB.setTaskPriority();
    videoStreamerRGB.registerOutput(faceRecognition);
    videoStreamerRGB.begin();
    Camera.channelBegin(CHANNEL_NN);

    OSD.configVideo(CHANNEL, videoConfig);
    OSD.begin();
    drawNetworkInfo();

    Serial.print("Web control: http://");
    Serial.print(boardIp);
    Serial.println("/");
    Serial.print("RTSP: rtsp://");
    Serial.print(boardIp);
    Serial.print(":");
    Serial.println(rtspPort);
}

void loop() {
    handleWebClient();
    delay(10);
}
