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
            

/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------Public variables-------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
WiFiClient          WiFiClient;
PubSubClient        mqttPubSub(WiFiClient);
Preferences         preferences;


int                 count = 0;
int                 mqttCounterConn = 0;
bool                InitSystem = true;
String              UNIQUE_ID;
bool                SendMqttData = false;

unsigned long       dataPreviousMillis = 0;
unsigned long       wifiPreviousMillis = 0;
unsigned long       currentMillis;

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

  mqttPubSub.setBufferSize(600);

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
  mqttPubSub.setCallback(MqttReceiverCallback);



}

/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------LOOP-------------------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
void loop() 
{
  currentMillis = millis();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // MQTT Connection
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if(WiFi.status() == WL_CONNECTED)
    {
        if(!mqttPubSub.connected())
            MqttReconnect();
        else
            mqttPubSub.loop();
    }else{
      delay(10000);
      Serial.println("Retry");
      wifiSetup();
    }

    if(InitSystem)
    {
        delay(100);
        InitSystem = false;
        Serial.println("INIT SYSTEM...");
        MqttHomeAssistantDiscovery();     // Send Discovery Data
    }

    if(currentMillis - dataPreviousMillis > PERIOD_SECOND_1)  // Every 1000 [msec]
    {
        dataPreviousMillis = currentMillis;
       
        
        if(count++ == 20 || SendMqttData) // Every 10 [sec] or if input status changed
        {
            count = 0;

            
            SendMqttData = true;
        }

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // SEND MQTT DATA
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  
        if(SendMqttData == true)        
        {
            StaticJsonDocument<200> payload;  
            //payload["temp"] = Temperature;
            //payload["hum"] = Humidity;
            //payload["inputstatus"] = strDoorStatus;
            payload["pot"] = analogRead(POT);

            String strPayload;
            serializeJson(payload, strPayload);

            if(mqttPubSub.connected())
            {
              String name = "_pot";
              String stateTopic = MQTT_TOPIC_HA_PREFIX + "sensor/" + MQTT_TOPIC_STATUS + name + MQTT_TOPIC_STATE_SUFFIX;
              mqttPubSub.publish(stateTopic.c_str(), strPayload.c_str()); 
              Serial.println("MQTT: Send Data!!!");
              Serial.println(" ");
              Serial.println(" ");
              SendMqttData = false;
          }
        }
    }


  
  
  if (currentMillis - wifiPreviousMillis >= PERIOD_MINUTE_15) {
    if(WiFi.status() != WL_CONNECTED)
      wifiSetup();

    wifiPreviousMillis = currentMillis;
  }

}


/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------ Public Functions -----------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
void wifiSetup() 
{
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

void MqttReconnect() 
{
    // Loop until we're reconnected
    while (!mqttPubSub.connected()  && (mqttCounterConn++ < 4))
    {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (mqttPubSub.connect(DEVICE_NAME.c_str(), MQTT_USER.c_str(), MQTT_PASSWORD.c_str())) 
        {
            Serial.println("connected");
            // Subscribe
            mqttPubSub.subscribe("homeassistant/status");
            delay(100);
        } else 
        {
            Serial.print("failed, rc=");
            Serial.print(mqttPubSub.state());
            Serial.println(" try again in 1 seconds");
            delay(1000);
        }
    }  
    mqttCounterConn = 0;
}

void MqttHomeAssistantDiscovery()
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

void MqttReceiverCallback(char* topic, byte* payload, unsigned int length) 
{
    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(". Message: ");
    byte state = 0;
    String messageTemp;
    
    for (int i = 0; i < length; i++) 
    {
        Serial.print((char)payload[i]);
        messageTemp += (char)payload[i];
    }
    Serial.println();
  
    if(String(topic) == String("homeassistant/status")) 
    {
        if(messageTemp == "online")
            MqttHomeAssistantDiscovery();
    }
}
