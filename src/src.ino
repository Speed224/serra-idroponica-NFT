/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------Include Files----------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>




/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------Local definitions------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
#define PERIOD_MILLSEC_250      250
#define PERIOD_MILLSEC_500      500
#define PERIOD_SECOND_1         1000
#define PERIOD_SECOND_5         5000
#define PERIOD_SECOND_10        10000
#define PERIOD_SECOND_15        15000
#define PERIOD_MINUTE_1         60000
#define PERIOD_MINUTE_5         300000
#define PERIOD_MINUTE_10        600000
#define PERIOD_MINUTE_15        900000
#define PERIOD_HOUR_1           3600000




/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------I/O Definitions--------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
const int POT = 35; 

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
const String        MQTT_TOPIC_STATE_SUFFIX = "/state";                       // MQTT Topic   
const String        MQTT_TOPIC_HA_PREFIX = "homeassistant/";
const String        MQTT_TOPIC_DISCOVERY_SUFFIX = "/config";

const String        MQTT_TOPIC_OPTION = MQTT_TOPIC_HA_PREFIX + MQTT_TOPIC_STATUS + "/option";
const String        MQTT_TOPIC_COMMAND = MQTT_TOPIC_HA_PREFIX + MQTT_TOPIC_STATUS + "/command";
const String        MQTT_TOPIC_HA_STATUS = MQTT_TOPIC_HA_PREFIX + "status";

const int           topicsNumber = 3;
const String        topics[topicsNumber] = {MQTT_TOPIC_HA_STATUS, MQTT_TOPIC_OPTION, MQTT_TOPIC_COMMAND};


/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------Public variables-------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
WiFiClient          wifiClient;
PubSubClient        mqttPubSub(wifiClient);
Preferences         preferences;


int                 count = 0;
String              UNIQUE_ID;
bool                sendMqttData = false;

//DEFAULT wait time for sending data
unsigned int        sendDataPeriod = PERIOD_SECOND_5;

unsigned long       dataPreviousMillis = 0;
unsigned long       wifiPreviousMillis = 0;
unsigned long       currentMillis;

char                command;


String              POT_NAME = "pot";

/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------SETUP------------------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
void setup() 
{
  Serial.begin(115200);
  Serial.println("Inizio setup");

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

  

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Input output Pin definitions
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  pinMode(POT, INPUT);



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
     
     /*
      StaticJsonDocument<200> payload;  
      //payload["temp"] = Temperature;
      //payload["hum"] = Humidity;
      //payload["inputstatus"] = strDoorStatus;
      payload["pot"] = analogRead(POT);

      String strPayload;
      serializeJson(payload, strPayload);

      String name = "_pot";
      String potStateTopic = MQTT_TOPIC_HA_PREFIX + "sensor/" + MQTT_TOPIC_STATUS + name + MQTT_TOPIC_STATE_SUFFIX;
      bool err = mqttPubSub.publish(potStateTopic.c_str(), strPayload.c_str());

      if(!err)
        Serial.println("MQTT: ERROR cannot send data");
      else
        Serial.println("MQTT: Data SENT");
      */

      bool error = mqttSendData("pot", (String)analogRead(POT));
      if(!error)
        Serial.println("MQTT: ERROR cannot send data");
      else
        Serial.println("MQTT: Sending Data");

      sendMqttData = false;
      
    }
  }

  
  
  if (currentMillis - wifiPreviousMillis >= PERIOD_MINUTE_15) {
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
        mqttHomeAssistantDiscovery();
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
    String name;
    String topic;
    
    String payload;
    String strPayload;

    

    if(mqttPubSub.connected())
    {
        Serial.println("SEND HOME ASSISTANT DISCOVERY!!!");
        StaticJsonDocument<600> payload;
        JsonObject device;
        JsonArray identifiers;

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // POT
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //discoveryTopic = "homeassistant/sensor/esp32iotsensor/" + DEVICE_NAME + "_temp" + "/config";
        name = "_pot";
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

    if(String(topic) == String(MQTT_TOPIC_OPTION)) 
    {
      deserializeJson(json, payload);
      sendDataPeriod = json["sendDataPeriod"];
      Serial.print("sendDataPeriod: ");
      Serial.println(sendDataPeriod);
    }

    Serial.println(MQTT_TOPIC_COMMAND);
    if(String(topic) == String(MQTT_TOPIC_COMMAND)){
      Serial.println((char)payload[0]);
      commandExecutor((char)payload[0]);
    }

    if(String(topic) == String(MQTT_TOPIC_HA_STATUS)) 
    {
        if(message == "online")
            mqttHomeAssistantDiscovery();
    }
}
