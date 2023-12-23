//TODO: refresh command

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

Bounce                generalButton = Bounce();

const char*                commandTopic = "data/esp32serra/command";
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
/*
void IRAM_ATTR floatSensorISR(){
  waterPumpState = false;
  digitalWrite(WATER_PUMP, HIGH);
  floatSensorState = false;
}
*/

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
  //attachInterrupt(FLOAT_SENSOR, floatSensorISR, FALLING);

  //tutti i relè sono spenti
  digitalWrite(WATER_PUMP, HIGH);
  digitalWrite(AIR_PUMP, HIGH);
  digitalWrite(LIGHT, HIGH);
  digitalWrite(FAN, HIGH);

  floatSensorState = digitalRead(FLOAT_SENSOR);
  generalButtonManager(digitalRead(GENERAL_BUTTON));

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

  floatSensorState = digitalRead(FLOAT_SENSOR);
  if(!floatSensorState){
    waterPumpState = false;
    writeStates();
  }

  generalButton.update();
  if(generalButton.changed()){
    generalButtonManager(generalButton.read());
  }

  bsGeneralButton.setState(generalButtonState);
  swWaterPump.onCommand(onWaterPumpCommand);
  swAirPump.onCommand(onAirPumpCommand);
  swFan.onCommand(onFanCommand);
  lLight.onStateCommand(onLightCommand);


  timer.tick();
}

void generalButtonManager(bool state){
  if(state){
      generalButtonState = state;
      waterPumpState = true;
      airPumpState = true;
      fanState = fanState;
      lightState = true;
      Serial.println("General Button Enabled");
      writeStates();
    }else{
      generalButtonState = state;
      waterPumpState = false;
      airPumpState = false;
      fanState = false;
      lightState = false;
      Serial.println("General Button Disabled");
      writeStates();
    }
}

void setupWiFi() {
  WiFi.disconnect();
  delay(100);
  WiFi.mode(WIFI_STA);        //define connection tipe

  int counter = 0;
  delay(10);
  
                              //connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  Serial.println(WIFI_PASSWORD);

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
/*------------------------------------FUNCTION---------------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/

void writeStates(){
  digitalWrite(WATER_PUMP, (waterPumpState ? LOW : HIGH));
  digitalWrite(AIR_PUMP, (airPumpState ? LOW : HIGH));
  digitalWrite(FAN, (fanState ? LOW : HIGH));
  digitalWrite(LIGHT, (lightState ? LOW : HIGH));
}

/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------TIMER FUNCTION---------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
bool scrivi(void *){
  Serial.println("5 secondi timer");
  mqtt.isConnected() ? Serial.println("Connesso a MQTT") : Serial.println("Non connesso a MQTT");
  Serial.println("temperatureC: " + String(am2320.temperatureC) + " C");
  Serial.println("humidity: " + String(am2320.humidity) + " %");
  Serial.println();
  Serial.println();
  return true;
}

bool mqttStates(void *){
  bsFloatSensor.setState(floatSensorState);
  bsGeneralButton.setState(generalButtonState);
  swWaterPump.setState(waterPumpState);
  swAirPump.setState(airPumpState);
  swFan.setState(fanState);
  lLight.setState(lightState);

  if(am2320.update()!=0){
    Serial.println("Error: Cannot update sensor values.");
  }else{
    temperature = am2320.temperatureC - temperatureOffset;
    humidity = am2320.humidity;
    char temp[8];
    dtostrf(temperature, 6, 2, temp);
    sTemperature.setValue(temp);
    dtostrf(humidity, 6, 2, temp);
    sHumidity.setValue(temp);
  }
  

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
  mqtt.subscribe(commandTopic);
}

void onMessage(const char* topic, const uint8_t* payload, uint16_t length) {
  // this method will be called each time the device receives an MQTT message
  String message;
  for (int i = 0; i < length; i++) {
    //Serial.print((char)payload[i]);
    message += (char)payload[i];
  }
  char* nothing;
  if(message == "r"){
    mqttStates(nothing);
    Serial.println();
    Serial.println("Dati inviati");
    Serial.println();

  }

  if(message == "w"){
    Serial.println();
    Serial.println("TODO sensore TDS e PH");
    Serial.println();
  }
}

/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------MQTT COMMAND-----------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
// relay works inverted HIGH is low LOW is high

void onWaterPumpCommand(bool state, HASwitch* sender){
  waterPumpState = state;
  digitalWrite(WATER_PUMP, (waterPumpState ? LOW : HIGH));
  sender->setState(waterPumpState); // report state back to the Home Assistant
}

void onAirPumpCommand(bool state, HASwitch* sender){
  airPumpState = state;
  digitalWrite(AIR_PUMP, (airPumpState ? LOW : HIGH));
  sender->setState(airPumpState); // report state back to the Home Assistant
}

void onFanCommand(bool state, HASwitch* sender){
  fanState = state;
  digitalWrite(FAN, (fanState ? LOW : HIGH));
  sender->setState(fanState); // report state back to the Home Assistant
}

void onLightCommand(bool state, HALight* sender) {
  lightState = state;
  digitalWrite(LIGHT, (lightState ? LOW : HIGH));
  sender->setState(lightState); // report state back to the Home Assistant
}





