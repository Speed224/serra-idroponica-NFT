/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------Include Files----------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <AM2320_asukiaaa.h>




/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------Local definitions------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
#define PERIOD_SECOND_1         1000
#define PERIOD_SECOND_5         5000
#define PERIOD_SECOND_15        15000
#define PERIOD_MINUTE_1         60000
#define PERIOD_MINUTE_5         300000
#define PERIOD_MINUTE_15        900000
#define PERIOD_HOUR_1           3600000




/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------I/O Definitions--------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
#define POT   35
#define LIGHT 32
#define WATER_PUMP  33 

const String              POT_NAME = "pot";
const String              LIGHT_NAME = "_light";
const String              WATER_PUMP_NAME = "_waterpump";
const String              TEMPERATURE_NAME = "_temperature";
const String              HUMIDITY_NAME = "_humidity";



/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------Configuration----------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
String              WIFI_SSID;
String              WIFI_PASSWORD;
String              MQTT_SERVER = "192.168.1.225";                            // MQTT Server IP
String              MQTT_USER;                                                // MQTT Server User Name
String              MQTT_PASSWORD;                                            // MQTT Server password
int                 MQTT_PORT = 1883;                                         // MQTT Server Port



const char*         DEVICE_MODEL = "esp32Serra";                              // Hardware Model
const char*         SOFTWARE_VERSION = "0.1";                                 // Firmware Version
const char*         MANUFACTURER = "Speed224";                                // Manufacturer Name
const String        DEVICE_NAME = "serra";                                    // Device Name
const String        MQTT_TOPIC_STATUS = "esp32iot/" + DEVICE_NAME;
const String        MQTT_TOPIC_HA_PREFIX = "homeassistant/";
const String        MQTT_TOPIC_DISCOVERY_SUFFIX = "/config";
const String        MQTT_TOPIC_STATE_SUFFIX = "/state";
const String        MQTT_TOPIC_COMMAND_SUFFIX = "/command";


const String        MQTT_TOPIC_OPTION = MQTT_TOPIC_HA_PREFIX + MQTT_TOPIC_STATUS + "/option";
const String        MQTT_TOPIC_COMMAND = MQTT_TOPIC_HA_PREFIX + MQTT_TOPIC_STATUS + MQTT_TOPIC_COMMAND_SUFFIX;
const String        MQTT_TOPIC_HA_STATUS = MQTT_TOPIC_HA_PREFIX + "status";

//Sensor && Actuator Topics

const String        MQTT_LIGHT_TOPIC =  MQTT_TOPIC_HA_PREFIX + "light/" + MQTT_TOPIC_STATUS + LIGHT_NAME;
const String        MQTT_LIGHT_TOPIC_STATE =  MQTT_LIGHT_TOPIC + MQTT_TOPIC_STATE_SUFFIX; 
const String        MQTT_LIGHT_TOPIC_COMMAND =  MQTT_LIGHT_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX;

const String        MQTT_WATER_PUMP_TOPIC =  MQTT_TOPIC_HA_PREFIX + "switch/" + MQTT_TOPIC_STATUS + WATER_PUMP_NAME;
const String        MQTT_WATER_PUMP_TOPIC_STATE =  MQTT_WATER_PUMP_TOPIC + MQTT_TOPIC_STATE_SUFFIX; 
const String        MQTT_WATER_PUMP_TOPIC_COMMAND =  MQTT_WATER_PUMP_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX; 

const String        MQTT_TEMPERATURE_TOPIC =  MQTT_TOPIC_HA_PREFIX + "sensor/" + MQTT_TOPIC_STATUS + TEMPERATURE_NAME;
const String        MQTT_HUMIDITY_TOPIC =  MQTT_TOPIC_HA_PREFIX + "sensor/" + MQTT_TOPIC_STATUS + HUMIDITY_NAME;
const String        MQTT_THERMOMETER_TOPIC_STATE =  MQTT_TOPIC_HA_PREFIX + "sensor/" + MQTT_TOPIC_STATUS + "_thermometer" + MQTT_TOPIC_STATE_SUFFIX; 


const int           topicsNumber = 5;
const String        topics[topicsNumber] = {
                                            MQTT_TOPIC_HA_STATUS, 
                                            MQTT_TOPIC_OPTION, 
                                            MQTT_TOPIC_COMMAND, 
                                            MQTT_LIGHT_TOPIC_COMMAND, 
                                            MQTT_WATER_PUMP_TOPIC_COMMAND

                                           };


