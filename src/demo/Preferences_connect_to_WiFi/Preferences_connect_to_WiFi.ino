#include <Preferences.h>
#include <WiFi.h>

Preferences preferences;

String ssid;
String password;

void setup() {
  Serial.begin(115200);
  Serial.println();
  
  preferences.begin("credentials", true);
 
  ssid = preferences.getString("SSID", ""); 
  password = preferences.getString("PASSWORD", "");

  Serial.printf("\nSSID: %s\n", ssid.c_str());
  Serial.printf("PASSWORD: %s\n", password.c_str());


  if (ssid == "" || password == ""){
    Serial.println("No values saved for ssid or password");
  }
  else {
    // Connect to Wi-Fi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.println("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print('.');
      delay(1000);
    }
    Serial.println();
    Serial.println(WiFi.localIP());  
  }
}

void loop() {
  // put your main code here, to run repeatedly:
}