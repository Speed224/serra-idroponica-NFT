//TODO CHECK WHEN MQTT IS OFFLINE

/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------Include Files----------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <AM2320_asukiaaa.h>




/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------TIME definitions-------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
#define PERIOD_SECOND_1         1000
#define PERIOD_SECOND_5         5000
#define PERIOD_SECOND_15        15000
#define PERIOD_MINUTE_1         60000
#define PERIOD_MINUTE_5         300000
#define PERIOD_MINUTE_15        900000
#define PERIOD_HOUR_1           3600000


#define RELAY_ON                LOW
#define RELAY_OFF               HIGH




/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------I/O Definitions--------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
const byte                LIGHT = 32;
const byte                WATER_PUMP = 33;
const byte                TDS = 35;
const byte                PH = 34;
const byte                AIR_PUMP = 19;
const byte                FLOAT_SENSOR = 17;
const byte                FAN = 16;
const byte                GENERAL_BUTTON = 5;

const byte                ONBOARD_LED = 2;

const String              LIGHT_NAME = "_light";
const String              LIGHT_RECOVER_NAME = "_light_recover";
const String              ISDAY_RECOVER_NAME = "_isday_recover";
const String              LIGHT_ON_NAME = "_light_on";
const String              LIGHT_OFF_NAME = "_light_off";
const String              WATER_PUMP_NAME = "_waterpump";
const String              AIR_PUMP_NAME = "_airpump";
const String              TEMPERATURE_NAME = "_temperature";
const String              HUMIDITY_NAME = "_humidity";
const String              TDS_NAME = "_tds";
const String              PH_NAME = "_ph";
const String              FLOAT_SENSOR_NAME = "_floatsensor";
const String              FAN_NAME = "_fan";
const String              SECURITY_SWITCH_NAME = "_securityswitch";
const String              SEND_DATA_PERIOD_NAME = "_senddataperiod";
const String              POLLING_PERIOD_NAME = "_pollingperiod";


 

/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------Configuration----------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
String              WIFI_SSID;
String              WIFI_PASSWORD;
String              MQTT_SERVER;                                              // MQTT Server IP
String              MQTT_USER;                                                // MQTT Server User Name
String              MQTT_PASSWORD;                                            // MQTT Server password
int                 MQTT_PORT = 1883;                                         // MQTT Server Port


const char*         DEVICE_MODEL = "esp32Serra";                              // Hardware Model
const char*         SOFTWARE_VERSION = "0.8";                                 // Firmware Version
const char*         MANUFACTURER = "Speed224";                                // Manufacturer Name
const String        DEVICE_NAME = "serra";                                    // Device Name
const String        MQTT_TOPIC_STATUS = "esp32iot/" + DEVICE_NAME;

const String        MQTT_TOPIC_HA_PREFIX = "homeassistant/";

const String        MQTT_TOPIC_DISCOVERY_SUFFIX = "/config";
const String        MQTT_TOPIC_STATE_SUFFIX = "/state";
const String        MQTT_TOPIC_COMMAND_SUFFIX = "/command";
const String        MQTT_TOPIC_OPTION_SUFFIX = "/option";

const String        MQTT_TOPIC_HA_SYNC = MQTT_TOPIC_HA_PREFIX + MQTT_TOPIC_COMMAND_SUFFIX.substring(1);



const String        MQTT_TOPIC_SERRA_SYNC = MQTT_TOPIC_HA_PREFIX + MQTT_TOPIC_STATUS + "/sync";
const String        MQTT_TOPIC_COMMAND = MQTT_TOPIC_HA_PREFIX + MQTT_TOPIC_STATUS + MQTT_TOPIC_COMMAND_SUFFIX;
const String        MQTT_TOPIC_HA_STATUS = MQTT_TOPIC_HA_PREFIX + "status";

//Sensor && Actuator Topics
const String        MQTT_LIGHT_TOPIC =  MQTT_TOPIC_HA_PREFIX + "light/" + MQTT_TOPIC_STATUS + LIGHT_NAME;
// WATER PUMP
const String        MQTT_WATER_PUMP_TOPIC =  MQTT_TOPIC_HA_PREFIX + "switch/" + MQTT_TOPIC_STATUS + WATER_PUMP_NAME;
// AIR PUMP
const String        MQTT_AIR_PUMP_TOPIC =  MQTT_TOPIC_HA_PREFIX + "switch/" + MQTT_TOPIC_STATUS + AIR_PUMP_NAME;
// FAN
const String        MQTT_FAN_TOPIC =  MQTT_TOPIC_HA_PREFIX + "switch/" + MQTT_TOPIC_STATUS + FAN_NAME;
// THERMOMETER
const String        MQTT_TEMPERATURE_TOPIC =  MQTT_TOPIC_HA_PREFIX + "sensor/" + MQTT_TOPIC_STATUS + TEMPERATURE_NAME;
const String        MQTT_HUMIDITY_TOPIC =  MQTT_TOPIC_HA_PREFIX + "sensor/" + MQTT_TOPIC_STATUS + HUMIDITY_NAME;
const String        MQTT_THERMOMETER_TOPIC_STATE =  MQTT_TOPIC_HA_PREFIX + "sensor/" + MQTT_TOPIC_STATUS + "_thermometer" + MQTT_TOPIC_STATE_SUFFIX;
// WATER QUALITY
const String        MQTT_TDS_TOPIC =  MQTT_TOPIC_HA_PREFIX + "sensor/" + MQTT_TOPIC_STATUS + TDS_NAME;
const String        MQTT_PH_TOPIC =  MQTT_TOPIC_HA_PREFIX + "sensor/" + MQTT_TOPIC_STATUS + PH_NAME;
const String        MQTT_WATER_QUALITY_TOPIC_STATE =  MQTT_TOPIC_HA_PREFIX + "sensor/" + MQTT_TOPIC_STATUS + "_water_quality" + MQTT_TOPIC_STATE_SUFFIX;
//WATER LEVEL
const String        MQTT_FLOAT_SENSOR_TOPIC =  MQTT_TOPIC_HA_PREFIX + "binary_sensor/" + MQTT_TOPIC_STATUS + FLOAT_SENSOR_NAME;
//OPTIONS STATE
const String        MQTT_SECURITY_SWITCH_TOPIC =  MQTT_TOPIC_HA_PREFIX + "switch/" + MQTT_TOPIC_STATUS + SECURITY_SWITCH_NAME;
const String        MQTT_LIGHT_RECOVER_TOPIC =  MQTT_TOPIC_HA_PREFIX + "number/" + MQTT_TOPIC_STATUS + LIGHT_RECOVER_NAME;
const String        MQTT_ISDAY_RECOVER_TOPIC =  MQTT_TOPIC_HA_PREFIX + "binary_sensor/" + MQTT_TOPIC_STATUS + ISDAY_RECOVER_NAME;
const String        MQTT_LIGHT_ON_TIME_TOPIC =  MQTT_TOPIC_HA_PREFIX + "number/" + MQTT_TOPIC_STATUS + LIGHT_ON_NAME;
const String        MQTT_LIGHT_OFF_TIME_TOPIC =  MQTT_TOPIC_HA_PREFIX + "number/" + MQTT_TOPIC_STATUS + LIGHT_OFF_NAME;
const String        MQTT_SEND_DATA_PERIOD_TOPIC =  MQTT_TOPIC_HA_PREFIX + "number/" + MQTT_TOPIC_STATUS + SEND_DATA_PERIOD_NAME;
const String        MQTT_POLLING_PERIOD_TOPIC =  MQTT_TOPIC_HA_PREFIX + "number/" + MQTT_TOPIC_STATUS + POLLING_PERIOD_NAME;






