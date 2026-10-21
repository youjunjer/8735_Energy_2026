/*
  HUB-8735 Ultra + SG90 servo

  Signal: IO21
  Sequence: 0 -> 90 -> 180 -> 0 degrees
  Each position is held for 1 second. After returning to 0 degrees,
  wait 5 seconds before starting the sequence again.

  Based on the HUB-8735 Ultra Analog/PWM_ServoControl example.
*/

#include <AmebaServo.h>

const uint8_t SERVO_PIN = 21;
AmebaServo servo;

void moveAndHold(int angle, unsigned long holdMs) {
    servo.write(angle);
    delay(holdMs);
}

void setup() {
    servo.attach(SERVO_PIN);
    moveAndHold(0, 1000);
}

void loop() {
    moveAndHold(90, 1000);
    moveAndHold(180, 1000);
    moveAndHold(0, 5000);
}
