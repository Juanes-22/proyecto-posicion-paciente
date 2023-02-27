//-------------------------------------------------------------------------------
//  TinyZero MQTT Ubidots Patient Position Detector
//  Last Updated 26 Aug 2022
//
//  This sketch uses MQTT to publish data to Ubidots. Data is an integer
//  that represents the current patient position (i.e. a discrete state).
//  Also, a description of the state is sent in the context key of the 
//  payload. 
//  This data will later be presented as a Widget in a Ubidots Dashboard.
//
//  Written by Juan García
//-------------------------------------------------------------------------------

#include <PubSubClient.h>
#include <WiFi101.h>
#include <stdio.h>

#if defined (ARDUINO_ARCH_AVR)
#define SerialMonitorInterface Serial
#elif defined(ARDUINO_ARCH_SAMD)
#define SerialMonitorInterface SerialUSB
#endif

/* WiFi Credentials */
#define WIFI_SSID ""                          // Put here your WiFi SSID
#define WIFI_PASSWORD ""                      // Put here your WiFi password

/* Ubidots parameters */
#define UBIDOTS_TOKEN ""                                      // Put here your Ubidots TOKEN
#define VARIABLE_LABEL "patient_position"                     // Put here your Variable label to which data  will be published
#define DEVICE_LABEL "tinyzero-position-detector"             // Put here your Device label to which data  will be published

/* MQTT parameters */
#define MQTT_PUBLISH_INTERVAL_MS    500     // MQTT publish rate in milliseconds
#define MQTT_DEBUG                  1       // For MQTT debugging purposes

/* Buttons parameters */
#define BUTTONS_DEBUG               0       // For debugging buttons state

/* Pin definitions */
const uint8_t leftSidePin = 1;
const uint8_t rightSidePin = 5;
const uint8_t backPin0 = 3;
const uint8_t backPin1 = 6;
const uint8_t chestPin = 9;

/* Global variables to store state of group of buttons */
uint8_t chestState = 0;       // State of group of buttons placed on the chest
uint8_t backState = 0;        // State of group of buttons placed on the back
uint8_t leftSideState = 0;    // State of group of buttons placed on the left side
uint8_t rightSideState = 0;   // State of group of buttons placed on the right side

/* Define enumerated type for state machine states */
enum States
{
  UPDATE_PATIENT_POSITION,
  PUBLISH_DATA
};

enum PatientStates
{
  NONE,
  LYING_FACEUP,
  LYING_FACEDOWN,
  LYING_RIGHTSIDE,
  LYING_LEFTSIDE
};

States state;                         // Global variable to store current program state
PatientStates patient_state;          // Global variable to store current patient state

unsigned int start_state_millis = 0;  // Global variable to store start millis after a state change
unsigned int elapsed_seconds;         // Global variable to store elapsed seconds after a state change

/* MQTT variables */
char mqttBroker[] = "industrial.api.ubidots.com ";
char payload[700];
char topic[150];
char str_description[20];
unsigned int publish_timer;

/* Creating WiFi and MQTT client instances */
WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);

/****************************************
   Main Functions
 ****************************************/

void setup() {
  /* Initialize button pins as INPUTs */
  pinMode(leftSidePin, INPUT_PULLUP);
  pinMode(rightSidePin, INPUT_PULLUP);
  pinMode(backPin0, INPUT_PULLUP);
  pinMode(backPin1, INPUT_PULLUP);
  pinMode(chestPin, INPUT_PULLUP);

  SerialMonitorInterface.begin(9600);         // Initialize serial communication
  while (!SerialMonitorInterface);
  WiFi.setPins(8, 2, A3, -1);                 // Very important for Tinyduino

  connectToWiFi(WIFI_SSID, WIFI_PASSWORD);    // Connect board to WiFi

  mqtt_client.setServer(mqttBroker, 1883);    // Connect to Ubidots MQTT broker

  state = UPDATE_PATIENT_POSITION;            // Initial program state
  patient_state = NONE;                       // Initial patient state

  publish_timer = millis();
}

void loop() {
  switch (state)
  {   
    case UPDATE_PATIENT_POSITION:

      unsigned int current_state_time;
      uint8_t previous_state;

      previous_state = patient_state;
      
      SM_PatientPosition();

      if (patient_state != previous_state) {
        start_state_millis = millis();
      }

      current_state_time = millis() - start_state_millis;
      elapsed_seconds = (int) current_state_time / 1000;

      if ((millis() - publish_timer) > MQTT_PUBLISH_INTERVAL_MS && patient_state != NONE) {
        state = PUBLISH_DATA;
      }

      break;

    case PUBLISH_DATA:
      if (!mqtt_client.connected())
      {
        reconnect();

        // Subscribes to get the value of the patient_state variable in the TinyZero device
        char topicToSubscribe[200];

        sprintf(topicToSubscribe, "%s", "");
        sprintf(topicToSubscribe, "%s%s", "/v1.6/devices/", DEVICE_LABEL);
        sprintf(topicToSubscribe, "%s/%s", topicToSubscribe, VARIABLE_LABEL);

        mqtt_client.subscribe(topicToSubscribe);
      }

      // Publish patient state data
      publishPatientState();

      // This should be called regularly to allow the client to process incoming messages and maintain its connection to the server
      mqtt_client.loop();

      state = UPDATE_PATIENT_POSITION;

      publish_timer = millis();

      break;
  }
  delay(200);
}

