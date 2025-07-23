#include <Adafruit_Fingerprint.h>
HardwareSerial fpSerial(2);
Adafruit_Fingerprint finger(&fpSerial);

void setup() {
  Serial.begin(115200);
  fpSerial.begin(57600, SERIAL_8N1, 14, 15);
  finger.begin(57600);
  if (!finger.verifyPassword()) {
    Serial.println("Capteur non détecté ⚠");
    while (true) delay(10);
  }
}

void loop() {
   if (finger.getParameters() == FINGERPRINT_OK) {
    Serial.print("\nCapacité maximale : ");
    Serial.println(finger.capacity);
    Serial.print("Déjà enrôlés      : ");
    Serial.println(finger.templateCount);
  } else {
    Serial.println("Impossible de lire les paramètres.");
  }

  delay(500);
}
