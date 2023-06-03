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
const byte                POT = 35;
const byte                LIGHT = 32;
const byte                WATER_PUMP = 33;
const byte                TDS = 34;
//const byte              PH = 35;
const byte                AIR_PUMP = 19;
const byte                FLOAT_SENSOR = 17;

const String              POT_NAME = "pot";
const String              LIGHT_NAME = "_light";
const String              WATER_PUMP_NAME = "_waterpump";
const String              AIR_PUMP_NAME = "_airpump";
const String              TEMPERATURE_NAME = "_temperature";
const String              HUMIDITY_NAME = "_humidity";
const String              TDS_NAME = "_tds";
const String              PH_NAME = "_ph";
const String              FLOAT_SENSOR_NAME = "_floatsensor";


/*
* TDS && PH 
*/
const byte SCOUNT = 30;            // sum of sample point
const float VREF = 3.3;         //Volt

int analogBuffer[SCOUNT];     // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
int copyIndex = 0;

float averageVoltage = 0;
float waterTemperature = 20;  

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

// WATER PUMP
const String        MQTT_WATER_PUMP_TOPIC =  MQTT_TOPIC_HA_PREFIX + "switch/" + MQTT_TOPIC_STATUS + WATER_PUMP_NAME;
const String        MQTT_WATER_PUMP_TOPIC_STATE =  MQTT_WATER_PUMP_TOPIC + MQTT_TOPIC_STATE_SUFFIX; 
const String        MQTT_WATER_PUMP_TOPIC_COMMAND =  MQTT_WATER_PUMP_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX; 

// AIR PUMP
const String        MQTT_AIR_PUMP_TOPIC =  MQTT_TOPIC_HA_PREFIX + "switch/" + MQTT_TOPIC_STATUS + AIR_PUMP_NAME;
const String        MQTT_AIR_PUMP_TOPIC_STATE =  MQTT_AIR_PUMP_TOPIC + MQTT_TOPIC_STATE_SUFFIX; 
const String        MQTT_AIR_PUMP_TOPIC_COMMAND =  MQTT_AIR_PUMP_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX; 

// THERMOMETER
const String        MQTT_TEMPERATURE_TOPIC =  MQTT_TOPIC_HA_PREFIX + "sensor/" + MQTT_TOPIC_STATUS + TEMPERATURE_NAME;
const String        MQTT_HUMIDITY_TOPIC =  MQTT_TOPIC_HA_PREFIX + "sensor/" + MQTT_TOPIC_STATUS + HUMIDITY_NAME;
const String        MQTT_THERMOMETER_TOPIC_STATE =  MQTT_TOPIC_HA_PREFIX + "sensor/" + MQTT_TOPIC_STATUS + "_thermometer" + MQTT_TOPIC_STATE_SUFFIX;

// WATER QUALITY
const String        MQTT_TDS_TOPIC =  MQTT_TOPIC_HA_PREFIX + "sensor/" + MQTT_TOPIC_STATUS + TDS_NAME;
//const String        MQTT_PH_TOPIC =  MQTT_TOPIC_HA_PREFIX + "sensor/" + MQTT_TOPIC_STATUS + PH_NAME;
const String        MQTT_WATER_QUALITY_TOPIC_STATE =  MQTT_TOPIC_HA_PREFIX + "sensor/" + MQTT_TOPIC_STATUS + "_water_quality" + MQTT_TOPIC_STATE_SUFFIX;

//WATER LEVEL
const String        MQTT_FLOAT_SENSOR_TOPIC =  MQTT_TOPIC_HA_PREFIX + "binary_sensor/" + MQTT_TOPIC_STATUS + FLOAT_SENSOR_NAME;
const String        MQTT_FLOAT_SENSOR_TOPIC_STATE =  MQTT_FLOAT_SENSOR_TOPIC + MQTT_TOPIC_STATE_SUFFIX;


const int           topicsNumber = 6;
const String        topics[topicsNumber] = {
                                            MQTT_TOPIC_HA_STATUS, 
                                            MQTT_TOPIC_OPTION, 
                                            MQTT_TOPIC_COMMAND, 
                                            MQTT_LIGHT_TOPIC_COMMAND, 
                                            MQTT_WATER_PUMP_TOPIC_COMMAND,
                                            MQTT_AIR_PUMP_TOPIC_COMMAND

                                           };


/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------Public variables-------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
WiFiClient          wifiClient;
PubSubClient        mqttPubSub(wifiClient);
Preferences         preferences;
AM2320_asukiaaa     am2320;

bool                firstBoot = true;

short               numberOfSamples = 5;
String              UNIQUE_ID;
bool                sendMqttData = false;

//DEFAULT wait time for sending data
unsigned int        pollingDataPeriod = PERIOD_SECOND_1*3;
unsigned int        sendDataPeriod = PERIOD_SECOND_5;

unsigned long       pollingPreviousMillis = 0;
unsigned long       dataPreviousMillis = 0;
unsigned long       wifiPreviousMillis = 0;
unsigned long       lightPreviousMillis = 0;
unsigned long       tdsSamplePreviousMillis = 0;
unsigned long       currentMillis;

