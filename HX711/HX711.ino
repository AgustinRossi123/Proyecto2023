#include "HX711.h"

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 0;
const int LOADCELL_SCK_PIN = 13;

HX711 scale;

void setup() {
  Serial.begin(115200);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
}

void loop() {

  if (scale.is_ready()) {
    long reading = scale.read();
    float peso = scale.read();
    float pesoreal = (peso - 92600)/100;
    Serial.print(pesoreal);
    Serial.println(" gramos");
  } else {
    Serial.println("HX711 not found.");
  }

  delay(1000);
  
}
