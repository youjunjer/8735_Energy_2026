/*
  HUB-8735 Ultra - YOLO object detection
  Based on the built-in 8735_example/12_yolo example.
  Model: DEFAULT_YOLOV4TINY
*/

#include "WiFi.h"
#include "StreamIO.h"
#include "VideoStream.h"
#include "RTSP.h"
#include "NNObjectDetection.h"
#include "VideoStreamOverlay.h"
#include "ObjectClassList.h"
#include "U8g2lib.h"

#define CHANNEL   0
#define CHANNELNN 3

#define NNWIDTH  576
#define NNHEIGHT 320

VideoSetting config(VIDEO_FHD, 30, VIDEO_H264, 0);
VideoSetting configNN(NNWIDTH, NNHEIGHT, 10, VIDEO_RGB, 0);
NNObjectDetection ObjDet;
RTSP rtsp;
StreamIO videoStreamer(1, 1);
StreamIO videoStreamerNN(1, 1);

char ssid[] = "a";
char pass[] = "12345678";
int status = WL_IDLE_STATUS;

IPAddress ip;
int rtsp_portnum;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

void drawStatus(const char *line1, const char *line2 = nullptr,
                const char *line3 = nullptr)
{
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 16, line1);
    if (line2 != nullptr) {
        u8g2.drawStr(0, 34, line2);
    }
    if (line3 != nullptr) {
        u8g2.drawStr(0, 52, line3);
    }
    u8g2.sendBuffer();
}

void drawNetworkInfo()
{
    char ipText[24];
    char rtspText[24];
    snprintf(ipText, sizeof(ipText), "IP: %u.%u.%u.%u",
             ip[0], ip[1], ip[2], ip[3]);
    snprintf(rtspText, sizeof(rtspText), "RTSP: %d", rtsp_portnum);
    drawStatus("YOLO ready", ipText, rtspText);
}

void connectWiFi()
{
    while (status != WL_CONNECTED) {
        drawStatus("WiFi connecting...", "SSID: a");
        Serial.println("Attempting to connect to WPA SSID: a");
        status = WiFi.begin(ssid, pass);
        if (status != WL_CONNECTED) {
            drawStatus("WiFi retrying...", "SSID: a");
            delay(2000);
        }
    }

    ip = WiFi.localIP();
    char ipText[24];
    snprintf(ipText, sizeof(ipText), "IP: %u.%u.%u.%u",
             ip[0], ip[1], ip[2], ip[3]);
    drawStatus("WiFi connected", ipText, "Starting camera...");
}

void ODPostProcess(std::vector<ObjectDetectionResult> results);

void setup()
{
    Serial.begin(115200);

    u8g2.setI2CAddress(0x3C * 2);
    u8g2.begin();
    drawStatus("YOLO camera", "Starting...");

    connectWiFi();

    config.setBitrate(2 * 1024 * 1024);
    Camera.configVideoChannel(CHANNEL, config);
    Camera.configVideoChannel(CHANNELNN, configNN);
    Camera.videoInit();

    rtsp.configVideo(config);
    rtsp.begin();
    rtsp_portnum = rtsp.getPort();

    ObjDet.configVideo(configNN);
    ObjDet.setResultCallback(ODPostProcess);
    ObjDet.modelSelect(OBJECT_DETECTION, DEFAULT_YOLOV4TINY, NA_MODEL, NA_MODEL);
    ObjDet.begin();

    videoStreamer.registerInput(Camera.getStream(CHANNEL));
    videoStreamer.registerOutput(rtsp);
    if (videoStreamer.begin() != 0) {
        Serial.println("StreamIO RTSP link start failed");
    }
    Camera.channelBegin(CHANNEL);

    videoStreamerNN.registerInput(Camera.getStream(CHANNELNN));
    videoStreamerNN.setStackSize();
    videoStreamerNN.setTaskPriority();
    videoStreamerNN.registerOutput(ObjDet);
    if (videoStreamerNN.begin() != 0) {
        Serial.println("StreamIO NN link start failed");
    }
    Camera.channelBegin(CHANNELNN);

    OSD.configVideo(CHANNEL, config);
    OSD.begin();

    drawNetworkInfo();

    Serial.print("RTSP stream: rtsp://");
    Serial.print(ip);
    Serial.print(":");
    Serial.println(rtsp_portnum);
    Serial.println("YOLO object detection ready");
}

void loop()
{
}

void ODPostProcess(std::vector<ObjectDetectionResult> results)
{
    uint16_t im_h = config.height();
    uint16_t im_w = config.width();

    printf("Total number of objects detected = %d\r\n", ObjDet.getResultCount());
    OSD.createBitmap(CHANNEL);

    if (ObjDet.getResultCount() > 0) {
        for (int i = 0; i < ObjDet.getResultCount(); i++) {
            int obj_type = results[i].type();
            if (itemList[obj_type].filter) {
                ObjectDetectionResult item = results[i];
                int xmin = (int)(item.xMin() * im_w);
                int xmax = (int)(item.xMax() * im_w);
                int ymin = (int)(item.yMin() * im_h);
                int ymax = (int)(item.yMax() * im_h);

                printf("Item %d %s:\t%d %d %d %d\n\r", i,
                       itemList[obj_type].objectName, xmin, xmax, ymin, ymax);
                OSD.drawRect(CHANNEL, xmin, ymin, xmax, ymax, 3, OSD_COLOR_WHITE);

                char text_str[20];
                snprintf(text_str, sizeof(text_str), "%s %d",
                         itemList[obj_type].objectName, item.score());
                OSD.drawText(CHANNEL, xmin,
                             ymin - OSD.getTextHeight(CHANNEL),
                             text_str, OSD_COLOR_CYAN);
            }
        }
    }
    OSD.update(CHANNEL);
}