const int           topicsNumber = 12;
const String        topics[topicsNumber] = {
                                            MQTT_TOPIC_HA_STATUS, 
                                            MQTT_TOPIC_SERRA_SYNC, 
                                            MQTT_TOPIC_COMMAND, 
                                            (MQTT_LIGHT_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX), 
                                            (MQTT_WATER_PUMP_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX),
                                            (MQTT_AIR_PUMP_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX),
                                            (MQTT_FAN_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX),
                                            (MQTT_SECURITY_SWITCH_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX),
                                            (MQTT_LIGHT_ON_TIME_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX),
                                            (MQTT_LIGHT_OFF_TIME_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX),
                                            (MQTT_SEND_DATA_PERIOD_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX),
                                            (MQTT_POLLING_PERIOD_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX)
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


unsigned int        pollingDataPeriod = PERIOD_SECOND_1*4;        //DEFAULT wait time for sending data
unsigned int        sendDataPeriod = PERIOD_SECOND_5;

unsigned long       pollingPreviousMillis = 0;
unsigned long       dataPreviousMillis = 0;
unsigned long       wifiPreviousMillis = 0;
unsigned long       lightPreviousMillis = 0;
unsigned long       samplePreviousMillis = 0;
unsigned long       mqttLoopPreviousMillis = 0;
unsigned int        blinkPreviousMillis = 0;

unsigned long       currentMillis;

//unsigned int        lightONPeriod = PERIOD_HOUR_1*12;
unsigned int        lightONPeriod = 30000;
//unsigned int        lightOFFPeriod = PERIOD_HOUR_1*12;
unsigned int        lightOFFPeriod = 30000;

unsigned int        lightPeriodBeforeReset = 0;
unsigned int        lightPeriodPassed = 0;

bool                blink = true;
int                 blinkDelay = 500;

char                command;
bool                disableSecurity = false;

bool                isDay = true;
bool                lightState = true;
bool                waterPumpState = true;
bool                airPumpState = true;
bool                floatSensorState = true;
bool                fanState = false;

float               temperatureOffset = 2;
float               temperature;
float               humidity;
float               tds;
float               ph;

/*
* TDS && PH 
*/
const byte SCOUNT = 30;                 // sum of sample point
const float VREF = 3.3;                 //Volt sensor alimentation
const int analogResolution = 4096;

int tdsAnalogBuffer[SCOUNT];            // store the analog value in the array, read from ADC
int tdsAnalogBufferTemp[SCOUNT];
int tdsAnalogBufferIndex = 0;

int phAnalogBuffer[SCOUNT];             // store the analog value in the array, read from ADC
int phAnalogBufferTemp[SCOUNT];
int phAnalogBufferIndex = 0;


/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------INTERRUPT FUNCTIONS----------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/

// when float sensor fall
void IRAM_ATTR floatSensorISR(){
  if(!disableSecurity){
    waterPumpState = false;
    floatSensorState = false;
    sendMqttData = true;
  }
}


/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------SETUP------------------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
void setup() 
{
  Serial.begin(115200);
  Serial.println("Inizio setup");


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

  pinMode(GENERAL_BUTTON, INPUT_PULLDOWN);

  pinMode(FLOAT_SENSOR, INPUT_PULLUP);
  attachInterrupt(FLOAT_SENSOR, floatSensorISR, FALLING);

  Wire.begin();
  am2320.setWire(&Wire);


/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------CONNECTIONS------------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
  preferences.begin("credentials", true);

  WIFI_SSID = preferences.getString("SSID", ""); 
  WIFI_PASSWORD = preferences.getString("PASSWORD", "");
  MQTT_SERVER = preferences.getString("MQTT_SERVER", "");
  MQTT_USER = preferences.getString("MQTT_USER", ""); 
  MQTT_PASSWORD = preferences.getString("MQTT_PASSWORD", "");

  preferences.end();
  /*
  Serial.print("\nSSID: ");
  Serial.println(WIFI_SSID);
  Serial.print("\nPASSWORD: ");
  Serial.println(WIFI_PASSWORD);

  Serial.print("\nSERVER: ");
  Serial.println(MQTT_SERVER);
  Serial.print("\nMQTT_USER: ");
  Serial.println(MQTT_USER);
  Serial.print("\nMQTT_PASSWORD: ");
  Serial.println(MQTT_PASSWORD);
  */
  if (WIFI_SSID == "" || WIFI_PASSWORD == ""){
    Serial.println("No values saved for ssid or password");
  }
 
  delay(150);

  wifiSetup();

  //MQTT OPTIONS
  mqttPubSub.setServer(MQTT_SERVER.c_str(), MQTT_PORT);
  mqttPubSub.setCallback(mqttReceiverCallback);
  mqttPubSub.setBufferSize(600);

  mqttConnect();


  //printing device information
  Serial.println("");
  Serial.println("----------------------------------------------");
  Serial.print("MODEL: ");
  Serial.println(DEVICE_MODEL);
  Serial.print("DEVICE: ");
  Serial.println(DEVICE_NAME);
  Serial.print("SW Rev: ");
  Serial.println(SOFTWARE_VERSION);
  Serial.println("----------------------------------------------");

/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------LOGIC------------------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
  logics();
}

/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------LOOP-------------------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
void loop() 
{
  currentMillis = millis();                                           //

  if(currentMillis - mqttLoopPreviousMillis > 200){
    mqttPubSub.loop();                                                //call often (docs), if called too fast could occur MQTT issues
    mqttLoopPreviousMillis = currentMillis;                           //save current millis in previous for offsetting
  }

  
  if(currentMillis - pollingPreviousMillis > pollingDataPeriod || sendMqttData)       //sensor polling is splitted from INTERENET activities
  {
    Serial.println("Start Logics");
    logics();
    Serial.println("End Logics");

    lightPeriodPassed = currentMillis - lightPreviousMillis;

    pollingPreviousMillis = currentMillis;
  }

  
  //PH and TDS sampling for median algortihm
  if(currentMillis - samplePreviousMillis > sendDataPeriod/numberOfSamples){      //every 1/n of dataPeriod milliseconds,read the analog value from the ADC
      tdsAnalogBuffer[tdsAnalogBufferIndex] = analogRead(TDS);                    //read the analog value and store into the buffer
      tdsAnalogBufferIndex++;
      if(tdsAnalogBufferIndex == SCOUNT){ 
        tdsAnalogBufferIndex = 0;
      }

      phAnalogBuffer[phAnalogBufferIndex] = analogRead(PH);                       //read the analog value and store into the buffer
      phAnalogBufferIndex++;
      if(phAnalogBufferIndex == SCOUNT){ 
        phAnalogBufferIndex = 0;
      }

      samplePreviousMillis = currentMillis;
    }   

  //is called when sendDataPeriod time pass or sendMqttData is true, usefull for instant sensor refresh
  if((currentMillis - dataPreviousMillis > sendDataPeriod || sendMqttData) && !firstBoot)
  {
    //if mqtt is not connected try to connect else send data
    if(!mqttPubSub.connected()){
      //mqttConnect();
      if(WiFi.status() != WL_CONNECTED)
        Serial.println("Wifi not connected");
      else
        Serial.println("Mqtt not connected");
    }else{
      mqttStates();
    }
    sendMqttData = false;
    dataPreviousMillis = currentMillis;
  }
    
  //light cycle WIP
  if(isDay){
    if(currentMillis - lightPreviousMillis >= lightONPeriod - lightPeriodBeforeReset) {
      isDay = false;
      lightLogic(false);
      lightPeriodPassed = 0;
      lightPreviousMillis = currentMillis;

      if(!firstBoot)
        lightPeriodBeforeReset = 0;
    }
  }else{
    if(currentMillis - lightPreviousMillis >= lightOFFPeriod - lightPeriodBeforeReset) {
      isDay = true;
      lightLogic(true);
      lightPreviousMillis = currentMillis;
      lightPeriodPassed = 0;

      if(!firstBoot)
        lightPeriodBeforeReset = 0;
    }
  }

  // WIFI and MQTT connection onboard led FEEDBACK
  if(WiFi.status() != WL_CONNECTED){
      blinkDelay = 500;
    if (currentMillis - blinkPreviousMillis >= blinkDelay){
      digitalWrite(ONBOARD_LED, blink);
      blink = !blink;
      blinkPreviousMillis = currentMillis;
    }
  }else{
    if(!mqttPubSub.connected()){
      blinkDelay = 200;
      if (currentMillis - blinkPreviousMillis >= blinkDelay){
        digitalWrite(ONBOARD_LED, blink);
        blink = !blink;
        blinkPreviousMillis = currentMillis;
      }
    }else{
      if (currentMillis - blinkPreviousMillis >= blinkDelay){
        blinkDelay = 1000;
        digitalWrite(ONBOARD_LED, blink);
        blink = !blink;
        blinkPreviousMillis = currentMillis;
      }
    }
  }
  
  //every minutes check if there is wifi connection else try to reconnect
  if (currentMillis - wifiPreviousMillis >= PERIOD_MINUTE_1) {
    if(WiFi.status() != WL_CONNECTED){
      wifiSetup();
    }
    if(WiFi.status() == WL_CONNECTED){
      mqttConnect();
    }

    wifiPreviousMillis = currentMillis;
  }

  

  //read command from serial
  if(Serial.available())
    command = Serial.read();
  commandExecutor(command);  

  delay(1);
}


/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------ Public Functions -----------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------*/
void wifiSetup() 
{
  WiFi.disconnect();
  delay(100);
  WiFi.mode(WIFI_STA);        //define connection tipe

  int counter = 0;
  byte mac[6];
  delay(10);
  
                              //connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID.c_str(), WIFI_PASSWORD.c_str());

  WiFi.macAddress(mac);
  UNIQUE_ID = String(mac[0],HEX) +String(mac[1],HEX) +String(mac[2],HEX) +String(mac[3],HEX) + String(mac[4],HEX) + String(mac[5],HEX);

  Serial.print("Unique ID: ");
  Serial.println(UNIQUE_ID);    
  
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
    digitalWrite(ONBOARD_LED, HIGH);
    Serial.println("WiFi NOT connected!!!");
  }
}

