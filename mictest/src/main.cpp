/*
 * Improved ESP32-S3 Mic Test
 * - Lights LED when loud sound is detected
 * - Counts sound events only once per sound
 */

#include <I2S.h>

#define LED_PIN 21
#define SOUND_THRESHOLD 4500
#define EVENT_COOLDOWN 100  // ms between events

unsigned long lastEventTime = 0;
int soundCount = 0;

bool soundPreviouslyHigh = false;   // prevents repeated prints

void setup() {
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  I2S.setAllPins(-1, 42, 41, -1, -1);

  if (!I2S.begin(PDM_MONO_MODE, 16000, 16)) {
    Serial.println("Failed to initialize I2S!");
    while (1);
  }
}

void loop() {
  int sample = I2S.read();

  if (sample && sample != -1 && sample != 1) {
    int magnitude = abs(sample);

    bool soundIsHigh = (magnitude > SOUND_THRESHOLD);

    if (soundIsHigh) {
      digitalWrite(LED_BUILTIN, LOW);

      // Only count and print when FIRST crossing the threshold
      if (!soundPreviouslyHigh && millis() - lastEventTime > EVENT_COOLDOWN) {
        soundCount++;
        lastEventTime = millis();
        Serial.print("Sound detected! Count = ");
        Serial.println(soundCount);
        Serial.println(magnitude);
      }

    } else {
      digitalWrite(LED_BUILTIN, HIGH);
    }

    // Update previous state
    soundPreviouslyHigh = soundIsHigh;
  }
}
