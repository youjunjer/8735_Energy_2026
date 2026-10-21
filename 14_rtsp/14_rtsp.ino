/*
  HUB-8735 Ultra RTSP camera streamer

  Video: HD 1280x720, H.264
  Audio: 16 kHz mono digital PDM microphone, AAC
  RTSP: rtsp://<board-ip>:<port>
  OLED: connection status, IP address and RTSP URL

  Based on the AmebaPro2 Multimedia/StreamRTSP/SingleVideoWithAudio example.
*/

#include "WiFi.h"
#include "StreamIO.h"
#include "VideoStream.h"
#include "AudioStream.h"
#include "AudioEncoder.h"
#include "RTSP.h"
#include "U8g2lib.h"

#define VIDEO_CHANNEL 0

VideoSetting videoConfig(VIDEO_HD, CAM_FPS, VIDEO_H264, 0);
AudioSetting audioConfig(3);  // 16 kHz mono digital PDM microphone
Audio audio;
AAC aac;
RTSP rtsp;
StreamIO audioStreamer(1, 1);  // Audio -> AAC
StreamIO avMixStreamer(2, 1);  // Video + AAC -> RTSP

char ssid[] = "a";
char pass[] = "12345678";
int wifiStatus = WL_IDLE_STATUS;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

void drawStatus(const char *line1, const char *line2 = nullptr,
                const char *line3 = nullptr) {
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

void drawIpAndRtsp() {
    char ipText[24];
    char rtspText[32];
    IPAddress ip = WiFi.localIP();
    snprintf(ipText, sizeof(ipText), "IP: %u.%u.%u.%u",
             ip[0], ip[1], ip[2], ip[3]);
    snprintf(rtspText, sizeof(rtspText), "RTSP: %d", rtsp.getPort());
    drawStatus("WiFi connected", ipText, rtspText);
}

void connectWiFi() {
    while (wifiStatus != WL_CONNECTED) {
        drawStatus("WiFi connecting...", "SSID: a");
        Serial.println("Attempting to connect to WPA SSID: a");
        wifiStatus = WiFi.begin(ssid, pass);
        if (wifiStatus != WL_CONNECTED) {
            drawStatus("WiFi retrying...", "SSID: a");
            delay(2000);
        }
    }
}

void setup() {
    Serial.begin(115200);

    u8g2.setI2CAddress(0x3C * 2);
    u8g2.begin();
    drawStatus("RTSP camera", "Starting...");

    connectWiFi();
    drawStatus("WiFi connected", "Starting camera...");

    // Configure HD 1280x720 H.264 camera stream.
    Camera.configVideoChannel(VIDEO_CHANNEL, videoConfig);
    Camera.videoInit();

    // Configure 16 kHz mono PDM microphone and AAC audio encoding.
    audio.configAudio(audioConfig);
    audio.begin();
    aac.configAudio(audioConfig);
    aac.begin();

    // Configure RTSP with H.264 video and AAC audio.
    rtsp.configVideo(videoConfig);
    rtsp.configAudio(audioConfig, CODEC_AAC);
    rtsp.begin();

    audioStreamer.registerInput(audio);
    audioStreamer.registerOutput(aac);
    if (audioStreamer.begin() != 0) {
        Serial.println("Audio StreamIO link start failed");
        drawStatus("Audio stream error");
    }

    avMixStreamer.registerInput1(Camera.getStream(VIDEO_CHANNEL));
    avMixStreamer.registerInput2(aac);
    avMixStreamer.registerOutput(rtsp);
    if (avMixStreamer.begin() != 0) {
        Serial.println("A/V StreamIO link start failed");
        drawStatus("A/V stream error");
    }

    Camera.channelBegin(VIDEO_CHANNEL);
    delay(1000);
    drawIpAndRtsp();

    Serial.println("------------------------------");
    Serial.println("- HD H.264 + AAC RTSP stream -");
    Serial.println("------------------------------");
    Camera.printInfo();
    Serial.print("rtsp://");
    Serial.print(WiFi.localIP());
    Serial.print(":");
    rtsp.printInfo();
    Serial.println("- Audio Information -");
    audio.printInfo();
}

void loop() {
    // Multimedia pipelines run in the background.
    delay(1000);
}
