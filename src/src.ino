#include <WiFi.h>
#include <Preferences.h>
#include <arduino-timer.h>
#include <ArduinoHA.h>
#include <Bounce2.h>
#include <AM2320_asukiaaa.h>

/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------I/O Definitions--------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
const byte          LIGHT = 32;
const byte          WATER_PUMP = 33;
const byte          TDS = 35;
const byte          PH = 34;
const byte          AIR_PUMP = 19;
const byte          FLOAT_SENSOR = 17;
const byte          FAN = 16;
const byte          GENERAL_BUTTON = 5;

const byte          ONBOARD_LED = 2;

/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------TIME definitions-------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
#define PERIOD_SECOND_1         1000
#define PERIOD_SECOND_5         5000
#define PERIOD_SECOND_15        15000
#define PERIOD_MINUTE_1         60000
#define PERIOD_MINUTE_5         300000
#define PERIOD_MINUTE_15        900000

auto timer = timer_create_default();

#define DEBOUNCE_DELAY          50


/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------Public variables-------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
byte                  mac[6];

WiFiClient            wifiClient;
Preferences           preferences;

AM2320_asukiaaa       am2320;
float                 temperatureOffset = 2;
float                 temperature;
float                 humidity;

Bounce generalButton = Bounce();
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------I/O States-------------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
bool                  generalButtonState = false;
bool                  lightState = false;
bool                  waterPumpState = false;
bool                  airPumpState = false;
bool                  floatSensorState = false;
bool                  fanState = false;


String                WIFI_SSID;
String                WIFI_PASSWORD;
String                MQTT_SERVER;
String                MQTT_USER;
String                MQTT_PASSWORD;
  

/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------HA DEVICE--------------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/

HADevice          device("esp32serra");

HAMqtt            mqtt(wifiClient, device, 12); //always before devices type see documentation

HABinarySensor    bsGeneralButton("generalButton");
HABinarySensor    bsFloatSensor("floatSensor");
HASwitch          swWaterPump("waterPump");
HASwitch          swAirPump("airPump");
HASwitch          swFan("fan");
HALight           lLight("light");
HASensor          sTemperature("temperature");
HASensor          sHumidity("humidity");







/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------Interrupts-------------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
// when float sensor fall
void IRAM_ATTR floatSensorISR(){
  waterPumpState = false;
  floatSensorState = false;
}

/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------SETUP------------------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
void setup() {
  Serial.begin(115200);

  preferences.begin("credentials", true);

  WIFI_SSID = preferences.getString("SSID", ""); 
  WIFI_PASSWORD = preferences.getString("PASSWORD", "");
  MQTT_SERVER = preferences.getString("MQTT_SERVER", "");
  MQTT_USER = preferences.getString("MQTT_USER", ""); 
  MQTT_PASSWORD = preferences.getString("MQTT_PASSWORD", "");

  preferences.end();
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------GPIO DEFINITION--------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
  pinMode(ONBOARD_LED, OUTPUT);

  pinMode(LIGHT, OUTPUT);
  pinMode(WATER_PUMP, OUTPUT);
  pinMode(AIR_PUMP, OUTPUT);
  pinMode(FAN, OUTPUT);

  pinMode(TDS, INPUT);
  pinMode(PH, INPUT);

  //pinMode(GENERAL_BUTTON, INPUT_PULLDOWN);
  generalButton.attach(GENERAL_BUTTON, INPUT_PULLDOWN);
  generalButton.interval(DEBOUNCE_DELAY);


  pinMode(FLOAT_SENSOR, INPUT_PULLUP);
  attachInterrupt(FLOAT_SENSOR, floatSensorISR, FALLING);
  
  Wire.begin();
  am2320.setWire(&Wire);



  setupWiFi();

/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------MQTT DEFINITION--------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/


  //generalButtonState = digitalRead(GENERAL_BUTTON);

  mqtt.setDiscoveryPrefix("homeassistant");
  mqtt.setDataPrefix("data");
  mqtt.onMessage(onMessage);
  mqtt.onConnected(onConnected);
  

  // optional device's details
  device.setName("ESP32Serra");
  device.setSoftwareVersion("1.2");
  device.setManufacturer("Speed224");
  device.enableSharedAvailability();
  device.enableLastWill();


  bsGeneralButton.setCurrentState(generalButtonState);
  bsGeneralButton.setName("General Button");
  bsGeneralButton.setIcon("mdi:fire");

  bsFloatSensor.setCurrentState(floatSensorState);
  bsFloatSensor.setName("Float Sensor");
  bsFloatSensor.setIcon("mdi:water-check");

  swWaterPump.setCurrentState(waterPumpState);
  swWaterPump.setName("Water Pump");
  swWaterPump.setIcon("mdi:water-pump");

  swAirPump.setCurrentState(airPumpState);
  swAirPump.setName("Air Pump");
  swAirPump.setIcon("mdi:pump");

  swFan.setCurrentState(fanState);
  swFan.setName("Fan");
  swFan.setIcon("mdi:fan");

  lLight.setCurrentState(lightState);
  lLight.setName("Light");
  lLight.setIcon("mdi:light-flood-down");

  sTemperature.setName("Temperture");
  sTemperature.setIcon("mdi:thermometer");
  sTemperature.setUnitOfMeasurement("C");
  
  sHumidity.setName("Humidity");
  sHumidity.setIcon("mdi:water-percent");
  sHumidity.setUnitOfMeasurement("%");
  
  

  mqtt.begin(MQTT_SERVER.c_str(), MQTT_USER.c_str(), MQTT_PASSWORD.c_str());


/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*-----------------------------------TIMER-------------------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
  timer.every(5000, scrivi);
  timer.every(5000, checkWiFi);
  timer.every(2000, mqttStates);

}

