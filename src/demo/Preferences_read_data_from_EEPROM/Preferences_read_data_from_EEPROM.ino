//https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/
#include <Preferences.h>
#include <WiFi.h>

Preferences preferences;

String ssid;
String password;

void setup() {
  Serial.begin(115200);
  Serial.println();
  
  preferences.begin("credentials", false);
 
  ssid = preferences.getString("SSID", ""); 
  password = preferences.getString("PASSWORD", "");

  if (ssid == "" || password == ""){
    Serial.println("No values saved for ssid or password");
  }
  else {
    Serial.printf("ssid: %s\n", ssid);
    Serial.printf("password: %s\n", password);
  }
  preferences.end();
}

void loop() {
  // put your main code here, to run repeatedly:
}