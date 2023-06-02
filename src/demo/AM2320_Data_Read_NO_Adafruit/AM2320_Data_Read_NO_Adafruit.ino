#include <AM2320_asukiaaa.h>

AM2320_asukiaaa am2320;

void setup() {
  Serial.begin(115200);
  while(!Serial); //waiting for serial start

  Serial.println("started");
  Wire.begin();
  am2320.setWire(&Wire);
}

void loop() {
  if (am2320.update() != 0) {
    Serial.println("Error: Cannot update sensor values.");
  } else {
    Serial.println("temperatureC: " + String(am2320.temperatureC) + " C");
    Serial.println("temperatureF: " + String(am2320.temperatureF) + " F");
    Serial.println("humidity: " + String(am2320.humidity) + " %");
  }
  Serial.println("");
  delay(2000);
}