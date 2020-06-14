#include <Arduino.h>
#include <WiFiManager.h>
#include <Ticker.h>

#include "JsonConfiguration.h"
#include "HttpServer.h"
#include "Mqtt.h"
#include "settings.h"
#include "Logger.h"
#include "SensorDS18B20.h"
#include "SensorAM2301.h"
#include "RollerShutter.h"
#include "SolenoidValve.h"

// #define ENABLE_OTA    // If defined, enable Arduino OTA code.

// OTA
#ifdef ENABLE_OTA
#include <ArduinoOTA.h>
#endif

SensorDS18B20 ds18b20_1(DS18B20_1_PIN); //, ds18b20_2(DS18B20_2_PIN);
SensorAM2301 am2301(DHT_PIN);
RollerShutter rollerShutter(RELAY_1_PIN, RELAY_2_PIN);
SolenoidValve valve(RELAY_3_PIN, RELAY_4_PIN);

static Ticker tick_blinker, tick_ntp, tick_flowMetter, tick_sendDataMqtt;
static uint32_t flow1IntCnt, flow2IntCnt;

// Value of flow
static float waterFlow1, waterFlow2; // in l/min
static float waterQty1, waterQty2;   // in l
// Value of water level
static float waterLevel; // in cm
// Temperature value
static float waterTemp1, waterTemp2, intTemp; // in °C
// Humidity value
static float intHumidity; // in %

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

  // Check state of MQTT
  if (MqttClient.isConnected())
  {
    tick_blinker.once(LED_TIME_WORK, blinkLED);
  }
  else
  {
    tick_blinker.once(LED_TIME_NOMQTT, blinkLED);
  }
}

void updateTimeAndSaveData()
{
  Log.println("Update NTP");

  configTime(UTC_OFFSET * 3600, 0, NTP_SERVERS);
  delay(500);
  while (!time(nullptr))
  {
    Log.print("#");
    delay(1000);
  }

  Log.println("Save data");
  Configuration._waterQtyA = waterQty1;
  Configuration._waterQtyB = waterQty2;
  Configuration.saveConfig();

  // Restart ticker
  if (Configuration._timeSaveData > 0)
    tick_ntp.once(Configuration._timeSaveData, updateTimeAndSaveData);
}

// Call every 1 second, so the counter is equal to frequency
void computeFlowMetter()
{
  static uint32_t oldTime = 0;
  uint32_t currentTime = millis();
  float ratio = 1000.0 / (currentTime - oldTime);

  // Detach the interrupt while calculating flow rate
  detachInterrupt(digitalPinToInterrupt(FLOW_1_PIN));
  detachInterrupt(digitalPinToInterrupt(FLOW_2_PIN));

  // Compute flow metter 1
  waterFlow1 = 0;
  if (flow1IntCnt > 0)
    waterFlow1 = FLOW_COEF_A * (ratio * flow1IntCnt) + FLOW_COEF_B;
  flow1IntCnt = 0;

  // Compute quantity of water (integral)
  waterQty1 += waterFlow1 / (60.0 * ratio);

  // Compute flow metter 2
  waterFlow2 = 0;
  if (flow2IntCnt > 0)
    waterFlow2 = FLOW_COEF_A * (ratio * flow2IntCnt) + FLOW_COEF_B;
  flow2IntCnt = 0;

  // Compute quantity of water (integral)
  waterQty2 += waterFlow2 / (60.0 * ratio);

  // Save current timestamp
  oldTime = currentTime;

  // Log.println("flow 1 : " + String(waterFlow1) + " L/min");
  // Log.println("flow 2 : " + String(waterFlow2) + " L/min");
  // Log.println("waterQty 1 : " + String(waterQty1) + " L");
  // Log.println("waterQty 2 : " + String(waterQty2) + " L");

  // Enable the interrupt
  attachInterrupt(digitalPinToInterrupt(FLOW_1_PIN), onFlow1Interrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(FLOW_2_PIN), onFlow2Interrupt, FALLING);
}