unsigned int        lightONPeriod = PERIOD_HOUR_1*12;
unsigned int        lightOFFPeriod = PERIOD_HOUR_1*12;

char                command;
bool                bypassPumpSecurity = false;

bool                isDayTime = true;
bool                lightState = true;
bool                waterPumpState = true;
bool                airPumpState = true;
bool                floatSensorState = true;
float               temperature;
float               humidity;
float               tds;
//float             ph;





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
  pinMode(AIR_PUMP, OUTPUT);
  pinMode(TDS, INPUT);
  pinMode(FLOAT_SENSOR, INPUT_PULLUP);

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
  lightLogic(lightState);
  floatSensorLogic();
  waterPumpLogic(waterPumpState);
  airPumpLogic(airPumpState);
  am2320Logic();
  tdsLogic();

  


}

/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------LOOP-------------------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
void loop() 
{
  currentMillis = millis();
  mqttPubSub.loop(); //call often (docs)
  
  if(currentMillis - pollingPreviousMillis > pollingDataPeriod)
  {
    lightLogic(lightState);
    floatSensorLogic();
    waterPumpLogic(waterPumpState);
    airPumpLogic(airPumpState);
    am2320Logic();
    tdsLogic();

    pollingPreviousMillis = currentMillis;
  }

  
  //tds sample for algorithm

  if(currentMillis - tdsSamplePreviousMillis > sendDataPeriod/numberOfSamples){     //every 1/n of dataPeriod milliseconds,read the analog value from the ADC
      analogBuffer[analogBufferIndex] = analogRead(TDS);    //read the analog value and store into the buffer
      analogBufferIndex++;
      if(analogBufferIndex == SCOUNT){ 
        analogBufferIndex = 0;
      }

      tdsSamplePreviousMillis = currentMillis;
    }   


  if(currentMillis - dataPreviousMillis > sendDataPeriod || sendMqttData)
  {
    dataPreviousMillis = currentMillis;

    lightLogic(lightState);
    floatSensorLogic();
    waterPumpLogic(waterPumpState);
    airPumpLogic(airPumpState);
    am2320Logic();
    tdsLogic();

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
      lightLogic(true);

      lightPreviousMillis = currentMillis;
      isDayTime = false;
    }
  }else{
    if(currentMillis - lightPreviousMillis >= lightOFFPeriod) {
      lightLogic(false);

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
    // AIR_PUMP
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    delay(500);


    payload.clear();
    device.clear();
    identifiers.clear();
    strPayload.clear();

    discoveryTopic = MQTT_AIR_PUMP_TOPIC + MQTT_TOPIC_DISCOVERY_SUFFIX;

    
    payload["name"] = DEVICE_NAME + AIR_PUMP_NAME;
    payload["unique_id"] = UNIQUE_ID + AIR_PUMP_NAME;
    payload["state_topic"] = MQTT_AIR_PUMP_TOPIC_STATE;
    payload["command_topic"] = MQTT_AIR_PUMP_TOPIC_COMMAND;
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

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // TDS
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    delay(500);

    payload.clear();
    device.clear();
    identifiers.clear();
    strPayload.clear();

    discoveryTopic = MQTT_TDS_TOPIC + MQTT_TOPIC_DISCOVERY_SUFFIX;
    
    payload["name"] = DEVICE_NAME + TDS_NAME;
    payload["unique_id"] = UNIQUE_ID + TDS_NAME;
    payload["state_topic"] = MQTT_WATER_QUALITY_TOPIC_STATE;

    payload["value_template"] = "{{ value_json.tds | is_defined }}";
    payload["unit_of_measurement"] = "ppm";
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
    // FLOAT_SENSOR
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    delay(500);

    payload.clear();
    device.clear();
    identifiers.clear();
    strPayload.clear();

    discoveryTopic = MQTT_FLOAT_SENSOR_TOPIC + MQTT_TOPIC_DISCOVERY_SUFFIX;
    
    payload["name"] = DEVICE_NAME + FLOAT_SENSOR_NAME;
    payload["unique_id"] = UNIQUE_ID + FLOAT_SENSOR_NAME;
    payload["state_topic"] = MQTT_FLOAT_SENSOR_TOPIC_STATE;



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
      lightLogic(true);
    break;
    case 'l':
      lightLogic(false);
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
      //DATA PERIOD
      deserializeJson(json, payload);
      sendDataPeriod = json["sendDataPeriod"];
      if(sendDataPeriod < 3000)
        sendDataPeriod = 3000;
      Serial.print("sendDataPeriod: ");
      Serial.println(sendDataPeriod);

    //BYPASS SECURITY
      bypassPumpSecurity = json["bypassPumpSecurity"];
      
      if(bypassPumpSecurity)
        Serial.println("PUMP SECURITY BYPASSED");
      else
        Serial.println("PUMP SECURITY BYPASS REMOVED");
        
    }


    //SERRA COMMAND FROM MQTT
    if(String(topic) == String(MQTT_TOPIC_COMMAND)){
      Serial.println((char)payload[0]);
      commandExecutor((char)payload[0]);
    }

    if(String(topic) == String(MQTT_LIGHT_TOPIC_COMMAND)) 
    {
      if(message == "ON")
        lightLogic(true);
      else
        lightLogic(false);
      
      mqttStates();
    }

    if(String(topic) == String(MQTT_WATER_PUMP_TOPIC_COMMAND)) 
    {
      //if there is water or bypass is true
      if(floatSensorState || bypassPumpSecurity){
        if(message == "ON")
          waterPumpLogic(true);
        else
          waterPumpLogic(false);
      }
      mqttStates();
    }

    if(String(topic) == String(MQTT_AIR_PUMP_TOPIC_COMMAND)) 
    {
      if(message == "ON")
        airPumpLogic(true);
      else
        airPumpLogic(false);
      
      mqttStates();
    }

    //HOME ASSISTANT STATUS
    if(String(topic) == String(MQTT_TOPIC_HA_STATUS)) 
    {
      if(message == "online")
          mqttHomeAssistantDiscovery();
    }
    

}