/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------Public variables-------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
WiFiClient          wifiClient;
PubSubClient        mqttPubSub(wifiClient);
Preferences         preferences;
AM2320_asukiaaa     am2320;

boolean             firstBoot = true;

int                 count = 0;
String              UNIQUE_ID;
bool                sendMqttData = false;

//DEFAULT wait time for sending data
unsigned int        sendDataPeriod = PERIOD_SECOND_5;

unsigned long       dataPreviousMillis = 0;
unsigned long       wifiPreviousMillis = 0;
unsigned long       lightPreviousMillis = 0;
unsigned long       currentMillis;

unsigned int        lightONPeriod = PERIOD_HOUR_1*12;
unsigned int        lightOFFPeriod = PERIOD_HOUR_1*12;

char                command;


bool                isDayTime = true;
bool                lightState = true;
bool                pumpState = true;



/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------SETUP------------------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
void setup() 
{



  Serial.begin(115200);
  Serial.println("Inizio setup");


  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Input output Pin definitions
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  pinMode(POT, INPUT);
  pinMode(LIGHT, OUTPUT);
  pinMode(WATER_PUMP, OUTPUT);

  Wire.begin();
  am2320.setWire(&Wire);




  preferences.begin("credentials", true);

  WIFI_SSID = preferences.getString("SSID", ""); 
  WIFI_PASSWORD = preferences.getString("PASSWORD", "");
  MQTT_USER = preferences.getString("MQTT_USER", ""); 
  MQTT_PASSWORD = preferences.getString("MQTT_PASSWORD", "");

  Serial.printf("\nSSID: %s", WIFI_SSID);
  Serial.printf("\nPASSWORD: %s\n", WIFI_PASSWORD);
  Serial.printf("\nMQTT_USER: %s", MQTT_USER);
  Serial.printf("\nMQTT_PASSWORD: %s\n", MQTT_PASSWORD);

  

  if (WIFI_SSID == "" || WIFI_PASSWORD == ""){
    Serial.println("No values saved for ssid or password");
  }
 

  delay(500);

  Serial.println("");
  Serial.println("");
  Serial.println("----------------------------------------------");
  Serial.print("MODEL: ");
  Serial.println(DEVICE_MODEL);
  Serial.print("DEVICE: ");
  Serial.println(DEVICE_NAME);
  Serial.print("SW Rev: ");
  Serial.println(SOFTWARE_VERSION);
  Serial.println("----------------------------------------------");
  
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Wifi Init
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  wifiSetup();

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // MQTT Init
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  mqttPubSub.setServer(MQTT_SERVER.c_str(), MQTT_PORT);
  mqttPubSub.setCallback(mqttReceiverCallback);
  mqttPubSub.setBufferSize(600);

  mqttConnect();



  //ALL THE LOGIC THAT MUST START ANYWAY AFTER A REBOOT
  lightLogic();
  pumpLogic();


}

/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------LOOP-------------------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
void loop() 
{
  currentMillis = millis();
  mqttPubSub.loop(); //call often docs


  if(currentMillis - dataPreviousMillis > sendDataPeriod || sendMqttData)
  {
    dataPreviousMillis = currentMillis;

    if(!mqttPubSub.connected())
      mqttConnect();
    else{

      mqttStates();

      bool error = mqttSendData("pot", (String)analogRead(POT));
      if(!error)
        Serial.println("MQTT: ERROR cannot send data");
      else
        Serial.println("MQTT: Sending Data");

      sendMqttData = false;
      
    }
  }


  if(isDayTime){
    if(currentMillis - lightPreviousMillis >= lightONPeriod) {
      lightState = true;
      lightLogic();

      lightPreviousMillis = currentMillis;
      isDayTime = false;
    }
  }else{
    if(currentMillis - lightPreviousMillis >= lightOFFPeriod) {
      lightState = false;
      lightLogic();

      lightPreviousMillis = currentMillis;
      isDayTime = true;
    }
  }

  
  
  
  if (currentMillis - wifiPreviousMillis >= PERIOD_MINUTE_1) {
    if(WiFi.status() != WL_CONNECTED)
      wifiSetup();

    wifiPreviousMillis = currentMillis;
  }



  if(Serial.available())
    command = Serial.read();
  commandExecutor(command);  

}


