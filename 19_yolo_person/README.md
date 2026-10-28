# 19_yolo_person

以 `18_yolo` 為基礎，使用 HUB-8735 Ultra 的 YOLOv4-tiny 只辨識並標示 `person`。

- 只標示 person 類別，忽略其他物件
- 人之外框：紅色，線寬 5
- 左上角顯示目前人數，例如 `Person=5`
- 保留 FHD H.264 RTSP 串流
- 保留 OLED 的 Wi-Fi 連線狀態、IP 與 RTSP 埠號顯示
- Wi-Fi：SSID `a`，密碼 `12345678`