void mqttConnect() {
  mqttPubSub.disconnect();

    if(!mqttPubSub.connected()){
      Serial.print("Attempting MQTT connection...");
      // attempt to connect
      if (mqttPubSub.connect(DEVICE_NAME.c_str(), MQTT_USER.c_str(), MQTT_PASSWORD.c_str())) {
        Serial.println("connected");
        // subscribe to topics
        mqttTopicsSubscribe(topics, topicsNumber);
        //only the first time is connected to MQTT send discovery info
        if (firstBoot){
          mqttHomeAssistantDiscovery();
          Serial.println("PUBLISH SYNC HA MESSAGE");
          mqttPubSub.publish(MQTT_TOPIC_HA_SYNC.c_str(), "syncserra");
        }
      }else {
        Serial.print("failed, rc=");
        Serial.print(mqttPubSub.state());
        Serial.println();
      }
    }
}

//subscribes to defined topics
void mqttTopicsSubscribe(const String topics[topicsNumber], const int topicsNumber){
  for(int i = 0; i < topicsNumber; i++){
    String success = (mqttPubSub.subscribe(topics[i].c_str())) ? "Subscribed" : "ERROR cannot subscribe";
    Serial.print(success);
    Serial.print(" to topic ");
    Serial.print(topics[i]);
    Serial.println();
  }
      
}