/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------ Public Functions -----------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
void wifiSetup() 
{
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);

    int counter = 0;
    byte mac[6];
    delay(10);
    
    // We start by connecting to a WiFi network
    Serial.print("Connecting to ");
    Serial.println(WIFI_SSID);

    WiFi.begin(WIFI_SSID.c_str(), WIFI_PASSWORD.c_str());

    WiFi.macAddress(mac);
    UNIQUE_ID = String(mac[0],HEX) +String(mac[1],HEX) +String(mac[2],HEX) +String(mac[3],HEX) + String(mac[4],HEX) + String(mac[5],HEX);

    Serial.print("Unique ID: ");
    Serial.println(UNIQUE_ID);    
   
    while(WiFi.status() != WL_CONNECTED && counter++ < 8) 
    {
      delay(1000);
      Serial.print(".");
    }
    Serial.println("");

    if(WiFi.status() == WL_CONNECTED)
    {
      Serial.println("WiFi connected");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    } else
    {
      Serial.println("WiFi NOT connected!!!");
    }
}

void mqttConnect() 
{
  mqttPubSub.disconnect();
  if (WiFi.status() == WL_CONNECTED)
  {
    if(!mqttPubSub.connected())
    {
      Serial.print("Attempting MQTT connection...");
      // Attempt to connect
      if (mqttPubSub.connect(DEVICE_NAME.c_str(), MQTT_USER.c_str(), MQTT_PASSWORD.c_str())) 
      {
        Serial.println("connected");
        //subscribe to topics
        mqttTopicsSubscribe(topics, topicsNumber);
        if (firstBoot){
          mqttHomeAssistantDiscovery();
          firstBoot = false;
        }
      } else 
      {
        Serial.print("failed, rc=");
        Serial.print(mqttPubSub.state());
        Serial.println();
      }
    } 
  }else{
    Serial.println("Cannot connect to mqtt because wifi is offline");
  }
}

void mqttTopicsSubscribe(const String topics[topicsNumber], const int topicsNumber){
  for(int i = 0; i < topicsNumber; i++){
    String success = (mqttPubSub.subscribe(topics[i].c_str())) ? "Subscribed" : "ERROR cannot subscribe";
    Serial.print(success);
    Serial.print(" to topic ");
    Serial.print(topics[i]);
    Serial.println();
  }
      
}

bool mqttSendData(String sensorName, String data){
  StaticJsonDocument<200> payload;

  payload[String(sensorName)] = data;

  String strPayload;
  serializeJson(payload, strPayload);


  String potStateTopic = MQTT_TOPIC_HA_PREFIX + "sensor/" + MQTT_TOPIC_STATUS + '_' + sensorName + MQTT_TOPIC_STATE_SUFFIX;
  return mqttPubSub.publish(potStateTopic.c_str(), strPayload.c_str());
  
}



