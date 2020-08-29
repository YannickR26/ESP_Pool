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
#include "SimpleRelay.h"

// #define ENABLE_OTA    // If defined, enable Arduino OTA code.

// OTA
#ifdef ENABLE_OTA
#include <ArduinoOTA.h>
#endif

SensorDS18B20 ds18b20(DS18B20_PIN);
SensorAM2301 am2301_ext(DHT_1_PIN), am2301_int(DHT_2_PIN);
SolenoidValve valve(RELAY_1_PIN, RELAY_2_PIN);
RollerShutter rollerShutter(RELAY_3_PIN, RELAY_4_PIN);
SimpleRelay pump(RELAY_5_PIN, "pump");
SimpleRelay lamp(RELAY_5_PIN, "lamp");

static Ticker tick_blinker, tick_flowMetter;
static uint32_t flow1IntCnt; // flow2IntCnt;

// Value of flow
static float waterFlow1; //, waterFlow2; // in l/min

/*****************/
/*** INTERRUPT ***/
/*****************/

// Compute flow 1
IRAM_ATTR void onFlow1Interrupt()
{
  flow1IntCnt++;
}

// Compute flow 2
// IRAM_ATTR void onFlow2Interrupt()
// {
//   flow2IntCnt++;
// }

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

// Call every 1 second, so the counter is equal to frequency
void computeFlowMetter()
{
  static uint32_t oldTime = 0;
  uint32_t currentTime = millis();
  float ratio = 1000.0 / (currentTime - oldTime);

  // Detach the interrupt while calculating flow rate
  detachInterrupt(digitalPinToInterrupt(FLOW_1_PIN));
  // detachInterrupt(digitalPinToInterrupt(FLOW_2_PIN));

  // Compute flow metter 1
  waterFlow1 = 0;
  if (flow1IntCnt > 0)
    waterFlow1 = FLOW_COEF_A * (ratio * flow1IntCnt) + FLOW_COEF_B;
  flow1IntCnt = 0;

  // Compute quantity of water (integral)
  Configuration._waterQtyA += waterFlow1 / (60.f * ratio);

  // Compute flow metter 2
  // waterFlow2 = 0;
  // if (flow2IntCnt > 0)
  //   waterFlow2 = FLOW_COEF_A * (ratio * flow2IntCnt) + FLOW_COEF_B;
  // flow2IntCnt = 0;

  // // Compute quantity of water (integral)
  // Configuration._waterQtyB += waterFlow2 / (60.0 * ratio);

  // Save current timestamp
  oldTime = currentTime;

  // Log.println("flow1IntCnt : " + String(flow1IntCnt) + " Hz");
  // Log.println("waterFlow1 : " + String(waterFlow1) + " L/min");
  // Log.println("waterQty 1 : " + String(waterQty1) + " L");
  // Log.println("waterQty 2 : " + String(waterQty2) + " L");

  // Compute Water level, in cm
  uint16_t adc = analogRead(WATER_LEVEL_PIN);
  float valueInCm = WATER_LEVEL_COEF_A * adc + WATER_LEVEL_COEF_B;
  if (valueInCm < 0)
    valueInCm = 0;
  Configuration._waterLevel = (Configuration._waterLevel * 29 + valueInCm * 1) / 30;

  // Enable the interrupt
  attachInterrupt(digitalPinToInterrupt(FLOW_1_PIN), onFlow1Interrupt, FALLING);
  // attachInterrupt(digitalPinToInterrupt(FLOW_2_PIN), onFlow2Interrupt, FALLING);
}

void updateTimeAndSaveData()
{
  time_t now = time(nullptr);
  Log.print("Update NTP...");

  configTime(UTC_OFFSET * 3600, 0, NTP_SERVERS);
  delay(500);
  while (now < EPOCH_1_1_2019)
  {
    now = time(nullptr);
    Log.print(".");
    delay(500);
  }

  Log.println(" Done !");
  Log.println("Save data...");
  Configuration.saveConfig();
}

