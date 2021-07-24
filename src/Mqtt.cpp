#include "Mqtt.h"

#include "JsonConfiguration.h"
#include <rom/rtc.h>
#include <WiFi.h>
#include <ESP_WiFiManager.h>
#include "Logger.h"
#include "RollerShutter.h"
#include "SolenoidValve.h"
#include "SimpleRelay.h"
#include "Pwm.h"

WiFiClient espClient;
extern RollerShutter rollerShutter;
extern SolenoidValve valve;
extern SimpleRelay pump, lamp;
extern Pwm lightExt;

/********************************************************/
/******************** Public Method *********************/
/********************************************************/

Mqtt::Mqtt()
{
}

Mqtt::~Mqtt()
{
}

void Mqtt::setup()
{
  clientMqtt.setClient(espClient);
  clientMqtt.setServer(Configuration._mqttIpServer.c_str(), Configuration._mqttPortServer);
  clientMqtt.setCallback([this](char *topic, uint8_t *payload, unsigned int length) { this->callback(topic, payload, length); });
  startedAt = String(Log.getDateTimeString());
}

void Mqtt::handle()
{
  if (!clientMqtt.connected())
  {
    reconnect();
  }
  clientMqtt.loop();
}

void Mqtt::publish(String topic, String body)
{
  clientMqtt.publish(String(Configuration._hostname + '/' + topic).c_str(), String(body).c_str());
  delay(5);
}