void loop() {
  mqtt.loop();

  generalButton.update();


  if(generalButton.changed()){
    if(generalButton.read()){
      generalButtonState = true;
      waterPumpState = true;
      airPumpState = true;
      floatSensorState = true;
      //fanState = true;
      Serial.println("General Button Enabled");
    }else{
      generalButtonState = false;
      waterPumpState = false;
      airPumpState = false;
      floatSensorState = false;
      fanState = false;
      Serial.println("General Button Disabled");
    }
  }

  bsGeneralButton.setState(generalButtonState);
  swWaterPump.onCommand(onWaterPumpCommand);
  swAirPump.onCommand(onAirPumpCommand);
  swFan.onCommand(onFanCommand);
  lLight.onStateCommand(onLightCommand);


  timer.tick();
}

void setupWiFi() {
  WiFi.disconnect();
  delay(100);
  WiFi.mode(WIFI_STA);        //define connection tipe

  int counter = 0;
  delay(10);
  
                              //connecting to a WiFi network
  Serial.print("Connecting to ");
 // Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);


  WiFi.macAddress(mac);
  
  //wait if the connection begin
  while(WiFi.status() != WL_CONNECTED && counter++ < 8){
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");

  if(WiFi.status() == WL_CONNECTED){
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }else{
    Serial.println("WiFi NOT connected!!!");
  }
}

/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------TIMER FUNCTION---------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
bool scrivi(void *){
  Serial.println("5 secondi");
  mqtt.isConnected() ? Serial.println("true") : Serial.println("false");

  return true;
}

bool mqttStates(void *){
  bsFloatSensor.setState(floatSensorState);
  bsGeneralButton.setState(generalButtonState);
  swWaterPump.setState(waterPumpState);
  swAirPump.setState(airPumpState);
  swFan.setState(fanState);
  temperature = am2320.temperatureC - temperatureOffset;
  humidity = am2320.humidity;
  char temp[5];
  dtostrf(temperature, 0, 3, temp);
  sTemperature.setValue(temp);
  dtostrf(humidity, 0, 3, temp);
  sHumidity.setValue(temp);

  return true;
}

bool checkWiFi(void *){
  if (WiFi.status() != WL_CONNECTED){
    Serial.println("Reconnecting to WiFi");
    setupWiFi();
  }
  return true;
}

/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------MQTT-------------------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
void onConnected() {
  // this method will be called when connection to MQTT broker is established
  Serial.println("Connesso al broker");
}

void onMessage(const char* topic, const uint8_t* payload, uint16_t length) {
  // this method will be called each time the device receives an MQTT message
  
}

/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------MQTT COMMAND-----------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/


void onWaterPumpCommand(bool state, HASwitch* sender){
  waterPumpState = state;
  digitalWrite(WATER_PUMP, (waterPumpState ? HIGH : LOW));
  sender->setState(waterPumpState); // report state back to the Home Assistant
}

void onAirPumpCommand(bool state, HASwitch* sender){
  airPumpState = state;
  digitalWrite(AIR_PUMP, (airPumpState ? HIGH : LOW));
  sender->setState(airPumpState); // report state back to the Home Assistant
}

void onFanCommand(bool state, HASwitch* sender){
  fanState = state;
  digitalWrite(FAN, (fanState ? HIGH : LOW));
  sender->setState(fanState); // report state back to the Home Assistant
}

void onLightCommand(bool state, HALight* sender) {
  lightState = state;
  sender->setState(lightState); // report state back to the Home Assistant
}





