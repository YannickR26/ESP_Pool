#include <Arduino.h>
#include <ESP_WiFiManager.h>
#include <Ticker.h>
#include <time.h>

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
#include "ExtendedRelay.h"
#include "Pwm.h"

// OTA
#ifdef ENABLE_OTA
#include <ArduinoOTA.h>
#endif

SensorDS18B20 ds18b20(DS18B20_PIN);
SensorAM2301 am2301_ext(DHT_1_PIN), am2301_int(DHT_2_PIN);
SolenoidValve valve(RELAY_1_PIN, RELAY_2_PIN);
RollerShutter rollerShutter(RELAY_3_PIN, RELAY_4_PIN);
SimpleRelay lamp(RELAY_5_PIN, "lamp");
ExtendedRelay pump(RELAY_6_PIN, "pump");
Pwm lightExt(PWM_LAMP_EXT_PIN, 0, PWM_FREQUENCY);

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
  static unsigned long oldTime = 0;
  const unsigned long currentTime = millis();
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
  struct tm timeinfo;

  Log.print("Update NTP...");

  configTzTime(TIMEZONE, NTP_SERVERS);
  getLocalTime(&timeinfo);

  Log.println(" Done !");
  Log.print("Date Time: ");
  Log.println(asctime(&timeinfo));

  Configuration.saveConfig();
}

void sendData()
{
  // If MQTT is not connected, we return now
  if (!MqttClient.isConnected())
    return;

  Log.println();
  Log.println("Read and Send data to MQTT :");

  // Read Wifi RSSI
  int8_t rssi = WiFi.RSSI();
  Log.println("\t wifiRSSI: \t" + String(rssi) + " dBm");
  MqttClient.publish(String("wifiRSSI"), String(rssi));

  // Read Water Temp, in °C
  float tmp = ds18b20.readTemp();
  if (tmp != DEVICE_DISCONNECTED_C)
  {
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

void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.printf("[WiFi-event] event: %d => ", event);

  switch (event)
  {
  case SYSTEM_EVENT_WIFI_READY:
    Serial.println("WiFi interface ready");
    break;
  case SYSTEM_EVENT_SCAN_DONE:
    Serial.println("Completed scan for access points");
    break;
  case SYSTEM_EVENT_STA_START:
    Serial.println("WiFi client started");
    break;
  case SYSTEM_EVENT_STA_STOP:
    Serial.println("WiFi client stopped");
    break;
  case SYSTEM_EVENT_STA_CONNECTED:
    Serial.println("Connected to access point");
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    Serial.println("Disconnected from WiFi access point");
    break;
  case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
    Serial.println("Authentication mode of access point has changed");
    break;
  case SYSTEM_EVENT_STA_GOT_IP:
    Serial.print("Obtained IP address: ");
    // Serial.println(WiFi.localIP());
    //Serial.println("WiFi connected");
    //Serial.print("IP address: ");
    Serial.println(IPAddress(info.got_ip.ip_info.ip.addr));
    break;
  case SYSTEM_EVENT_STA_LOST_IP:
    Serial.println("Lost IP address and IP address is reset to 0");
    break;
  case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
    Serial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode");
    break;
  case SYSTEM_EVENT_STA_WPS_ER_FAILED:
    Serial.println("WiFi Protected Setup (WPS): failed in enrollee mode");
    break;
  case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
    Serial.println("WiFi Protected Setup (WPS): timeout in enrollee mode");
    break;
  case SYSTEM_EVENT_STA_WPS_ER_PIN:
    Serial.println("WiFi Protected Setup (WPS): pin code in enrollee mode");
    break;
  case SYSTEM_EVENT_AP_START:
    Serial.println("WiFi access point started");
    break;
  case SYSTEM_EVENT_AP_STOP:
    Serial.println("WiFi access point  stopped");
    break;
  case SYSTEM_EVENT_AP_STACONNECTED:
    Serial.println("Client connected");
    break;
  case SYSTEM_EVENT_AP_STADISCONNECTED:
    Serial.println("Client disconnected");
    break;
  case SYSTEM_EVENT_AP_STAIPASSIGNED:
    Serial.println("Assigned IP address to client");
    break;
  case SYSTEM_EVENT_AP_PROBEREQRECVED:
    Serial.println("Received probe request");
    break;
  case SYSTEM_EVENT_GOT_IP6:
    Serial.println("IPv6 is preferred");
    break;
  case SYSTEM_EVENT_ETH_START:
    Serial.println("Ethernet started");
    break;
  case SYSTEM_EVENT_ETH_STOP:
    Serial.println("Ethernet stopped");
    break;
  case SYSTEM_EVENT_ETH_CONNECTED:
    Serial.println("Ethernet connected");
    break;
  case SYSTEM_EVENT_ETH_DISCONNECTED:
    Serial.println("Ethernet disconnected");
    break;
  case SYSTEM_EVENT_ETH_GOT_IP:
    Serial.println("Obtained IP address");
    break;
  default:
    break;
  }
}

void wifiSetup()
{
  WiFi.onEvent(WiFiEvent);

  ESP_WiFiManager wm;
  // wm.setDebugOutput(false);
  // wm.resetSettings();

  // WiFiManagerParameter
  ESP_WMParameter custom_mqtt_hostname("hostname", "hostname", Configuration._hostname.c_str(), 60);
  ESP_WMParameter custom_mqtt_server("server", "mqtt ip", Configuration._mqttIpServer.c_str(), 40);
  ESP_WMParameter custom_mqtt_port("port", "mqtt port", String(Configuration._mqttPortServer).c_str(), 6);
  ESP_WMParameter custom_time_update("timeSendData", "time update data (s)", String(Configuration._timeSendData).c_str(), 6);
  ESP_WMParameter custom_time_save("timeSaveData", "time save data (s)", String(Configuration._timeSaveData).c_str(), 6);

  // add all your parameters here
  wm.addParameter(&custom_mqtt_hostname);
  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_time_update);
  wm.addParameter(&custom_time_save);

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
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);

  /* Get configuration from WifiManager */
  Configuration._hostname = custom_mqtt_hostname.getValue();
  Configuration._mqttIpServer = custom_mqtt_server.getValue();
  Configuration._mqttPortServer = atoi(custom_mqtt_port.getValue());
  Configuration._timeSendData = atoi(custom_time_update.getValue());
  Configuration._timeSaveData = atoi(custom_time_save.getValue());
  Configuration.saveConfig();
}