void mqttHomeAssistantDiscovery()
{
  String discoveryTopic;
  String stateTopic;
  String commandTopic;
  String name;
  String topic;
  
  String payload;
  String strPayload;

  

  if(mqttPubSub.connected())
  {
    Serial.println("SEND HOME ASSISTANT DISCOVERY!!!");

    delay(500);


    StaticJsonDocument<600> payload;
    JsonObject device;
    JsonArray identifiers;

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // POT
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    name = "_" + POT_NAME;
    topic = MQTT_TOPIC_HA_PREFIX + "sensor/" + MQTT_TOPIC_STATUS + name;
    discoveryTopic = topic + MQTT_TOPIC_DISCOVERY_SUFFIX;
    stateTopic = topic + MQTT_TOPIC_STATE_SUFFIX;
    
    payload["name"] = DEVICE_NAME + name;
    payload["unique_id"] = UNIQUE_ID + name;
    payload["state_topic"] = stateTopic;
    payload["value_template"] = "{{ value_json.pot | is_defined }}";
    payload["device_class"] = "voltage";
    payload["unit_of_measurement"] = "mV";


    device = payload.createNestedObject("device");

    device["name"] = DEVICE_NAME;
    device["model"] = DEVICE_MODEL;
    device["sw_version"] = SOFTWARE_VERSION;
    device["manufacturer"] = MANUFACTURER;
    identifiers = device.createNestedArray("identifiers");
    identifiers.add(UNIQUE_ID);

    serializeJsonPretty(payload, Serial);
    Serial.println(" ");
    serializeJson(payload, strPayload);

    mqttPubSub.publish(discoveryTopic.c_str(), strPayload.c_str());

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // LIGHT
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    delay(500);
    
    payload.clear();
    device.clear();
    identifiers.clear();
    strPayload.clear();

    discoveryTopic = MQTT_LIGHT_TOPIC + MQTT_TOPIC_DISCOVERY_SUFFIX;

    
    payload["name"] = DEVICE_NAME + LIGHT_NAME;
    payload["unique_id"] = UNIQUE_ID + LIGHT_NAME;
    payload["state_topic"] = MQTT_LIGHT_TOPIC_STATE;
    payload["command_topic"] = MQTT_LIGHT_TOPIC_COMMAND;


    device = payload.createNestedObject("device");

    device["name"] = DEVICE_NAME;
    device["model"] = DEVICE_MODEL;
    device["sw_version"] = SOFTWARE_VERSION;
    device["manufacturer"] = MANUFACTURER;
    identifiers = device.createNestedArray("identifiers");
    identifiers.add(UNIQUE_ID);

    serializeJsonPretty(payload, Serial);
    Serial.println(" ");
    serializeJson(payload, strPayload);

    mqttPubSub.publish(discoveryTopic.c_str(), strPayload.c_str());

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // WATER_PUMP
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    delay(500);


    payload.clear();
    device.clear();
    identifiers.clear();
    strPayload.clear();

    discoveryTopic = MQTT_WATER_PUMP_TOPIC + MQTT_TOPIC_DISCOVERY_SUFFIX;

    
    payload["name"] = DEVICE_NAME + WATER_PUMP_NAME;
    payload["unique_id"] = UNIQUE_ID + WATER_PUMP_NAME;
    payload["state_topic"] = MQTT_WATER_PUMP_TOPIC_STATE;
    payload["command_topic"] = MQTT_WATER_PUMP_TOPIC_COMMAND;
    payload["device_class"] = "switch";

    device = payload.createNestedObject("device");

    device["name"] = DEVICE_NAME;
    device["model"] = DEVICE_MODEL;
    device["sw_version"] = SOFTWARE_VERSION;
    device["manufacturer"] = MANUFACTURER;
    identifiers = device.createNestedArray("identifiers");
    identifiers.add(UNIQUE_ID);

    serializeJsonPretty(payload, Serial);
    Serial.println(" ");
    serializeJson(payload, strPayload);

    mqttPubSub.publish(discoveryTopic.c_str(), strPayload.c_str());

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // TEMPERATURE
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    delay(500);


    payload.clear();
    device.clear();
    identifiers.clear();
    strPayload.clear();

    discoveryTopic = MQTT_TEMPERATURE_TOPIC + MQTT_TOPIC_DISCOVERY_SUFFIX;
    
    payload["name"] = DEVICE_NAME + TEMPERATURE_NAME;
    payload["unique_id"] = UNIQUE_ID + TEMPERATURE_NAME;
    payload["state_topic"] = MQTT_THERMOMETER_TOPIC_STATE;

    payload["value_template"] = "{{ value_json.temperature | is_defined}}";
    payload["device_class"] = "temperature";
    payload["unit_of_measurement"] = "°C";
    payload["suggested_display_precision"] = 1;


    device = payload.createNestedObject("device");

    device["name"] = DEVICE_NAME;
    device["model"] = DEVICE_MODEL;
    device["sw_version"] = SOFTWARE_VERSION;
    device["manufacturer"] = MANUFACTURER;
    identifiers = device.createNestedArray("identifiers");
    identifiers.add(UNIQUE_ID);

    serializeJsonPretty(payload, Serial);
    Serial.println(" ");
    serializeJson(payload, strPayload);

    mqttPubSub.publish(discoveryTopic.c_str(), strPayload.c_str());

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // HUMIDITY
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    delay(500);

    payload.clear();
    device.clear();
    identifiers.clear();
    strPayload.clear();

    discoveryTopic = MQTT_HUMIDITY_TOPIC + MQTT_TOPIC_DISCOVERY_SUFFIX;
    
    payload["name"] = DEVICE_NAME + HUMIDITY_NAME;
    payload["unique_id"] = UNIQUE_ID + HUMIDITY_NAME;
    payload["state_topic"] = MQTT_THERMOMETER_TOPIC_STATE;

    payload["value_template"] = "{{ value_json.humidity | is_defined }}";
    payload["device_class"] = "humidity";
    payload["unit_of_measurement"] = "%";


    device = payload.createNestedObject("device");

    device["name"] = DEVICE_NAME;
    device["model"] = DEVICE_MODEL;
    device["sw_version"] = SOFTWARE_VERSION;
    device["manufacturer"] = MANUFACTURER;
    identifiers = device.createNestedArray("identifiers");
    identifiers.add(UNIQUE_ID);

    serializeJsonPretty(payload, Serial);
    Serial.println(" ");
    serializeJson(payload, strPayload);

    mqttPubSub.publish(discoveryTopic.c_str(), strPayload.c_str());

  }
}