void sendData()
{
  Log.println();
  Log.println("Send data to MQTT :");

  // Read Water Temp 1, in °C
  float tmp = ds18b20_1.readTemp();
  waterTemp1 = (waterTemp1 + tmp) / 2;
  Log.println("\t waterTemp1: \t" + String(waterTemp1) + " °C");
  MqttClient.publish("waterTemp1", String(waterTemp1));

  // Read Water Temp 2, in °C
  // tmp = ds18b20_2.readTemp();
  // waterTemp2 = (waterTemp2 + tmp) / 2;
  // Log.println("\t waterTemp2: \t" + String(waterTemp2) + " °C");
  // MqttClient.publish("waterTemp2", String(waterTemp2));

  // Read Internal Temp, in °C
  tmp = am2301.readTemp();
  intTemp = (intTemp + tmp) / 2;
  Log.println("\t intTemp: \t" + String(intTemp) + " °C");
  MqttClient.publish("intTemp", String(intTemp));

  // Read Internal Humidity, in %
  tmp = am2301.readHumidity();
  intHumidity = (intHumidity + tmp) / 2;
  Log.println("\t intHumidity: \t" + String(intHumidity) + " %");
  MqttClient.publish("intHumidity", String(intHumidity));

  // flow metter 1, in L/min
  Log.println("\t waterFlow1: \t" + String(waterFlow1) + " L/Min");
  MqttClient.publish("waterFlow1", String(waterFlow1));

  // Water quantity 1, in L
  Log.println("\t waterQty1: \t" + String(waterQty1) + " L");
  MqttClient.publish("waterQty1", String(waterQty1));

  // flow metter 2, in L/min
  Log.println("\t waterFlow2: \t" + String(waterFlow2) + " L/Min");
  MqttClient.publish("waterFlow2", String(waterFlow2));

  // Water quantity 2, in L
  Log.println("\t waterQty2: \t" + String(waterQty2) + " L");
  MqttClient.publish("waterQty2", String(waterQty2));

  // Water level, in cm
  uint16_t adc = analogRead(WATER_LEVEL_PIN);
  float valueInCm = WATER_LEVEL_COEF_A * adc + WATER_LEVEL_COEF_B;
  if (valueInCm < 0)
    valueInCm = 0;
  waterLevel = (waterLevel + valueInCm) / 2;
  Log.println("\t waterLevel: \t" + String((int)waterLevel) + " cm");
  MqttClient.publish("waterLevel", String((int)waterLevel));

  // Restart ticker
  if (Configuration._timeSendData > 0)
    tick_sendDataMqtt.once(Configuration._timeSendData, sendData);
}

/************/
/*** WIFI ***/
/************/
void wifiSetup()
{
  WiFiManager wm;
  wm.setDebugOutput(false);
  // wm.resetSettings();

  // WiFiManagerParameter
  WiFiManagerParameter custom_mqtt_hostname("hostname", "hostname", Configuration._hostname.c_str(), 60);
  WiFiManagerParameter custom_mqtt_server("server", "mqtt ip", Configuration._mqttIpServer.c_str(), 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", String(Configuration._mqttPortServer).c_str(), 6);
  WiFiManagerParameter custom_time_update("timeUpdate", "time update data (s)", String(Configuration._timeSendData).c_str(), 6);

  // add all your parameters here
  wm.addParameter(&custom_mqtt_hostname);
  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_time_update);

  Log.println("Try to connect to WiFi...");
  // wm.setWiFiChannel(6);
  wm.setConfigPortalTimeout(300); // Set Timeout for portal configuration to 300 seconds
  if (!wm.autoConnect(Configuration._hostname.c_str()))
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
  Log.println();
  Log.println(String(F("==============================")));
  Log.println(String(F("---------- ESP_Pool ----------")));
  Log.println(String(F("  Version: ")) + F(VERSION));
  Log.println(String(F("  Build: ")) + F(__DATE__) + " " + F(__TIME__));
  Log.println(String(F("==============================")));
  Log.println();

  // Setup PIN
  pinMode(RELAY_1_PIN, OUTPUT);
  pinMode(RELAY_2_PIN, OUTPUT);
  pinMode(RELAY_3_PIN, OUTPUT);
  pinMode(RELAY_4_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(FLOW_1_PIN, INPUT_PULLUP);
  pinMode(FLOW_2_PIN, INPUT_PULLUP);
  // pinMode(WATER_LEVEL_PIN, INPUT);

  // Create ticker for blink LED
  tick_blinker.once(LED_TIME_NOMQTT, blinkLED);

  // Attach interrupt for compute frequency
  attachInterrupt(digitalPinToInterrupt(FLOW_1_PIN), onFlow1Interrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(FLOW_2_PIN), onFlow2Interrupt, FALLING);
  flow1IntCnt = flow2IntCnt = 0;

  /* Read configuration from SPIFFS */
  Configuration.setup();
  // Configuration.restoreDefault();
  waterQty1 = Configuration._waterQtyA;
  waterQty2 = Configuration._waterQtyB;
  rollerShutter.setTimeout(Configuration._rollerShutterTimeout);

  // Configure and run WifiManager
  wifiSetup();

  /* Initialize HTTP Server */
  HTTPServer.setup();

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
  updateTimeAndSaveData();
  if (Configuration._timeSaveData == 0)
    Configuration._timeSaveData = 1;
  tick_ntp.once(Configuration._timeSaveData, updateTimeAndSaveData);

  // Create ticker for send data to MQTT
  if (Configuration._timeSendData == 0)
    Configuration._timeSendData = 1;
  tick_sendDataMqtt.once(Configuration._timeSendData, sendData);

  // Create ticker for compute Flow Metter, must be each 1 seconds
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