void sendData()
{
  Log.println();
  Log.println("Send data to MQTT :");

  // Read Water Temp, in °C
  float tmp = ds18b20.readTemp();
  if (tmp != DEVICE_DISCONNECTED_C)
  {
    tmp += 0.5f;
    Configuration._waterTemp = (Configuration._waterTemp + tmp) / 2;
    Log.println("\t waterTemp1: \t" + String(Configuration._waterTemp) + " °C");
    MqttClient.publish(String("waterTemp1"), String(Configuration._waterTemp));
  }

  // Read Internal Temp, in °C
  tmp = am2301_int.readTemp();
  const char *state;
  state = am2301_int.getStatus();
  if (strcmp("OK", state) == 0)
  {
    Configuration._intTemp = (Configuration._intTemp + tmp) / 2;
    Log.println("\t intTemp: \t" + String(Configuration._intTemp) + " °C" + " (" + String(state) + ")");
    MqttClient.publish(String("intTemp"), String(Configuration._intTemp));

    // Read Internal Humidity, in %
    tmp = am2301_int.readHumidity();
    Configuration._intHumidity = (Configuration._intHumidity + tmp) / 2;
    Log.println("\t intHumidity: \t" + String(Configuration._intHumidity) + " %");
    MqttClient.publish(String("intHumidity"), String(Configuration._intHumidity));
  }

  // Read External Temp, in °C
  tmp = am2301_ext.readTemp();
  state = am2301_ext.getStatus();
  if (strcmp("OK", state) == 0)
  {
    Configuration._extTemp = (Configuration._extTemp + tmp) / 2;
    Log.println("\t extTemp: \t" + String(Configuration._extTemp) + " °C" + " (" + String(state) + ")");
    MqttClient.publish(String("extTemp"), String(Configuration._extTemp));

    // Read External Humidity, in %
    tmp = am2301_ext.readHumidity();
    Configuration._extHumidity = (Configuration._extHumidity + tmp) / 2;
    Log.println("\t extHumidity: \t" + String(Configuration._extHumidity) + " %");
    MqttClient.publish(String("extHumidity"), String(Configuration._extHumidity));
  }

  // flow metter 1, in L/min
  Log.println("\t waterFlow1: \t" + String(waterFlow1) + " L/Min");
  MqttClient.publish(String("waterFlow1"), String(waterFlow1));

  // Water quantity 1, in L
  Log.println("\t waterQty1: \t" + String(Configuration._waterQtyA) + " L");
  MqttClient.publish(String("waterQty1"), String(Configuration._waterQtyA));

  // // flow metter 2, in L/min
  // Log.println("\t waterFlow2: \t" + String(waterFlow2) + " L/Min");
  // MqttClient.publish("waterFlow2", String(waterFlow2));

  // // Water quantity 2, in L
  // Log.println("\t waterQty2: \t" + String(Configuration._waterQtyB) + " L");
  // MqttClient.publish("waterQty2", String(Configuration._waterQtyB));

  // Water level, in cm
  Log.println("\t waterLevel: \t" + String(Configuration._waterLevel) + " cm");
  MqttClient.publish(String("waterLevel"), String(Configuration._waterLevel));
}

/************/
/*** WIFI ***/
/************/
void wifiSetup()
{
  WiFiManager wm;
  // wm.setDebugOutput(false);
  // wm.resetSettings();

  // WiFiManagerParameter
  WiFiManagerParameter custom_mqtt_hostname("hostname", "hostname", Configuration._hostname.c_str(), 60);
  WiFiManagerParameter custom_mqtt_server("server", "mqtt ip", Configuration._mqttIpServer.c_str(), 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", String(Configuration._mqttPortServer).c_str(), 6);
  WiFiManagerParameter custom_time_update("timeSendData", "time update data (s)", String(Configuration._timeSendData).c_str(), 6);

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

  // Disable sleep mode
  WiFi.setSleep(false);

  // Stop AP Mode
  WiFi.enableAP(false);
  WiFi.softAPdisconnect();

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
  pinMode(LED_PIN, OUTPUT);
  pinMode(FLOW_1_PIN, INPUT);
  pinMode(FLOW_2_PIN, INPUT);

  // Create ticker for blink LED
  tick_blinker.once(LED_TIME_NOMQTT, blinkLED);

  // Attach interrupt for compute frequency
  flow1IntCnt = 0;
  // flow2IntCnt = 0;
  attachInterrupt(digitalPinToInterrupt(FLOW_1_PIN), onFlow1Interrupt, FALLING);
  // attachInterrupt(digitalPinToInterrupt(FLOW_2_PIN), onFlow2Interrupt, FALLING);

  /* Read configuration from SPIFFS */
  Configuration.setup();
  // Configuration.restoreDefault();
  rollerShutter.setTimeout(Configuration._rollerShutterTimeout);
  valve.setTimeout(Configuration._solenoidValveTimeout);
  valve.setMaxWaterQuantity(Configuration._solenoidValveMaxWaterQty);
  valve.setMaxWaterLevel(Configuration._solenoidValveMaxWaterLevel);
  pump.setTimeout(Configuration._pumpTimeout);
  lamp.setTimeout(Configuration._lampTimeout);

  // Configure and run WifiManager
  wifiSetup();

  /* Initialize HTTP Server */
  HTTPServer.setup();

  /* Initialize MQTT Client */
  MqttClient.setup();

  Log.setupTelnet();

  // Init OTA
#ifdef ENABLE_OTA
  Log.println("Arduino OTA activated");

  // Port defaults to 8266
  ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(Configuration._hostname.c_str());

  ArduinoOTA.onStart([]() {
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

  // Create ticker for update NTP time and save data
  updateTimeAndSaveData();

  // Create ticker for compute Flow Metter, must be each 1 seconds
  tick_flowMetter.attach(1, computeFlowMetter);
}

/************/
/*** LOOP ***/
/************/
void loop()
{
  unsigned long tick = millis();
  static unsigned long tickSaveData = 0, tickSendData = 0;
  static uint8_t noWifiConnection = 0;

  MqttClient.handle();
  Log.handle();
  HTTPServer.handle();
  valve.handle();

  if ((tick - tickSendData) >= (Configuration._timeSendData * 1000))
  {
    sendData();
    tickSendData = tick;
  }

  if ((tick - tickSaveData) >= (Configuration._timeSaveData * 1000))
  {
    updateTimeAndSaveData();
    tickSaveData = tick;
  }

  if (!WiFi.isConnected())
  {
    if (noWifiConnection >= 10)
    {
      ESP.restart();
    }
    else
    {
      noWifiConnection++;
    }
  }
  else
  {
    noWifiConnection = 0;
  }

#ifdef ENABLE_OTA
  ArduinoOTA.handle();
#endif

  delay(50);
}