void commandExecutor(char command){
  switch (command) {
    case 'd':
      sendMqttData = true;
      Serial.println("Command: Sending Data");
    break;
    case 'w':
      wifiSetup();
      mqttConnect();
    break;
    case 'R':
      ESP.restart();
    break;
    case 'L':
      lightState = true;
      lightLogic();
    break;
    case 'l':
      lightState = false;
      lightLogic();
    break;
    case 'h':
    case 'H':
    Serial.println();
    Serial.println("Available Command:\n");
    Serial.println("- h or H for help");
    Serial.println();
    break;
  }

}


void mqttReceiverCallback(char* topic, byte* payload, unsigned int length) 
{

    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(". Message: ");

    byte state = 0;

    String message;

    StaticJsonDocument<600> json;
    
    for (int i = 0; i < length; i++) 
    {
        Serial.print((char)payload[i]);
        message += (char)payload[i];
    }
    Serial.println();

    //SERRA OPTION FROM MQTT
    if(String(topic) == String(MQTT_TOPIC_OPTION)) 
    {
      deserializeJson(json, payload);
      sendDataPeriod = json["sendDataPeriod"];
      if(sendDataPeriod < 3000)
        sendDataPeriod = 3000;
      Serial.print("sendDataPeriod: ");
      Serial.println(sendDataPeriod);
    }


    //SERRA COMMAND FROM MQTT
    if(String(topic) == String(MQTT_TOPIC_COMMAND)){
      Serial.println((char)payload[0]);
      commandExecutor((char)payload[0]);
    }

    if(String(topic) == String(MQTT_LIGHT_TOPIC_COMMAND)) 
    {
      if(message == "ON")
        lightState = true;
      else
        lightState = false;
      lightLogic();
    }

    if(String(topic) == String(MQTT_WATER_PUMP_TOPIC_COMMAND)) 
    {
      if(message == "ON")
        pumpState = true;
      else
        pumpState = false;
      pumpLogic();
    }

    //HOME ASSISTANT STATUS
    if(String(topic) == String(MQTT_TOPIC_HA_STATUS)) 
    {
      if(message == "online")
          mqttHomeAssistantDiscovery();
    }

}

void lightLogic(){
  if(lightState){
    digitalWrite(LIGHT, HIGH);
    mqttPubSub.publish(MQTT_LIGHT_TOPIC_STATE.c_str(), "ON");
  }
  else{
    digitalWrite(LIGHT, LOW);
    mqttPubSub.publish(MQTT_LIGHT_TOPIC_STATE.c_str(), "OFF");
  }
}

void pumpLogic(){
  if(pumpState){
    digitalWrite(WATER_PUMP, HIGH);
    mqttPubSub.publish(MQTT_WATER_PUMP_TOPIC_STATE.c_str(), "ON");
  }
  else{
    digitalWrite(WATER_PUMP, LOW);
    mqttPubSub.publish(MQTT_WATER_PUMP_TOPIC_STATE.c_str(), "OFF");
  }
}

void mqttStates(){
  char* message;
  if(lightState)
    message = "ON";
  else
    message = "OFF";
  mqttPubSub.publish(MQTT_LIGHT_TOPIC_STATE.c_str(), message);
  if(pumpState)
    message = "ON";
  else
    message = "OFF";
  mqttPubSub.publish(MQTT_WATER_PUMP_TOPIC_STATE.c_str(), message);

  if (am2320.update() != 0) {
    Serial.println("Error: Cannot update sensor values.");
  }else{
        //cannot resuse message string (to check)
    StaticJsonDocument<200> payload;
    String strPayload;
    
    payload["temperature"] = am2320.temperatureC;
    payload["humidity"] = am2320.humidity;

    
    serializeJson(payload, strPayload);
    mqttPubSub.publish(MQTT_THERMOMETER_TOPIC_STATE.c_str(), strPayload.c_str());
    

    
  }
}