//Take command from serial and check if valid, used for testing
void commandExecutor(char command){
  switch (command) {
    case 'd':
      sendMqttData = true;
      Serial.println("Command: Sending Data");
    break;
    case 'w':
      wifiSetup();
    break;
    case 'm':
      mqttConnect();
    break;
    case 'M':
      mqttPubSub.disconnect();
    break;
    case 'R':
      ESP.restart();
    break;
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


//@param logicState set the state of light to this
void lightLogic(bool logicState){
  lightState = logicState;
  if(lightState){
    digitalWrite(LIGHT, RELAY_ON);
  }
  else{
    digitalWrite(LIGHT, RELAY_OFF);
  }
}

//@param logicState set the state off water pump to this
void waterPumpLogic(bool logicState){
  waterPumpState = logicState;
  if(waterPumpState){
    digitalWrite(WATER_PUMP, RELAY_ON);
  }
  else{
    digitalWrite(WATER_PUMP, RELAY_OFF);
  }

}

//@param logicState set the state of air pump to this
void airPumpLogic(bool logicState){
  airPumpState = logicState;
  if(airPumpState){
    digitalWrite(AIR_PUMP, RELAY_ON);
  }
  else{
    digitalWrite(AIR_PUMP, RELAY_OFF);
  }

}

//@param logicState set the state of fan to this
void fanLogic(bool logicState){
  fanState = logicState;
  if(fanState){
    digitalWrite(FAN, RELAY_ON);
  }
  else{
    digitalWrite(FAN, RELAY_OFF);
  }

}

//change sensor state depending on the value
void floatSensorLogic(){
  floatSensorState = digitalRead(FLOAT_SENSOR);
  if(!disableSecurity){
    if(!floatSensorState)
      waterPumpState = false;
  }
}

//not using official library because cannot read humidity with that
void am2320Logic(){
  if (am2320.update() != 0) {
    Serial.println("Error: Cannot update sensor values.");
  }else{
    //my sensor is a mess, the temperature is 2 degree higher
    temperature = am2320.temperatureC - temperatureOffset;
    humidity = am2320.humidity;
  }
}

//TODO not sure is nedded
//median algorith for better measurement
void tdsLogic(){
  float waterTemperature = 20;
  //MEDIAN ALGORITHM
  int bTab[SCOUNT];
  for (byte i = 0; i<SCOUNT; i++)
  bTab[i] = tdsAnalogBufferTemp[i];
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

  //TDS CALCS
  for(int copyIndex=0; copyIndex<SCOUNT; copyIndex++){
    tdsAnalogBufferTemp[copyIndex] = tdsAnalogBuffer[copyIndex];
    // read the analog value more stable by the median filtering algorithm, and convert to voltage value
    float averageVoltage = bTemp * (float)VREF / analogResolution;
    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0)); 
    float compensationCoefficient = 1.0+0.02*(waterTemperature-25.0);
    //temperature compensation
    float compensationVoltage = averageVoltage / compensationCoefficient;
    //convert voltage value to tds value
    //TODO look for this MAGIC NUMBERS
    tds = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage - 255.86 * compensationVoltage*  compensationVoltage + 857.39 * compensationVoltage) * 0.5;
  }
}

//TODO not sure is nedded

//median algorith for better measurement
void phLogic(){
  float calibration_value = 21.34;
  //MEDIAN ALGORITHM
  int bTab[SCOUNT];
  for (byte i = 0; i<SCOUNT; i++)
  bTab[i] = phAnalogBufferTemp[i];
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
  //PH CALCS
  for(int copyIndex=0; copyIndex<SCOUNT; copyIndex++){
    phAnalogBufferTemp[copyIndex] = phAnalogBuffer[copyIndex];

        //TODO look for this MAGIC NUMBERS
    float averageVoltage = bTemp * VREF / analogResolution / 6;     

    ph = -5.70 * averageVoltage + calibration_value;
  }
}


void logics(){
  if(!digitalRead(GENERAL_BUTTON)){
    Serial.println("General Button is OFF");
    lightLogic(false);
    floatSensorState = true;
    waterPumpLogic(false);
    airPumpLogic(false);
    fanLogic(false);
  }else{
    Serial.println("General Button is ON");
    lightLogic(lightState);
    floatSensorLogic();
    waterPumpLogic(waterPumpState);
    airPumpLogic(airPumpState);
    fanLogic(fanState);
    am2320Logic();
    tdsLogic();
    phLogic();
  }
  
}