/****************************************
   Auxiliar Functions
 ****************************************/

void SM_PatientPosition() {

  updatePatientState(); 

  switch (patient_state)
  {
    case NONE:
      break;
      
    case LYING_FACEUP:
      strcpy(str_description, "LYING_FACEUP");
      break;
      
    case LYING_FACEDOWN:         
      strcpy(str_description, "LYING_FACEDOWN");
      break;
      
    case LYING_RIGHTSIDE:          
      strcpy(str_description, "LYING_RIGHTSIDE");
      break;
      
    case LYING_LEFTSIDE:
      strcpy(str_description, "LYING_LEFTSIDE");
      break;
  }
}

void updatePatientState()
{
  readButtons();
  
  if (backState >= 1)
  {
    patient_state = LYING_FACEUP;
  }
  else if (chestState >= 1)
  {
    patient_state = LYING_FACEDOWN;
  }
  else if (rightSideState >= 1)
  {
    patient_state = LYING_RIGHTSIDE;
  }
  else if (leftSideState >= 1)
  {
    patient_state = LYING_LEFTSIDE;
  }
  else
  {
    patient_state = NONE;
  }
}

void readButtons()
{
  uint8_t backState0 = !digitalRead(backPin0);
  uint8_t backState1 = !digitalRead(backPin1);

  backState = backState0 + backState1;
  chestState = !digitalRead(chestPin);
  leftSideState = !digitalRead(leftSidePin);
  rightSideState = !digitalRead(rightSidePin);

#if BUTTONS_DEBUG
  SerialMonitorInterface.print("backState:");
  SerialMonitorInterface.println(backState);

  SerialMonitorInterface.print("chestState:");
  SerialMonitorInterface.println(chestState);

  SerialMonitorInterface.print("leftSideState:");
  SerialMonitorInterface.println(leftSideState);

  SerialMonitorInterface.print("rightSideState:");
  SerialMonitorInterface.println(rightSideState);
  
  SerialMonitorInterface.println();
#endif
}


void publishPatientState() {
  sprintf(topic, "%s", "");
  sprintf(topic, "%s%s", "/v1.6/devices/", DEVICE_LABEL);

  sprintf(payload, "%s", "");                                               // Cleans the payload content
  sprintf(payload, "{\"%s\":", VARIABLE_LABEL);                             // Adds the variable label
  sprintf(payload, "%s {\"value\": %d,", payload, (int)patient_state);      // Adds the value
  sprintf(payload, "%s \"context\": {\"description\": \"%s\",", payload, str_description);  // Adds the description 
  sprintf(payload, "%s \"seconds\": %d}}}", payload, elapsed_seconds);      // Adds the elapsed seconds value and closes the dictionary brackets

#if MQTT_DEBUG
  SerialMonitorInterface.println(payload);
#endif

  mqtt_client.publish(topic, payload); // Publish variable
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqtt_client.connected())
  {
#if MQTT_DEBUG
    SerialMonitorInterface.println("Attempting MQTT connection...");
#endif

    // Attempt to connect
    if (mqtt_client.connect("9704813141", UBIDOTS_TOKEN, ""))
    {
#if MQTT_DEBUG
      SerialMonitorInterface.println("connected");
#endif
    }
    else
    {
#if MQTT_DEBUG
      SerialMonitorInterface.print("failed, rc=");
      SerialMonitorInterface.print(mqtt_client.state());
      SerialMonitorInterface.println(" try again in 2 seconds");
#endif

      delay(2000);    // Wait 2 seconds before retrying
    }
  }
}

void connectToWiFi(char* ssid, char* password) {
  // Attempt to connect to Wifi network:
  SerialMonitorInterface.print("Connecting WiFi: ");
  SerialMonitorInterface.println(ssid);

  // Connect to WiFi, and loop until connection is secured
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    SerialMonitorInterface.print(".");
    delay(500);
  }

  // Print out the local IP address
  SerialMonitorInterface.println("");
  SerialMonitorInterface.println("WiFi connected");
  SerialMonitorInterface.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  SerialMonitorInterface.println(ip);
}
