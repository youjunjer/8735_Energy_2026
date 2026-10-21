# 8735 Ultra 專案交接紀錄

更新日期：2026-10-07（Asia/Taipei）

## 專案位置

- 工作目錄：`D:\115智慧節能`
- GitHub：`https://github.com/youjunjer/8735_Energy_2026.git`
- 遠端名稱：`origin`
- 主要分支：`main`
- 程式基準 commit：`a0ee3ed`（新增 `09_oled_dht_led_beep`）；最新 AQI 程式尚待提交
- 交接文件提交後的最新 commit：請以 `git log -1 --oneline` 確認

## Arduino CLI 重要路徑

Arduino CLI 不要只依賴 PATH，請使用絕對路徑：

```powershell
& 'C:\Program Files\Arduino CLI\arduino-cli.exe'
```

目前使用的 Arduino 核心與板卡：

- FQBN：`ideasHatch:AmebaPro2:Ameba_HUB-8735_ultra`
- 核心版本：`ideasHatch:AmebaPro2 4.0.15-Release`
- 核心位置：`C:\Users\youju\AppData\Local\Arduino15\packages\ideasHatch\hardware\AmebaPro2\4.0.15-Release`
- U8g2 使用者函式庫：`C:\Users\youju\OneDrive\Documents\Arduino\libraries\U8g2\src`
- 工作區域 Arduino 快取通常位於：`C:\Users\youju\AppData\Local\arduino\sketches`

## 編譯與燒錄指令

Windows PowerShell 建議先設定 locale，避免 Arduino CLI 出現：
`locale::facet::_S_create_c_locale name not valid`

```powershell
$env:LC_ALL='C'
$env:LANG='C'
$env:LANGUAGE='C'

& 'C:\Program Files\Arduino CLI\arduino-cli.exe' compile `
  --fqbn ideasHatch:AmebaPro2:Ameba_HUB-8735_ultra `
  '09_oled_dht_led_beep'

& 'C:\Program Files\Arduino CLI\arduino-cli.exe' board list --format json

& 'C:\Program Files\Arduino CLI\arduino-cli.exe' upload `
  --fqbn ideasHatch:AmebaPro2:Ameba_HUB-8735_ultra `
  --port COM3 `
  '09_oled_dht_led_beep'
