# 18_yolo

HUB-8735 Ultra YOLO 物件辨識範例，參考內建 `8735_example/12_yolo`。

- YOLO 模型：`DEFAULT_YOLOV4TINY`
- 主影像：FHD H.264，透過 RTSP 輸出
- NN 影像：576×320 RGB
- OLED：顯示啟動狀態、Wi-Fi 連線進度、取得後的 IP 與 RTSP 埠號
- Wi-Fi SSID：`a`
- Wi-Fi 密碼：`12345678`
- OLED 位址：`0x3C`

燒錄後，RTSP 位址會顯示在 OLED 第三行，例如：

`rtsp://<IP>:<port>`