void Mqtt::log(String level, String str)
{
  publish("log/" + level, str);
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/

String Mqtt::getResetReason(int cpu)
{
  switch (rtc_get_reset_reason(cpu))
  {
    case 1 : return String("POWERON_RESET");          /**<1, Vbat power on reset*/
    case 3 : return String("SW_RESET");               /**<3, Software reset digital core*/
    case 4 : return String("OWDT_RESET");             /**<4, Legacy watch dog reset digital core*/
    case 5 : return String("DEEPSLEEP_RESET");        /**<5, Deep Sleep reset digital core*/
    case 6 : return String("SDIO_RESET");             /**<6, Reset by SLC module, reset digital core*/
    case 7 : return String("TG0WDT_SYS_RESET");       /**<7, Timer Group0 Watch dog reset digital core*/
    case 8 : return String("TG1WDT_SYS_RESET");       /**<8, Timer Group1 Watch dog reset digital core*/
    case 9 : return String("RTCWDT_SYS_RESET");       /**<9, RTC Watch dog Reset digital core*/
    case 10 : return String("INTRUSION_RESET");       /**<10, Instrusion tested to reset CPU*/
    case 11 : return String("TGWDT_CPU_RESET");       /**<11, Time Group reset CPU*/
    case 12 : return String("SW_CPU_RESET");          /**<12, Software reset CPU*/
    case 13 : return String("RTCWDT_CPU_RESET");      /**<13, RTC Watch dog Reset CPU*/
    case 14 : return String("EXT_CPU_RESET");         /**<14, for APP CPU, reseted by PRO CPU*/
    case 15 : return String("RTCWDT_BROWN_OUT_RESET");/**<15, Reset when the vdd voltage is not stable*/
    case 16 : return String("RTCWDT_RTC_RESET");      /**<16, RTC Watch dog reset digital core and rtc module*/
    default : return String("NO_MEAN");
  }
}

void Mqtt::reconnect()
{
  static unsigned long tick = 0;

  if (!clientMqtt.connected())
  {
    if ((millis() - tick) >= 5000)
    {
      Log.print("Attempting MQTT connection... ");
      // Create a random clientMqtt ID
      String clientId = Configuration._hostname + String(random(0xffff), HEX);
      // Attempt to connect
      clientMqtt.setSocketTimeout(5);
      if (clientMqtt.connect(clientId.c_str()))
      {
        Log.println("connected !");
        // Once connected, publish an announcement...
        char *time = Log.getDateTimeString();
        publish(String("connectedFrom"), String(time));
        publish(String("version"), String(VERSION));
        publish(String("build"), String(String(__DATE__) + " " + String(__TIME__)));
        publish(String("ip"), WiFi.localIP().toString());
        publish(String("startedAt"), String(startedAt));
        publish(String("resetReason"), getResetReason(0));
        publish(String("timeSendData"), String(Configuration._timeSendData));
        publish(String("timeSaveData"), String(Configuration._timeSaveData));
        publish(String("rollerShutterDuration"), String(Configuration._rollerShutterDuration));
        publish(String("solenoidValve"), String("close"));
        publish(String("solenoidValveTimeout"), String(Configuration._solenoidValveTimeout));
        publish(String("solenoidValveMaxWaterQty"), String(Configuration._solenoidValveMaxWaterQty));
        publish(String("solenoidValveMaxWaterLevel"), String(Configuration._solenoidValveMaxWaterLevel));
        publish(String("pumpTimeout"), String(Configuration._pumpTimeout));
        publish(String("lampTimeout"), String(Configuration._lampTimeout));
        publish(String("lightExtFading"), String(Configuration._lightFading));
        // ... and resubscribe
        clientMqtt.subscribe(String(Configuration._hostname + "/set/#").c_str(), 1);
      }
      else
      {
        Log.print("failed, rc=");
        Log.print(String(clientMqtt.state()));
        Log.println(" try again in 5 seconds");
        tick = millis();
      }
    }
  }
}

void Mqtt::callback(char *topic, uint8_t *payload, unsigned int length)
{
  String data;

  Log.print("Message arrived [");
  Log.print(topic);
  Log.print("] ");
  for (unsigned int i = 0; i < length; i++)
  {
    data += (char)payload[i];
  }
  Log.println(data);

  String topicStr(topic);
  topicStr.remove(0, topicStr.lastIndexOf('/') + 1);

  /*********************
   * Global Conf
   *********************/
  if (topicStr == String("timeSendData"))
  {
    int time = data.toInt();
    Log.println(String("set timeSendData to: ") + String(time));
    Configuration._timeSendData = time;
    Configuration.saveConfig();
    publish(String("timeSendData"), String(Configuration._timeSendData));
  }
  else if (topicStr == String("timeSaveData"))
  {
    int time = data.toInt();
    Log.println(String("set timeSaveData to: ") + String(time));
    Configuration._timeSaveData = time;
    Configuration.saveConfig();
    publish(String("timeSaveData"), String(Configuration._timeSaveData));
  }
  else if (topicStr == String("hostname"))
  {
    Log.println("Change hostname to " + data);
    Configuration._hostname = data;
    Configuration.saveConfig();
  }
  else if (topicStr == String("restart"))
  {
    Log.println("Restart ESP !!!");
    ESP.restart();
  }
  else if (topicStr == String("reset"))
  {
    Log.println("Reset ESP and restart !!!");
    ESP_WiFiManager wifiManager;
    wifiManager.resetSettings();
    ESP.restart();
  }

  /*********************
   * Water
   *********************/
  else if (topicStr == String("waterQty1"))
  {
    int qty = data.toInt();
    Log.println("Set waterQty1 to: " + String(qty) + " L");
    Configuration._waterQtyA = qty;
    Configuration.saveConfig();
    publish(String("waterQty1"), String(Configuration._waterQtyA));
  }
  else if (topicStr == String("waterQty2"))
  {
    int qty = data.toInt();
    Log.println("Set waterQty2 to: " + String(qty) + " L");
    Configuration._waterQtyB = qty;
    Configuration.saveConfig();
    publish(String("waterQty2"), String(Configuration._waterQtyB));
  }

  /*********************
   * Roller Shutter
   *********************/
  else if (topicStr == String("rollerShutterDuration"))
  {
    float duration = data.toFloat();
    Log.println("Set rollerShutterDuration to: " + String(duration) + " s");
    rollerShutter.setDuration(duration);
    Configuration._rollerShutterDuration = duration;
    Configuration.saveConfig();
    publish(String("rollerShutterDuration"), String(Configuration._rollerShutterDuration));
  }
  else if (topicStr == String("rollerShutterPosition"))
  {
    float position = data.toFloat();
    if (rollerShutter.getTarget() == position)
      return;
    Log.println("Set rollerShutterPosition to: " + String(position) + " %");
    rollerShutter.setPosition(position);
    Configuration._rollerShutterPosition = position;
    Configuration.saveConfig();
  }
  else if (topicStr == String("rollerShutterOpen"))
  {
    Log.println("Set rollerShutter to Open");
    rollerShutter.open();
  }
  else if (topicStr == String("rollerShutterStop"))
  {
    Log.println("Set rollerShutter to Stop");
    rollerShutter.stop();
  }
  else if (topicStr == String("rollerShutterClose"))
  {
    Log.println("Set rollerShutter to Close");
    rollerShutter.close();
  }

  /*********************
   * Solenoid Valve
   *********************/
  else if (topicStr == String("solenoidValve"))
  {
    Log.println("Set solenoidValve to: " + data);
    if (data == String("open"))
      valve.open();
    else if (data == String("close"))
      valve.close();
  }
  else if (topicStr == String("solenoidValveTimeout"))
  {
    int timeout = data.toInt();
    Log.println("Set solenoidValveTimeout to: " + String(timeout) + " s");
    valve.setTimeout(timeout);
    Configuration._solenoidValveTimeout = timeout;
    Configuration.saveConfig();
    publish(String("solenoidValveTimeout"), String(Configuration._solenoidValveTimeout));
  }
  else if (topicStr == String("solenoidValveMaxWaterQty"))
  {
    int maxQty = data.toInt();
    Log.println("Set solenoidValveMaxWaterQty to: " + String(maxQty) + " L");
    valve.setMaxWaterQuantity(maxQty);
    Configuration._solenoidValveMaxWaterQty = maxQty;
    Configuration.saveConfig();
    publish(String("solenoidValveMaxWaterQty"), String(Configuration._solenoidValveMaxWaterQty));
  }
  else if (topicStr == String("solenoidValveMaxWaterLevel"))
  {
    float maxQty = data.toFloat();
    Log.println("Set solenoidValveMaxWaterLevel to: " + String(maxQty) + " cm");
    valve.setMaxWaterLevel(maxQty);
    Configuration._solenoidValveMaxWaterLevel = maxQty;
    Configuration.saveConfig();
    publish(String("solenoidValveMaxWaterLevel"), String(Configuration._solenoidValveMaxWaterLevel));
  }

  /*********************
   * Pump
   *********************/
  else if (topicStr == String("pump"))
  {
    int state = data.toInt();
    pump.setState(state);
  }
  else if (topicStr == String("pumpTimeout"))
  {
    int timeout = data.toInt();
    Log.println("Set pumpTimeout to: " + String(timeout) + " s");
    Configuration._pumpTimeout = timeout;
    Configuration.saveConfig();
    publish(String("pumpTimeout"), String(Configuration._pumpTimeout));
  }

  /*********************
   * Lamp
   *********************/
  else if (topicStr == String("lamp"))
  {
    int state = data.toInt();
    lamp.setState(state);
  }
  else if (topicStr == String("lampTimeout"))
  {
    int timeout = data.toInt();
    Log.println("Set lampTimeout to: " + String(timeout) + " s");
    Configuration._lampTimeout = timeout;
    Configuration.saveConfig();
    publish(String("lampTimeout"), String(Configuration._lampTimeout));
  }

  /*********************
   * Light
   *********************/
  else if (topicStr == String("lightExt"))
  {
    int value = data.toInt();
    lightExt.setValueInPercent(value);
    Log.println("Set lightExt to: " + String(value) + " %");
    publish(String("lightExt"), String(value));
  }
  else if (topicStr == String("lightExtFading"))
  {
    int fading = data.toInt();
    Log.println("Set lightExtFading speed to: " + String(fading) + " ms");
    lightExt.setFadingSpeed(fading);
    Configuration._lightFading = fading;
    Configuration.saveConfig();
    publish(String("lightExtFading"), String(Configuration._lightFading));
  }

  else
  {
    Log.println("Unknow command");
  }
}

#if !defined(NO_GLOBAL_INSTANCES)
Mqtt MqttClient;
#endif
