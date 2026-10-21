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
#define CHANNEL_JPEG  1
#define CHANNEL_NN    3
#define NN_WIDTH      576
#define NN_HEIGHT     320

VideoSetting videoConfig(VIDEO_FHD, 30, VIDEO_H264, 0);
// Lower-resolution preview keeps the web control page responsive.
VideoSetting jpegConfig(480, 270, CAM_FPS, VIDEO_JPEG, 1);
VideoSetting nnConfig(NN_WIDTH, NN_HEIGHT, 10, VIDEO_RGB, 0);
NNFaceDetectionRecognition faceRecognition;
RTSP rtsp;
StreamIO videoStreamer(1, 1);
StreamIO videoStreamerRGB(1, 1);
WiFiServer webServer(80);
WiFiServer mjpegServer(8080);
WiFiClient mjpegClient;
bool mjpegStreaming = false;
uint32_t jpegImageAddress = 0;
uint32_t jpegImageLength = 0;
unsigned long lastMjpegFrameMs = 0;

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
    client.println(F("<div class='card'><h3>即時影像預覽</h3><img style='width:100%;max-width:640px' src='http://"));
    client.print(boardIp);
    client.println(F(":8080/stream' alt='MJPEG preview'></div>"));
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

void startMjpegClient() {
    if (mjpegStreaming) return;
    WiFiClient candidate = mjpegServer.available();
    if (!candidate) return;

    candidate.setTimeout(500);
    String line;
    bool blankLine = false;
    unsigned long deadline = millis() + 1000;
    while (candidate.connected() && (long)(deadline - millis()) > 0) {
        if (!candidate.available()) {
            delay(1);
            continue;
        }
        char c = candidate.read();
        deadline = millis() + 1000;
        if (c == '\n') {
            if (line.length() == 0) {
                blankLine = true;
                break;
            }
            line = "";
        } else if (c != '\r') {
            line += c;
        }
    }

    if (!blankLine) {
        candidate.stop();
        return;
    }

    candidate.print(F("HTTP/1.1 200 OK\r\n"));
    candidate.print(F("Content-Type: multipart/x-mixed-replace; boundary=8735frame\r\n"));
    candidate.print(F("Cache-Control: no-cache\r\nConnection: close\r\n\r\n"));
    mjpegClient = candidate;
    mjpegStreaming = true;
    lastMjpegFrameMs = 0;
}

void sendMjpegFrame() {
    if (!mjpegStreaming) return;
    if (!mjpegClient.connected()) {
        mjpegClient.stop();
        mjpegStreaming = false;
        return;
    }

    // Limit preview to about 8 FPS so the control page remains responsive.
    if ((unsigned long)(millis() - lastMjpegFrameMs) < 125) return;
    lastMjpegFrameMs = millis();

    Camera.getImage(CHANNEL_JPEG, &jpegImageAddress, &jpegImageLength);
    if (jpegImageAddress == 0 || jpegImageLength == 0) return;

    char header[128];
    int headerLength = snprintf(
        header, sizeof(header),
        "--8735frame\r\nContent-Type: image/jpeg\r\nContent-Length: %lu\r\n\r\n",
        static_cast<unsigned long>(jpegImageLength));
    mjpegClient.write(reinterpret_cast<const uint8_t *>(header), headerLength);
    mjpegClient.write(reinterpret_cast<const uint8_t *>(jpegImageAddress), jpegImageLength);
    mjpegClient.print(F("\r\n"));
}

void handleWebClient() {
    WiFiClient client = webServer.available();
    if (!client) return;

    String request;
    bool firstLine = true;
    bool currentLineIsBlank = true;
    unsigned long deadline = millis() + 2000;

    // Follow the Ameba WiFiServer example: wait for the HTTP blank line
    // before sending the response, because browser headers may arrive in
    // multiple TCP packets.
    while (client.connected() && (long)(deadline - millis()) > 0) {
        if (!client.available()) {
            delay(1);
            continue;
        }
        char c = client.read();
        deadline = millis() + 2000;
        if (firstLine) {
            if (c == '\r') {
                firstLine = false;
            } else {
                request += c;
            }
        }
        if (c == '\n' && currentLineIsBlank) {
            break;
        }
        if (c == '\n') {
            currentLineIsBlank = true;
        } else if (c != '\r') {
            currentLineIsBlank = false;
        }
    }

    handleAction(request);
    printWebPage(client);
    // Flush and give the network stack time to transmit the complete HTML
    // response before closing the TCP connection.
    client.flush();
    delay(500);
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
    Camera.configVideoChannel(CHANNEL_JPEG, jpegConfig);
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
    Camera.channelBegin(CHANNEL_JPEG);

    videoStreamerRGB.registerInput(Camera.getStream(CHANNEL_NN));
    videoStreamerRGB.setStackSize();
    videoStreamerRGB.setTaskPriority();
    videoStreamerRGB.registerOutput(faceRecognition);
    videoStreamerRGB.begin();
    Camera.channelBegin(CHANNEL_NN);

    OSD.configVideo(CHANNEL, videoConfig);
    OSD.begin();
    webServer.begin();
    mjpegServer.begin();
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
    startMjpegClient();
    sendMjpegFrame();
    delay(10);
}
