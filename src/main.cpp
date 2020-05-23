#include <Arduino.h>
#include <WiFiManager.h>
#include <Ticker.h>

#include "JsonConfiguration.h"
#include "HttpServer.h"
#include "Mqtt.h"
#include "settings.h"
#include "Logger.h"
#include "DS18B20.h"

// #define ENABLE_OTA    // If defined, enable Arduino OTA code.

// OTA
#ifdef ENABLE_OTA
#include <ArduinoOTA.h>
#endif

static Ticker tick_blinker, tick_ntp, tick_flowMetter, tick_sendDataMqtt;
static bool mqttConnected = false;
static uint32_t flow1IntCnt, flow2IntCnt, flow1, flow2;
static DS18B20 temp1(TEMP_1_PIN), temp2(TEMP_2_PIN);

/*****************/
/*** INTERRUPT ***/
/*****************/

// Compute flow 1
IRAM_ATTR void onFlow1Interrupt()
{
  flow1IntCnt++;
}

// Compute flow 2
IRAM_ATTR void onFlow2Interrupt()
{
  flow2IntCnt++;
}

/**************/
/*** TICKER ***/
/**************/

// LED blink
void blinkLED()
{
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}

void updateNTP()
{
  configTime(UTC_OFFSET * 3600, 0, NTP_SERVERS);
  delay(500);
  while (!time(nullptr))
  {
    Log.print("#");
    delay(1000);
  }
  Log.println("Update NTP");
}

void computeFlowMetter()
{
  // Compute flow metter 1
  flow1 = flow1IntCnt * 6;
  flow1IntCnt = 0;
  // Compute flow metter 2
  flow2 = flow2IntCnt * 6;
  flow2IntCnt = 0;
}

void sendData()
{
  Log.println("Send data to MQTT");
  // Monitoring.handle();
  MqttClient.publishMonitoringData();

  // Check state of MQTT
  bool currentMqttState = MqttClient.isConnected();
  if (currentMqttState != mqttConnected)
  {
    if (currentMqttState)
    {
      tick_blinker.attach(LED_TIME_WORK, blinkLED);
    }
    else
    {
      tick_blinker.attach(LED_TIME_NOMQTT, blinkLED);
    }
    mqttConnected = currentMqttState;
  }
}

/************/
/*** WIFI ***/
/************/
void wifiSetup()
{
  WiFiManager wifiManager;
  // wifiManager.setDebugOutput(false);
  // wifiManager.resetSettings();

  // WiFiManagerParameter
  WiFiManagerParameter custom_mqtt_hostname("hostname", "hostname", Configuration._hostname.c_str(), 60);
  WiFiManagerParameter custom_mqtt_server("server", "mqtt ip", Configuration._mqttIpServer.c_str(), 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", String(Configuration._mqttPortServer).c_str(), 6);
  WiFiManagerParameter custom_time_update("timeUpdate", "time update data (s)", String(Configuration._timeSendData).c_str(), 6);

  // add all your parameters here
  wifiManager.addParameter(&custom_mqtt_hostname);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_time_update);

  Log.println("Try to connect to WiFi...");
  // wifiManager.setWiFiChannel(6);
  wifiManager.setConfigPortalTimeout(300); // Set Timeout for portal configuration to 300 seconds
  if (!wifiManager.autoConnect(Configuration._hostname.c_str()))
  {
    Log.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }

  Log.println(String("Connected to ") + WiFi.SSID());
  Log.println(String("IP address: ") + WiFi.localIP().toString());

  /* Get configuration from WifiManager */
  Configuration._hostname = custom_mqtt_hostname.getValue();
  Configuration._mqttIpServer = custom_mqtt_server.getValue();
  Configuration._mqttPortServer = atoi(custom_mqtt_port.getValue());
  Configuration._timeSendData = atoi(custom_time_update.getValue());
  Configuration.saveConfig();
}

/*************/
/*** SETUP ***/
/*************/
void setup()
{
  /* Initialize Logger */
  Log.setup();
  Log.println(String(F("ESP_Pool - Build: ")) + F(__DATE__) + " " + F(__TIME__));

  // Setup PIN
  pinMode(RELAY_1_PIN, OUTPUT);
  pinMode(RELAY_2_PIN, OUTPUT);
  pinMode(RELAY_3_PIN, OUTPUT);
  pinMode(RELAY_4_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(FLOW_1_PIN, INPUT_PULLUP);
  pinMode(FLOW_2_PIN, INPUT_PULLUP);

  // Create ticker for blink LED
  tick_blinker.attach(LED_TIME_NOMQTT, blinkLED);

  // Attach interrupt for compute frequency
  attachInterrupt(digitalPinToInterrupt(FLOW_1_PIN), onFlow1Interrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(FLOW_2_PIN), onFlow2Interrupt, FALLING);
  flow1IntCnt = flow2IntCnt = 0;

  /* Read configuration from SPIFFS */
  Configuration.setup();
  // Configuration.restoreDefault();

  // Configure and run WifiManager
  wifiSetup();

  /* Initialize HTTP Server */
  HTTPServer.setup();

  delay(100);

  /* Initialize MQTT Client */
  MqttClient.setup();

  // Init OTA
#ifdef ENABLE_OTA
  Log.println("Arduino OTA activated");

  // Port defaults to 8266
  ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(Configuration._hostname.c_str());

  ArduinoOTA.onStart([&]() {
    Log.println("Arduino OTA: Start updating");
  });
  ArduinoOTA.onEnd([]() {
    Log.println("Arduino OTA: End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Log.printf("Arduino OTA Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Log.printf("Arduino OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Log.println("Arduino OTA: Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Log.println("Arduino OTA: Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Log.println("Arduino OTA: Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Log.println("Arduino OTA: Receive Failed");
    else if (error == OTA_END_ERROR)
      Log.println("Arduino OTA: End Failed");
  });

  ArduinoOTA.begin();
  Log.println("");
#endif

  // Create ticker for update NTP time
  updateNTP();
  tick_ntp.attach(Configuration._timeUpdateNtp, updateNTP);

  // Create ticker for send data to MQTT
  tick_sendDataMqtt.attach(Configuration._timeSendData, sendData);

  // Create ticker for compute Flow Metter
  tick_flowMetter.attach(1, computeFlowMetter);
}

/************/
/*** LOOP ***/
/************/
void loop()
{
  MqttClient.handle();
  Log.handle();
  HTTPServer.handle();

#ifdef ENABLE_OTA
  ArduinoOTA.handle();
#endif

  delay(50);
}