void lightLogic(bool logicState){
  lightState = logicState;
  if(lightState){
    digitalWrite(LIGHT, HIGH);
  }
  else{
    digitalWrite(LIGHT, LOW);
  }
  //mqttStates();
}

void waterPumpLogic(bool logicState){
  waterPumpState = logicState;
  if(waterPumpState){
    digitalWrite(WATER_PUMP, HIGH);
  }
  else{
    digitalWrite(WATER_PUMP, LOW);
  }
  //mqttStates();
}

void airPumpLogic(bool logicState){
  airPumpState = logicState;
  if(airPumpState){
    digitalWrite(AIR_PUMP, HIGH);
  }
  else{
    digitalWrite(AIR_PUMP, LOW);
  }
 //mqttStates();
}

void floatSensorLogic(){
  floatSensorState = digitalRead(FLOAT_SENSOR);
  
  if(!floatSensorState || !bypassPumpSecurity)
    waterPumpState = false;

  //mqttStates();
}

void am2320Logic(){
  if (am2320.update() != 0) {
    Serial.println("Error: Cannot update sensor values.");
  }else{
    temperature = am2320.temperatureC;
    humidity = am2320.humidity;
  }
  //mqttStates();
}

void tdsLogic(){
  //MEDIAN ALGORITHM
  int bTab[SCOUNT];
  for (byte i = 0; i<SCOUNT; i++)
  bTab[i] = analogBufferTemp[i];
  int i, j, bTemp;
  for (j = 0; j < SCOUNT - 1; j++) {
    for (i = 0; i < SCOUNT - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((SCOUNT & 1) > 0){
    bTemp = bTab[(SCOUNT - 1) / 2];
  }
  else {
    bTemp = (bTab[SCOUNT / 2] + bTab[SCOUNT / 2 - 1]) / 2;
  }

  // TDS READING
  float tdsValue;
  //float phValue;

  //TDS CALCS
  for(copyIndex=0; copyIndex<SCOUNT; copyIndex++){
    analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
    
    // read the analog value more stable by the median filtering algorithm, and convert to voltage value
    averageVoltage = bTemp * (float)VREF / 4096.0;     //TO FIT WITH ESP32 (12 BIT analogRead resolution) == 4096
    
    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0)); 
    float compensationCoefficient = 1.0+0.02*(waterTemperature-25.0);
    //temperature compensation
    float compensationVoltage = averageVoltage / compensationCoefficient;
    
    //convert voltage value to tds value
    tds = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage - 255.86 * compensationVoltage*  compensationVoltage + 857.39 * compensationVoltage) * 0.5;
  }
  mqttStates();
}

//check the states and send to MQTT server
void mqttStates(){
  
  mqttPubSub.publish(MQTT_LIGHT_TOPIC_STATE.c_str(), (lightState ? "ON" : "OFF"));
  mqttPubSub.publish(MQTT_FLOAT_SENSOR_TOPIC_STATE.c_str(), (floatSensorState ? "ON" : "OFF"));
  mqttPubSub.publish(MQTT_WATER_PUMP_TOPIC_STATE.c_str(), (waterPumpState ? "ON" : "OFF"));
  mqttPubSub.publish(MQTT_AIR_PUMP_TOPIC_STATE.c_str(), (airPumpState ? "ON" : "OFF"));
  
  //am2320
  StaticJsonDocument<200> payload;
  String strPayload;
    
  payload["temperature"] = am2320.temperatureC;
  payload["humidity"] = am2320.humidity;

  
  serializeJson(payload, strPayload);
  mqttPubSub.publish(MQTT_THERMOMETER_TOPIC_STATE.c_str(), strPayload.c_str());

  //TDS
  payload.clear();
  strPayload.clear();
  
  payload["tds"] = tds; //ppm
  //payload["ec"] = (tds * 2)/1000; //µS/cm
  //payload["ph"] = tds;
  serializeJson(payload, strPayload);
  mqttPubSub.publish(MQTT_WATER_QUALITY_TOPIC_STATE.c_str(), strPayload.c_str());
}
