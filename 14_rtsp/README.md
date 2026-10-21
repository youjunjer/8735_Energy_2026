# 14_rtsp

HUB-8735 Ultra RTSP camera streamer based on the AmebaPro2 `StreamRTSP/SingleVideoWithAudio` example.

- Wi-Fi SSID: `a`
- Video: HD 1280×720 H.264
- Audio: 16 kHz mono PDM microphone encoded as AAC
- OLED: Wi-Fi connection status, board IP and RTSP port

After startup, connect with the URL shown on the OLED or serial monitor:

```text
rtsp://<board-ip>:<rtsp-port>
```