```

判定燒錄成功必須看到：

```text
upload success
```

板子最近偵測到的序列埠是 `COM3`，VID/PID 為 `0x1A86/0x7523`。若 COM3 改變，先重新執行 `board list --format json`，再替換 `--port`。

## 硬體接線

- HUB-8735 Ultra：`ideasHatch:AmebaPro2:Ameba_HUB-8735_ultra`
- DHT11 DATA：IO20
- OLED：SSD1306 128x64，I2C 位址 `0x3C`，U8g2 硬體 I2C
- OLED 程式不要呼叫 `Wire.begin()`；由 U8g2 初始化硬體 I2C
- 綠色 LED：IO24
- 黃色 LED：IO23
- 紅色 LED：IO22
- 無源蜂鳴器目前使用：IO11，GND 共地

## 重要腳位陷阱

目前安裝的 `ameba_hub8735_ultra` 核心腳位映射中：

- IO11（AMB_D11 / PF14）標示支援 `PIO_PWM`，可使用 `tone()`。
- IO18（AMB_D18 / PF5）在核心中只標示 GPIO／SPI，沒有 `PIO_PWM`。
- 因此原先把無源蜂鳴器接 IO18 並呼叫 `tone()` 時，程式雖可編譯及燒錄，但執行時會卡在 PWM 腳位檢查，沒有聲音。
- 蜂鳴器訊號線必須接到 IO11，不能只修改程式而保留 IO18 接線。

核心相關檔案：

- 腳位映射：`C:\Users\youju\AppData\Local\Arduino15\packages\ideasHatch\hardware\AmebaPro2\4.0.15-Release\variants\ameba_hub8735_ultra\variant.cpp`
- `tone()` 包裝：`C:\Users\youju\AppData\Local\Arduino15\packages\ideasHatch\hardware\AmebaPro2\4.0.15-Release\cores\ambpro2\Tone.cpp`
- PWM 檢查與 tone 實作：`C:\Users\youju\AppData\Local\Arduino15\packages\ideasHatch\hardware\AmebaPro2\4.0.15-Release\cores\ambpro2\wiring_analog.c`
- 腳位功能檢查：`C:\Users\youju\AppData\Local\Arduino15\packages\ideasHatch\hardware\AmebaPro2\4.0.15-Release\cores\ambpro2\amb_ard_pin_check.c`

## 程式版本

- `01_led`：IO25 綠燈閃爍。
- `02_RGled`：IO24 綠、IO23 黃、IO22 紅，紅綠燈循環。
- `03_OLED`：U8g2 OLED 基本測試。
- `04_oled_light`：光敏電阻 IO0，顯示亮度比例。
- `05_OLED_DHT11`：DHT11 + OLED 基本顯示。
- `06_oled_dht_icon`：左右溫濕度圖示，溫度計及水滴會隨數值填充。
- `07_oled_dht_led`：溫度 >25°C 紅燈、濕度 >80% 黃燈，皆正常亮綠燈。
- `08_beep`：IO11 無源蜂鳴器消防車升降警笛。
- `09_oled_dht_led_beep`：07 加上 IO11 警報；溫度或濕度任一異常時播放警笛，皆正常時停止。
- `10_wifi_aqi_oled`：使用 Wi-Fi 與 ArduinoJson 取得環境部 AQI API，篩選桃園市中壢測站，每分鐘更新 OLED 上的 AQI 與 PM2.5；連線與更新狀態會顯示於 OLED。
- `11_wifi_oled_aqi_dht`：整合 DHT11、三色 LED、IO11 蜂鳴器與 Wi-Fi AQI；OLED 每 5 秒輪播溫溼度及中壢 AQI／PM2.5。
- `12_thingspeak`：保留 Wi-Fi／AQI／DHT11 顯示功能，並將溫度、濕度、AQI、PM2.5 上傳 ThingSpeak field1～field4，每 20 秒更新。
- `13_mqtt`：使用 PubSubClient 連線 `mqttgo.io:1883`，每 15 秒將 DHT11 資料以 JSON 發布至 `eric/class105/data`；OLED 顯示連線與發布狀態。
- `14_rtsp`：依 HUB-8735 Ultra `StreamRTSP/SingleVideoWithAudio` 範例，使用 Wi-Fi 傳輸 HD 1280×720 H.264 影像與 AAC 聲音；OLED 顯示連線過程、板端 IP 與 RTSP 連接埠。
- `15_face`：以 Wi-Fi 網頁介面取代人臉辨識序列埠指令；網頁按鈕可登錄、刪除、清除、備份與還原人臉資料，並保留 RTSP、OSD 與 OLED 顯示。
- `15_face`：網頁控制頁使用 port 80，新增 port 8080 的 640×360 MJPEG 即時預覽；HD H.264 RTSP 串流維持不變。
- `15_face`：目前網頁 MJPEG 預覽已調整為 480×270、約 8 FPS；HD H.264 RTSP 串流維持不變。
- `16_sg90`：使用 `AmebaServo` 控制接在 IO21 的 SG90，依序 0°→90°→180°→0°，各角度 1 秒，回到 0° 後停 5 秒。
- `17_face_sg90`：整合 `15_face` 與 SG90；有已註冊人臉時 IO21 轉 90°、停 5 秒後回 0°，只有 unknown 時不動作。
- `17_face_sg90`：已移除不穩定的 MJPEG 預覽與 port 8080，只保留 HD H.264 RTSP 與 port 80 網頁控制。
- `17_face_sg90`：新增辨識燈號，已知人臉綠燈 IO24、只有 unknown 紅燈 IO22、無人臉黃燈 IO23。

目前最新使用程式是：
`D:\115智慧節能\15_face\15_face.ino`

RTSP 測試程式為：
`D:\115智慧節能\14_rtsp\14_rtsp.ino`

目前狀態：
- `13_mqtt` 已改用 `wifiClient.setNonBlockingMode()`，MQTT 重連不會阻塞主迴圈。
- 光敏電阻接在 IO0，亮度以 0–100% 表示（0 最暗、100 最亮），並以 `light` 欄位發布。
- 已使用 Wi-Fi `a`、MQTT Broker `mqttgo.io:1883`、Topic `eric/class105/data` 測試，OLED 連線流程正常。
- 已於 2026-10-14 編譯並成功燒錄至 HUB-8735 Ultra 的 COM4；目前功能確認正常，後續可直接沿用此版本。
- `14_rtsp` 已於 2026-10-21 編譯成功並燒錄至 COM4，Arduino CLI 回報 `upload success`。
- `14_rtsp` 使用 `VIDEO_HD`（1280×720）H.264、16 kHz AAC 聲音與 RTSP；OLED 顯示 Wi-Fi 連線過程、IP 及 RTSP 連接埠。

## 2026-10-21 對話交接摘要

- 使用者要求將本對話內容寫入交接紀錄，供後續 AI 接手。
- 目前主要程式為 `13_mqtt/13_mqtt.ino`，板型為 `ideasHatch:AmebaPro2:Ameba_HUB-8735_ultra`，最近使用連接埠為 COM4。
- Wi-Fi：SSID `a`；MQTT Broker：`mqttgo.io:1883`；Topic：`eric/class105/data`；每 15 秒發布一筆 DHT11 與光敏資料。
- DHT11 接 IO20；光敏電阻接 IO0。光敏 ADC 依目前硬體接法換算為 0–100%：0 為最暗、100 為最亮。
- MQTT payload 格式：`{"temp":25,"humi":65,"light":92}`。OLED 資料頁顯示溫度、濕度及亮度，連線與發布狀態也會顯示。
- MQTT Client ID 每次開機產生隨機六位數字；已啟用 `wifiClient.setNonBlockingMode()`，避免 Broker 連線問題阻塞主迴圈。
- 先前曾遇到 `MQTT connect fail`，診斷狀態為 `-4`；加入 DNS、狀態碼、keep-alive、TCP 清理及非阻塞 socket 後，使用者確認目前已無問題。
- 最近一次已完成編譯及 COM4 燒錄，版本 `v0.15.0`；最近一次程式提交為 `7affa2c`。
- 後續若修改程式，請同步更新 `CHANGELOG.md`、增加版本號、更新本交接紀錄，並依使用者既有要求 commit、push 至 `origin main`。不要將 `成果照片` 內未追蹤照片加入提交。

## 版本紀錄

- v0.10.0：新增 `09_oled_dht_led_beep`，已編譯並於 COM3 成功燒錄。
- v0.12.0：新增 `11_wifi_oled_aqi_dht`，整合 DHT11、三色 LED、IO11 蜂鳴器與 Wi-Fi AQI，已編譯並於 COM3 成功燒錄。
- v0.13.0：新增 `12_thingspeak`，每 20 秒將溫度、濕度、AQI、PM2.5 上傳 ThingSpeak，已編譯並於 COM3 成功燒錄。
- v0.14.0：新增 `13_mqtt`，每 15 秒以 MQTT 發布 DHT11 溫溼度 JSON；已編譯成功，尚未燒錄。
- v0.14.1：`13_mqtt` 改用每次開機產生的隨機 Client ID，已編譯並於 COM4 成功燒錄。
- v0.14.2：`13_mqtt` 加入 MQTT DNS／狀態碼診斷、keep-alive 及重連前 TCP 清理，已編譯並於 COM4 成功燒錄；目前需依 OLED 顯示的診斷訊息確認板端連線原因。
- v0.14.3：`13_mqtt` 依照 Ameba `MQTT_Basic`／非阻塞重連模式加入 `wifiClient.setNonBlockingMode()`，已編譯並於 COM4 成功燒錄。
- v0.15.0：`13_mqtt` 新增 IO0 光敏電阻亮度 0–100% 換算，MQTT payload 加入 `light` 欄位，OLED 顯示亮度；已編譯並於 COM4 成功燒錄。
- v0.16.0：新增 `14_rtsp`，提供 HD H.264 + AAC 音訊 RTSP 串流，OLED 顯示連線狀態、IP 與 RTSP 連接埠；待硬體驗證。
- v0.17.0：新增 `15_face`，以 port 80 Wi-Fi 網頁按鈕取代人臉辨識序列埠指令；已編譯成功並燒錄至 COM4。
- v0.18.0：`15_face` 新增 port 8080 的 MJPEG 網頁預覽，控制頁可邊看影像邊操作；已編譯成功並燒錄至 COM4，Arduino CLI 回報 `upload success`。
- v0.18.1：`15_face` 網頁 MJPEG 預覽改為 480×270、約 8 FPS，以改善網頁反應速度；已編譯成功並燒錄至 COM4，Arduino CLI 回報 `upload success`。
- v0.19.0：新增 `16_sg90` SG90 伺服馬達測試程式，使用 IO21；已編譯成功並燒錄至 COM4，Arduino CLI 回報 `upload success`。
- v0.20.0：新增 `17_face_sg90`，整合人臉辨識、RTSP／MJPEG／網頁控制與 IO21 SG90 觸發；已編譯成功並燒錄至 COM4，Arduino CLI 回報 `upload success`。
- v0.20.1：`17_face_sg90` 移除 MJPEG 預覽功能，保留 RTSP、網頁控制、人臉辨識與 SG90；已編譯成功並燒錄至 COM4，Arduino CLI 回報 `upload success`。
- v0.20.2：`17_face_sg90` 新增綠／紅／黃三色 LED 辨識狀態控制；已編譯成功並燒錄至 COM4，Arduino CLI 回報 `upload success`。
- v0.20.3：燈號判定改用人臉辨識回呼的 `results.size()`，避免函式庫內部計數同步時序導致辨識結果未反映到燈號；已編譯成功並燒錄至 COM4，Arduino CLI 回報 `upload success`。
- v0.11.1：修正 `10_wifi_aqi_oled` 的 HTTP chunked 回應解析，已編譯並於 COM3 成功燒錄。使用 Wi-Fi SSID `a`，每 60 秒抓取 API 並顯示桃園市中壢 AQI／PM2.5。
- v0.11.0：新增 `10_wifi_aqi_oled`，已編譯成功；首次版本尚未燒錄。
- v0.9.1：蜂鳴器由 IO18 改到 IO11，已編譯並於 COM3 成功燒錄。
- v0.9.0：新增 `08_beep`；原 IO18 版本實測無聲。
- v0.8.0：新增 `07_oled_dht_led`。
- v0.7.0：水滴全高並依濕度 0–100% 填充。

完整版本紀錄請讀取：`D:\115智慧節能\CHANGELOG.md`

## Git 注意事項

使用者要求後續整個專案同步 GitHub，不是只同步單一程式；每次程式變更都要更新 `CHANGELOG.md`、建立版號、commit 並 push 到 `origin main`。

目前工作目錄有三張未追蹤成果照片，先前刻意沒有納入程式提交：

- `成果照片/WIN_20260930_15_53_59_Pro.jpg`
- `成果照片/WIN_20260930_16_14_11_Pro.jpg`
- `成果照片/WIN_20260930_16_46_04_Pro.jpg`

提交前先執行：

```powershell
git status --short
git diff --check
```

不要在未確認前刪除或加入上述照片。
