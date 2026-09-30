/*
  HUB-8735 Ultra + passive buzzer

  Passive buzzer signal -> IO11 (PWM)
  Buzzer GND            -> board GND

  Alternate a rising and falling tone sweep to imitate a fire-truck siren.
*/

const uint8_t BUZZER_PIN = 11;
const int SIREN_LOW_HZ = 650;
const int SIREN_HIGH_HZ = 1250;
const int SIREN_STEP_HZ = 10;
const int STEP_DELAY_MS = 10;

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  // Sweep upward in pitch.
  for (int frequency = SIREN_LOW_HZ;
       frequency <= SIREN_HIGH_HZ;
       frequency += SIREN_STEP_HZ) {
    tone(BUZZER_PIN, frequency);
    delay(STEP_DELAY_MS);
  }

  // Sweep downward in pitch, then repeat continuously.
  for (int frequency = SIREN_HIGH_HZ - SIREN_STEP_HZ;
       frequency >= SIREN_LOW_HZ;
       frequency -= SIREN_STEP_HZ) {
    tone(BUZZER_PIN, frequency);
    delay(STEP_DELAY_MS);
  }
}