/*************/
/*** SETUP ***/
/*************/
void setup()
{
  /* Initialize Logger */
  Log.setup();
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
  rollerShutter.setDuration(Configuration._rollerShutterDuration);
  rollerShutter.setCurrentPosition(Configuration._rollerShutterPosition);
  valve.setTimeout(Configuration._solenoidValveTimeout);
  valve.setMaxWaterQuantity(Configuration._solenoidValveMaxWaterQty);
  valve.setMaxWaterLevel(Configuration._solenoidValveMaxWaterLevel);
  pump.setModeAuto(Configuration._pumpModeAuto);
  pump.setStartTime(Configuration._pumpStartHours, Configuration._pumpStartMinutes);
  pump.setStopTime(Configuration._pumpStopHours, Configuration._pumpStopMinutes);
  lamp.setTimeout(Configuration._lampTimeout);

  // Initialize PWM
  lightExt.setup();
  lightExt.setFadingSpeed(Configuration._lightFading);

  // Configure and run WifiManager
#ifdef USE_WIFI_MANAGER
  wifiSetup();
#else
  WiFi.setHostname(Configuration._hostname.c_str());
  WiFi.begin("Internet-Maison-Tenda", "MaisonMoreauRichardot");

  Log.print("Waiting for Wifi connection");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(200);
    Log.print(".");
  }
  Log.println(" done !");

  Log.println(String("Connected to ") + WiFi.SSID());
  Log.println(String("IP address: ") + WiFi.localIP().toString());

  // Stop AP Mode
  WiFi.enableAP(false);
  WiFi.softAPdisconnect();
#endif

  /* Update Time and save data */
  updateTimeAndSaveData();

  /* Initialize HTTP Server */
  HTTPServer.setup();

  /* Initialize MQTT Client */
  MqttClient.setup();

  Log.setupTelnet();

  // Init OTA
#ifdef ENABLE_OTA
  Log.println("Arduino OTA activated");

  ArduinoOTA.setHostname(Configuration._hostname.c_str());

  ArduinoOTA.onStart([]() {
    Log.println("Arduino OTA: Start updating");
  });
  ArduinoOTA.onEnd([]() {
    Log.println("Arduino OTA: End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Arduino OTA Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Arduino OTA Error[%u]: ", error);
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

  // Create ticker for compute Flow Metter, must be each 1 seconds
  tick_flowMetter.attach(1, computeFlowMetter);
}

/************/
/*** LOOP ***/
/************/
void loop()
{
  const unsigned long tick = millis();
  static unsigned long tickSaveData = 0, tickSendData = 0, tickCheckWifi = 0;
  static uint8_t noWifiConnection = 0;

  MqttClient.handle();
  Log.handle();
  HTTPServer.handle();
  valve.handle();
  lightExt.handle();
  rollerShutter.handle();
  pump.handle();

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

  // Check wifi connection every 10 seconds
  if ((tick - tickCheckWifi) >= 10000)
  {
    if (!WiFi.isConnected())
    {
      // If at 60 seconds we have no wifi, we force to reconnect
      if (noWifiConnection >= 6)
      {
        WiFi.reconnect();
        // ESP.restart();
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
    tickCheckWifi = tick;
  }

#ifdef ENABLE_OTA
  ArduinoOTA.handle();
#endif

  delay(10);
}