void syncWithServer(){
  
}

//check the states and send to MQTT server
void mqttStates(){
  Serial.println("MQTT: Sending Data");
  
  //If the server is slow put small delay between messages
  mqttPubSub.publish((MQTT_LIGHT_TOPIC + MQTT_TOPIC_STATE_SUFFIX).c_str(), (lightState ? "ON" : "OFF"), true);
  delay(10);
  mqttPubSub.publish((MQTT_FLOAT_SENSOR_TOPIC + MQTT_TOPIC_STATE_SUFFIX).c_str(), (floatSensorState ? "ON" : "OFF"), true);
  mqttPubSub.publish((MQTT_WATER_PUMP_TOPIC + MQTT_TOPIC_STATE_SUFFIX).c_str(), (waterPumpState ? "ON" : "OFF"), true);
  mqttPubSub.publish((MQTT_AIR_PUMP_TOPIC + MQTT_TOPIC_STATE_SUFFIX).c_str(), (airPumpState ? "ON" : "OFF"), true);
  mqttPubSub.publish((MQTT_FAN_TOPIC + MQTT_TOPIC_STATE_SUFFIX).c_str(), (fanState ? "ON" : "OFF"), true);
  mqttPubSub.publish((MQTT_SECURITY_SWITCH_TOPIC + MQTT_TOPIC_STATE_SUFFIX).c_str(), (disableSecurity ? "ON" : "OFF"), true);
  delay(10);
  mqttPubSub.publish((MQTT_LIGHT_ON_TIME_TOPIC + MQTT_TOPIC_STATE_SUFFIX).c_str(), String(lightONPeriod/PERIOD_HOUR_1).c_str(), true);
  mqttPubSub.publish((MQTT_LIGHT_OFF_TIME_TOPIC + MQTT_TOPIC_STATE_SUFFIX).c_str(), String(lightOFFPeriod/PERIOD_HOUR_1).c_str(), true);
  mqttPubSub.publish((MQTT_SEND_DATA_PERIOD_TOPIC + MQTT_TOPIC_STATE_SUFFIX).c_str(), String(sendDataPeriod/PERIOD_SECOND_1).c_str(), true);
  mqttPubSub.publish((MQTT_POLLING_PERIOD_TOPIC + MQTT_TOPIC_STATE_SUFFIX).c_str(), String(pollingDataPeriod/PERIOD_SECOND_1).c_str(), true);
  delay(10);
  mqttPubSub.publish((MQTT_LIGHT_RECOVER_TOPIC + MQTT_TOPIC_STATE_SUFFIX).c_str(), String(lightPeriodPassed/PERIOD_HOUR_1).c_str(), true);
  mqttPubSub.publish((MQTT_ISDAY_RECOVER_TOPIC + MQTT_TOPIC_STATE_SUFFIX).c_str(), (isDay ? "ON" : "OFF"), true);


  //////////////
 // AM2320 ////
//////////////
  StaticJsonDocument<200> payload;
  String strPayload;
    
  payload["temperature"] = am2320.temperatureC - temperatureOffset;
  payload["humidity"] = am2320.humidity;
  serializeJson(payload, strPayload);
  mqttPubSub.publish(MQTT_THERMOMETER_TOPIC_STATE.c_str(), strPayload.c_str());
  ////////////////
 // TDS & PH ////
////////////////
  payload.clear();
  strPayload.clear();
  
  payload["tds"] = tds; //ppm
  //payload["ec"] = (tds * 2)/1000; //µS/cm
  payload["ph"] = ph;
  serializeJson(payload, strPayload);
  mqttPubSub.publish(MQTT_WATER_QUALITY_TOPIC_STATE.c_str(), strPayload.c_str());

}

bool stringToBool(String s){
  if(s == "on")
    return true;
  else
    return false;
  
}
        


