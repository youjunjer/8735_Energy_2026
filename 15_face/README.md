# 15_face

HUB-8735 Ultra RTSP face recognition with a Wi-Fi web control page.

- RTSP video: H.264, with OSD face boxes and names
- Face detection: SCRFD
- Face recognition: MobileFaceNet
- Web control: `http://<board-ip>/`
- Web port: `80`
- MJPEG preview: `http://<board-ip>:8080/stream`

The web buttons replace the serial commands for registering, deleting, resetting, backing up and restoring registered faces.

The control page embeds the 640×360 MJPEG preview. The HD H.264 RTSP stream remains available separately for VLC or another RTSP player.
