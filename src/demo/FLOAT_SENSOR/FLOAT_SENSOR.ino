
#define floatSensor 17


void setup(){
  Serial.begin(115200);
  pinMode(floatSensor, INPUT_PULLUP);
}

void loop(){
  Serial.println(digitalRead(floatSensor));
  delay(10);
}