//read the message from subscribed topic and do the logic
void mqttReceiverCallback(char* topic, byte* payload, unsigned int length){
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");

  byte state = 0;

  String message;

  StaticJsonDocument<600> json;
  
  for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
      message += (char)payload[i];
  }
  Serial.println();

  //HOME ASSISTANT STATUS
  if(String(topic) == String(MQTT_TOPIC_HA_STATUS)) 
  {
    //when homeassistant turn on send "online" message to MQTT
    //then send discovery to ensure if system is resetted hassio
    //know sensor again
    if(message == "online")
      mqttHomeAssistantDiscovery();
  }

  //SECURITY SWITCH
  if(String(topic) == String(MQTT_SECURITY_SWITCH_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX)){
    if(message == "OFF")
      disableSecurity = false;
    else
      disableSecurity = true;
    
    //refreshing states when finished
    mqttStates();
  }

  //SERRA COMMAND FROM MQTT
  if(String(topic) == String(MQTT_TOPIC_COMMAND)){
    Serial.println((char)payload[0]);
    commandExecutor((char)payload[0]);
  }

  //SYNC FUNCTION
  if(String(topic) == String(MQTT_TOPIC_SERRA_SYNC)){
    deserializeJson(json, payload);

    airPumpState = stringToBool(json["airpump"]);
    waterPumpState = stringToBool(json["waterpump"]);
    fanState = stringToBool(json["fan"]);
    lightState = stringToBool(json["light"]);
    isDay = stringToBool(json["isday"]);
    lightPeriodBeforeReset = (int)json["light_recover"] * PERIOD_HOUR_1;
    lightONPeriod = (int)json["light_on"] * PERIOD_HOUR_1;
    lightOFFPeriod = (int)json["light_off"] * PERIOD_HOUR_1;
    sendDataPeriod = (int)json["sending"] * PERIOD_SECOND_1;
    pollingDataPeriod = (int)json["polling"] * PERIOD_SECOND_1;


    if(lightONPeriod <= 0)
      lightONPeriod = 1 * PERIOD_HOUR_1;
    if(lightOFFPeriod <= 0)
      lightOFFPeriod = 1 * PERIOD_HOUR_1;
    if(sendDataPeriod < 5)
      sendDataPeriod = 5 * PERIOD_SECOND_1;
    if(pollingDataPeriod < 4)
      pollingDataPeriod = 4 * PERIOD_SECOND_1;
    
    
    logics();
    mqttStates();
    firstBoot = false;
  }

  //LIGHT ON TIME FROM MQTT MULTIPLIED PER PERIOD_HOUR_1
  if(String(topic) == String((MQTT_LIGHT_ON_TIME_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX))){
    int num = 1;
    if(message.toInt() > 0)
      num = message.toInt();

    lightONPeriod = num * PERIOD_HOUR_1;
    mqttStates();
  }

  //LIGHT OFF TIME FROM MQTT MULTIPLIED PER PERIOD_HOUR_1
  if(String(topic) == String((MQTT_LIGHT_OFF_TIME_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX))){
    int num = 1;
    if(message.toInt() > 0)
      num = message.toInt();

    lightOFFPeriod = num * PERIOD_HOUR_1;
    mqttStates();
  }

  //TIME SENDING MQTT DATA
  if(String(topic) == String((MQTT_SEND_DATA_PERIOD_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX))){
    int num = 5;
    if(message.toInt() > 5) //DEFAULT MIN SECONDS
      num = message.toInt();

    sendDataPeriod = num * PERIOD_SECOND_1;
    mqttStates();
  }

  //TIME POLLING DATA
  if(String(topic) == String((MQTT_POLLING_PERIOD_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX))){
    int num = 4;
    if(message.toInt() > 4) //DEFAULT MIN SECONDS
      num = message.toInt();

    pollingDataPeriod = num * PERIOD_SECOND_1;
    mqttStates();
  }
   

  //LIGHT COMMAND
  if(String(topic) == String((MQTT_LIGHT_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX))) 
  {
    if(disableSecurity){
      if(message == "ON")
        lightLogic(true);
      else
        lightLogic(false);
    }
    mqttStates();
  }

  //WATER PUMP COMMAND
  if(String(topic) == String((MQTT_WATER_PUMP_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX))) 
  {
    //if there is water or security is true
    if(disableSecurity){
      if(message == "ON")
        waterPumpLogic(true);
      else
        waterPumpLogic(false);
    }
    mqttStates();
  }

  //AIR PUMP COMMAND
  if(String(topic) == String((MQTT_AIR_PUMP_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX))) 
  {
    if(message == "ON")
      airPumpLogic(true);
    else
      airPumpLogic(false);
    
    mqttStates();
  }
  
  //FAN COMMAND
  if(String(topic) == String((MQTT_FAN_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX))) 
  {
    if(message == "ON")
      fanLogic(true);
    else
      fanLogic(false);
    
    mqttStates();
  }

}

