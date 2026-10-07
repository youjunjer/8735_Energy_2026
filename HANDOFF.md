# 8735 Ultra 專案交接紀錄

更新日期：2026-10-07（Asia/Taipei）

## 專案位置

- 工作目錄：`D:\115智慧節能`
- GitHub：`https://github.com/youjunjer/8735_Energy_2026.git`
- 遠端名稱：`origin`
- 主要分支：`main`
- 目前最新 commit：`a0ee3ed`（新增 `09_oled_dht_led_beep`）

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

目前最新使用程式是：
`D:\115智慧節能\09_oled_dht_led_beep\09_oled_dht_led_beep.ino`

## 版本紀錄

- v0.10.0：新增 `09_oled_dht_led_beep`，已編譯並於 COM3 成功燒錄。
- v0.9.1：蜂鳴器由 IO18 改到 IO11，已編譯並於 COM3 成功燒錄。
- v0.9.0：新增 `08_beep`；原 IO18 版本實測無聲。
- v0.8.0：新增 `07_oled_dht_led`。
- v0.7.0：水滴全高並依濕度 0–100% 填充。

完整版本紀錄請讀取：`D:\115智慧節能\CHANGELOG.md`

## Git 注意事項

使用者要求後續整個專案同步 GitHub，不是只同步單一程式；每次程式變更都要更新 `CHANGELOG.md`、建立版號、commit 並 push 到 `origin main`。

目前工作目錄有兩張未追蹤成果照片，先前刻意沒有納入程式提交：

- `成果照片/WIN_20260930_15_53_59_Pro.jpg`
- `成果照片/WIN_20260930_16_14_11_Pro.jpg`

提交前先執行：

```powershell
git status --short
git diff --check
```

不要在未確認前刪除或加入上述照片。
