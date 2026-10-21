# 17_face_sg90

Combines `15_face` face recognition and `16_sg90` servo control.

- SG90 signal: IO21
- A recognized registered face triggers 90° for 5 seconds, then returns to 0°.
- Unknown-only faces do not move the servo.
- The same continuously visible known face triggers only once until it leaves the frame.
- RTSP HD H.264, HTTP control page on port 80, and 480×270 MJPEG preview on port 8080 are retained.