//Discovery information for HOMEASSISTANT MQTT integration
void mqttHomeAssistantDiscovery()
{
  String discoveryTopic;
  String stateTopic;
  String commandTopic;
  String name;
  String topic;
  
  String payload;
  String strPayload;

  int sendDelay = 100;

  if(mqttPubSub.connected())
  {
    Serial.println("SEND HOME ASSISTANT DISCOVERY!!!");

    StaticJsonDocument<600> payload;
    JsonObject device;
    JsonArray identifiers;
    
    

  /////////////
 // LIGHT ////
/////////////
    delay(sendDelay);

    discoveryTopic = MQTT_LIGHT_TOPIC + MQTT_TOPIC_DISCOVERY_SUFFIX;

    payload["name"] = DEVICE_NAME + LIGHT_NAME;
    payload["unique_id"] = UNIQUE_ID + LIGHT_NAME;
    payload["state_topic"] = (MQTT_LIGHT_TOPIC + MQTT_TOPIC_STATE_SUFFIX);
    payload["command_topic"] = (MQTT_LIGHT_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX);

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

    device.clear();
    identifiers.clear();

  /////////////////////
 // LIGHT RECOVER ////
/////////////////////
    delay(sendDelay);
    
    payload.clear();
    strPayload.clear();

    discoveryTopic = MQTT_LIGHT_RECOVER_TOPIC + MQTT_TOPIC_DISCOVERY_SUFFIX;

    payload["name"] = DEVICE_NAME + LIGHT_RECOVER_NAME;
    payload["unique_id"] = UNIQUE_ID + LIGHT_RECOVER_NAME;
    payload["state_topic"] = (MQTT_LIGHT_RECOVER_TOPIC + MQTT_TOPIC_STATE_SUFFIX);
    payload["command_topic"] = (MQTT_LIGHT_RECOVER_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX);

    serializeJsonPretty(payload, Serial);
    Serial.println(" ");
    serializeJson(payload, strPayload);

    mqttPubSub.publish(discoveryTopic.c_str(), strPayload.c_str());
  /////////////////////
 // ISDAY RECOVER ////
/////////////////////
    delay(sendDelay);
    
    payload.clear();
    strPayload.clear();

    discoveryTopic = MQTT_ISDAY_RECOVER_TOPIC + MQTT_TOPIC_DISCOVERY_SUFFIX;

    payload["name"] = DEVICE_NAME + ISDAY_RECOVER_NAME;
    payload["unique_id"] = UNIQUE_ID + ISDAY_RECOVER_NAME;
    payload["state_topic"] = (MQTT_ISDAY_RECOVER_TOPIC + MQTT_TOPIC_STATE_SUFFIX);



    serializeJsonPretty(payload, Serial);
    Serial.println(" ");
    serializeJson(payload, strPayload);

    mqttPubSub.publish(discoveryTopic.c_str(), strPayload.c_str());
  ////////////////
 // LIGHT ON ////
////////////////
    delay(sendDelay);
    
    payload.clear();
    strPayload.clear();

    discoveryTopic = MQTT_LIGHT_ON_TIME_TOPIC + MQTT_TOPIC_DISCOVERY_SUFFIX;

    payload["name"] = DEVICE_NAME + LIGHT_ON_NAME;
    payload["unique_id"] = UNIQUE_ID + LIGHT_ON_NAME;
    payload["state_topic"] = (MQTT_LIGHT_ON_TIME_TOPIC + MQTT_TOPIC_STATE_SUFFIX);
    payload["command_topic"] = (MQTT_LIGHT_ON_TIME_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX);

    serializeJsonPretty(payload, Serial);
    Serial.println(" ");
    serializeJson(payload, strPayload);

    mqttPubSub.publish(discoveryTopic.c_str(), strPayload.c_str());

  /////////////////
 // LIGHT OFF ////
/////////////////
    delay(sendDelay);
    
    payload.clear();
    strPayload.clear();

    discoveryTopic = MQTT_LIGHT_OFF_TIME_TOPIC + MQTT_TOPIC_DISCOVERY_SUFFIX;

    payload["name"] = DEVICE_NAME + LIGHT_OFF_NAME;
    payload["unique_id"] = UNIQUE_ID + LIGHT_OFF_NAME;
    payload["state_topic"] = (MQTT_LIGHT_OFF_TIME_TOPIC + MQTT_TOPIC_STATE_SUFFIX);
    payload["command_topic"] = (MQTT_LIGHT_OFF_TIME_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX);

    serializeJsonPretty(payload, Serial);
    Serial.println(" ");
    serializeJson(payload, strPayload);

    mqttPubSub.publish(discoveryTopic.c_str(), strPayload.c_str());

  //////////////////
 // WATER PUMP ////
//////////////////
    delay(sendDelay);
    
    payload.clear();
    strPayload.clear();

    discoveryTopic = MQTT_WATER_PUMP_TOPIC + MQTT_TOPIC_DISCOVERY_SUFFIX;
    
    payload["name"] = DEVICE_NAME + WATER_PUMP_NAME;
    payload["unique_id"] = UNIQUE_ID + WATER_PUMP_NAME;
    payload["state_topic"] = (MQTT_WATER_PUMP_TOPIC + MQTT_TOPIC_STATE_SUFFIX);
    payload["command_topic"] = (MQTT_WATER_PUMP_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX);
    payload["device_class"] = "switch";

    serializeJsonPretty(payload, Serial);
    Serial.println(" ");
    serializeJson(payload, strPayload);

    mqttPubSub.publish(discoveryTopic.c_str(), strPayload.c_str());

  ////////////////
 // AIR PUMP ////
////////////////
    delay(sendDelay);

    payload.clear();
    strPayload.clear();

    discoveryTopic = MQTT_AIR_PUMP_TOPIC + MQTT_TOPIC_DISCOVERY_SUFFIX;
    payload["name"] = DEVICE_NAME + AIR_PUMP_NAME;
    payload["unique_id"] = UNIQUE_ID + AIR_PUMP_NAME;
    payload["state_topic"] = (MQTT_AIR_PUMP_TOPIC + MQTT_TOPIC_STATE_SUFFIX);
    payload["command_topic"] = (MQTT_AIR_PUMP_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX);
    payload["device_class"] = "switch";

    serializeJsonPretty(payload, Serial);
    Serial.println(" ");
    serializeJson(payload, strPayload);

    mqttPubSub.publish(discoveryTopic.c_str(), strPayload.c_str());

  ///////////
 // FAN ////
///////////
    delay(sendDelay);

    payload.clear();
    strPayload.clear();

    discoveryTopic = MQTT_FAN_TOPIC + MQTT_TOPIC_DISCOVERY_SUFFIX;
    payload["name"] = DEVICE_NAME + FAN_NAME;
    payload["unique_id"] = UNIQUE_ID + FAN_NAME;
    payload["state_topic"] = (MQTT_FAN_TOPIC + MQTT_TOPIC_STATE_SUFFIX);
    payload["command_topic"] = (MQTT_FAN_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX);
    payload["device_class"] = "switch";

    serializeJsonPretty(payload, Serial);
    Serial.println(" ");
    serializeJson(payload, strPayload);

    mqttPubSub.publish(discoveryTopic.c_str(), strPayload.c_str());

  ///////////////////
 // TEMPERATURE ////
///////////////////
    delay(sendDelay);

    payload.clear();
    strPayload.clear();

    discoveryTopic = MQTT_TEMPERATURE_TOPIC + MQTT_TOPIC_DISCOVERY_SUFFIX;
    
    payload["name"] = DEVICE_NAME + TEMPERATURE_NAME;
    payload["unique_id"] = UNIQUE_ID + TEMPERATURE_NAME;
    payload["state_topic"] = MQTT_THERMOMETER_TOPIC_STATE;
    payload["value_template"] = "{{ value_json.temperature | is_defined}}";
    payload["device_class"] = "temperature";
    payload["unit_of_measurement"] = "°C";
    payload["suggested_display_precision"] = 1;

    serializeJsonPretty(payload, Serial);
    Serial.println(" ");
    serializeJson(payload, strPayload);

    mqttPubSub.publish(discoveryTopic.c_str(), strPayload.c_str());

  ////////////////
 // HUMIDITY ////
////////////////
    delay(sendDelay);

    payload.clear();
    strPayload.clear();

    discoveryTopic = MQTT_HUMIDITY_TOPIC + MQTT_TOPIC_DISCOVERY_SUFFIX;
    payload["name"] = DEVICE_NAME + HUMIDITY_NAME;
    payload["unique_id"] = UNIQUE_ID + HUMIDITY_NAME;
    payload["state_topic"] = MQTT_THERMOMETER_TOPIC_STATE;

    payload["value_template"] = "{{ value_json.humidity | is_defined }}";
    payload["device_class"] = "humidity";
    payload["unit_of_measurement"] = "%";

    serializeJsonPretty(payload, Serial);
    Serial.println(" ");
    serializeJson(payload, strPayload);

    mqttPubSub.publish(discoveryTopic.c_str(), strPayload.c_str());

  ///////////
 // TDS ////
///////////
    delay(sendDelay);

    payload.clear();
    strPayload.clear();

    discoveryTopic = MQTT_TDS_TOPIC + MQTT_TOPIC_DISCOVERY_SUFFIX;
    
    payload["name"] = DEVICE_NAME + TDS_NAME;
    payload["unique_id"] = UNIQUE_ID + TDS_NAME;
    payload["state_topic"] = MQTT_WATER_QUALITY_TOPIC_STATE;
    payload["value_template"] = "{{ value_json.tds | is_defined }}";
    payload["unit_of_measurement"] = "ppm";
    payload["suggested_display_precision"] = 1;

    serializeJsonPretty(payload, Serial);
    Serial.println(" ");
    serializeJson(payload, strPayload);

    mqttPubSub.publish(discoveryTopic.c_str(), strPayload.c_str());

  //////////
 // PH ////
//////////
    delay(sendDelay);

    payload.clear();
    strPayload.clear();

    discoveryTopic = MQTT_PH_TOPIC + MQTT_TOPIC_DISCOVERY_SUFFIX;
    
    payload["name"] = DEVICE_NAME + PH_NAME;
    payload["unique_id"] = UNIQUE_ID + PH_NAME;
    payload["state_topic"] = MQTT_WATER_QUALITY_TOPIC_STATE;
    payload["value_template"] = "{{ value_json.ph | is_defined }}";
    payload["unit_of_measurement"] = "ph";
    payload["suggested_display_precision"] = 1;

    serializeJsonPretty(payload, Serial);
    Serial.println(" ");
    serializeJson(payload, strPayload);

    mqttPubSub.publish(discoveryTopic.c_str(), strPayload.c_str());

  ////////////////////
 // FLOAT SENSOR ////
////////////////////
    delay(sendDelay);

    payload.clear();
    strPayload.clear();

    discoveryTopic = MQTT_FLOAT_SENSOR_TOPIC + MQTT_TOPIC_DISCOVERY_SUFFIX;
    
    payload["name"] = DEVICE_NAME + FLOAT_SENSOR_NAME;
    payload["unique_id"] = UNIQUE_ID + FLOAT_SENSOR_NAME;
    payload["state_topic"] = (MQTT_FLOAT_SENSOR_TOPIC + MQTT_TOPIC_STATE_SUFFIX);

    serializeJsonPretty(payload, Serial);
    Serial.println(" ");
    serializeJson(payload, strPayload);

    mqttPubSub.publish(discoveryTopic.c_str(), strPayload.c_str());

  ///////////////////////
 // SECURITY SWITCH ////
///////////////////////
    delay(sendDelay);

    payload.clear();
    strPayload.clear();

    discoveryTopic = MQTT_SECURITY_SWITCH_TOPIC + MQTT_TOPIC_DISCOVERY_SUFFIX;

    payload["name"] = DEVICE_NAME + SECURITY_SWITCH_NAME;
    payload["unique_id"] = UNIQUE_ID + SECURITY_SWITCH_NAME;
    payload["state_topic"] = (MQTT_SECURITY_SWITCH_TOPIC + MQTT_TOPIC_STATE_SUFFIX);
    payload["command_topic"] = (MQTT_SECURITY_SWITCH_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX);
    payload["device_class"] = "switch";


    serializeJsonPretty(payload, Serial);
    Serial.println(" ");
    serializeJson(payload, strPayload);

    mqttPubSub.publish(discoveryTopic.c_str(), strPayload.c_str());

  ///////////////////////////
 // SENDING DATA PERIOD ////
///////////////////////////
    delay(sendDelay);
    
    payload.clear();
    strPayload.clear();

    discoveryTopic = MQTT_SEND_DATA_PERIOD_TOPIC + MQTT_TOPIC_DISCOVERY_SUFFIX;

    payload["name"] = DEVICE_NAME + SEND_DATA_PERIOD_NAME;
    payload["unique_id"] = UNIQUE_ID + SEND_DATA_PERIOD_NAME;
    payload["state_topic"] = (MQTT_SEND_DATA_PERIOD_TOPIC + MQTT_TOPIC_STATE_SUFFIX);
    payload["command_topic"] = (MQTT_SEND_DATA_PERIOD_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX);

    serializeJsonPretty(payload, Serial);
    Serial.println(" ");
    serializeJson(payload, strPayload);

    mqttPubSub.publish(discoveryTopic.c_str(), strPayload.c_str());

  ///////////////////////////
 // POLLING DATA PERIOD ////
///////////////////////////
    delay(sendDelay);
    
    payload.clear();
    strPayload.clear();

    discoveryTopic = MQTT_POLLING_PERIOD_TOPIC + MQTT_TOPIC_DISCOVERY_SUFFIX;

    payload["name"] = DEVICE_NAME + POLLING_PERIOD_NAME;
    payload["unique_id"] = UNIQUE_ID + POLLING_PERIOD_NAME;
    payload["state_topic"] = (MQTT_POLLING_PERIOD_TOPIC + MQTT_TOPIC_STATE_SUFFIX);
    payload["command_topic"] = (MQTT_POLLING_PERIOD_TOPIC + MQTT_TOPIC_COMMAND_SUFFIX);

    serializeJsonPretty(payload, Serial);
    Serial.println(" ");
    serializeJson(payload, strPayload);

    mqttPubSub.publish(discoveryTopic.c_str(), strPayload.c_str());


  }


}
