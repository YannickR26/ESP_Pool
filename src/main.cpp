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

// OTA
#ifdef ENABLE_OTA
#include <ArduinoOTA.h>
#endif

SensorDS18B20 ds18b20(DS18B20_PIN);
SensorAM2301 am2301(DHT_PIN);

static Ticker tick_blinker, tick_flowMetter;
static uint32_t flowIntCnt;

// Value of flow
static float waterFlow; // in l/min

/*****************/
/*** INTERRUPT ***/
/*****************/

// Compute flow
IRAM_ATTR void onFlowInterrupt()
{
  flowIntCnt++;
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

// Call every 1 second, so the counter is equal to frequency
void computeFlowMetter()
{
  static uint32_t oldTime = 0;
  uint32_t currentTime = millis();
  float ratio = 1000.0 / (currentTime - oldTime);

  // Detach the interrupt while calculating flow rate
  detachInterrupt(digitalPinToInterrupt(FLOW_PIN));

  // Compute flow metter 1
  waterFlow = 0;
  if (flowIntCnt > 0)
    waterFlow = FLOW_COEF_A * (ratio * flowIntCnt) + FLOW_COEF_B;
  flowIntCnt = 0;

  // Compute quantity of water (integral)
  Configuration._waterQty += waterFlow / (60.f * ratio);

  // Save current timestamp
  oldTime = currentTime;

  // Log.println("flowIntCnt : " + String(flowIntCnt) + " Hz");
  // Log.println("waterFlow1 : " + String(waterFlow) + " L/min");
  // Log.println("waterQty 1 : " + String(waterQty) + " L");

  // Enable the interrupt
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), onFlowInterrupt, FALLING);
}

void sendData()
{
  Log.println();
  Log.println("Send data to MQTT :");

  // Read Water Temp, in °C
  float tmp = ds18b20.readTemp();
  if (tmp != DEVICE_DISCONNECTED_C)
  {
    Configuration._waterTemp = (Configuration._waterTemp + tmp) / 2;
    Log.println("\t waterTemp: \t" + String(Configuration._waterTemp) + " °C");
    MqttClient.publish(String("waterTemp"), String(Configuration._waterTemp));
  }

  // // Read Internal Temp, in °C
  tmp = am2301.readTemp();
  if (strcmp("OK", am2301.getStatus()) == 0)
  {
    Configuration._temp = (Configuration._temp + tmp) / 2;
    Log.println("\t temp: \t" + String(Configuration._temp) + " °C");
    MqttClient.publish(String("temp"), String(Configuration._temp));

    // Read Internal Humidity, in %
    tmp = am2301.readHumidity();
    Configuration._humidity = (Configuration._humidity + tmp) / 2;
    Log.println("\t humidity: \t" + String(Configuration._humidity) + " %");
    MqttClient.publish(String("humidity"), String(Configuration._humidity));
  }
  
  // flow metter 1, in L/min
  Log.println("\t waterFlow: \t" + String(waterFlow) + " L/Min");
  MqttClient.publish(String("waterFlow"), String(waterFlow));

  // Water quantity 1, in L
  Log.println("\t waterQty: \t" + String(Configuration._waterQty) + " L");
  MqttClient.publish(String("waterQty"), String(Configuration._waterQty));
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
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

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
  pinMode(FLOW_PIN, INPUT);

  // Create ticker for blink LED
  tick_blinker.once(LED_TIME_NOMQTT, blinkLED);

  // Attach interrupt for compute frequency
  flowIntCnt = 0;
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), onFlowInterrupt, FALLING);

  /* Read configuration from SPIFFS */
  Configuration.setup();
  // Configuration.restoreDefault();

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

  // update NTP time and save data
  updateTimeAndSaveData();

  // Create ticker for compute Flow Metter, must be each 1 seconds
  tick_flowMetter.attach(1, computeFlowMetter);
}

/************/
/*** LOOP ***/
/************/
void loop()
{
  unsigned long currentTick = millis();
  static unsigned long tickSendData = 0, tickSaveData = 0;
  static uint8_t noWifiConnection = 0;

  MqttClient.handle();
  Log.handle();
  HTTPServer.handle();

  if ((currentTick - tickSendData) > (Configuration._timeSendData * 1000))
  {
    sendData();
    tickSendData = currentTick;
  }

  if ((currentTick - tickSaveData) > (Configuration._timeSaveData * 1000))
  {
    updateTimeAndSaveData();
    tickSaveData = currentTick;
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
