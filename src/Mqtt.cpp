#include "Mqtt.h"

#include "JsonConfiguration.h"

#include <WiFi.h>
#include <WiFiManager.h>
#include "Logger.h"
#include "RollerShutter.h"
#include "SolenoidValve.h"

WiFiClient espClient;
extern RollerShutter rollerShutter;
extern SolenoidValve valve;

/********************************************************/
/******************** Public Method *********************/
/********************************************************/

Mqtt::Mqtt()
{
  clientMqtt.setClient(espClient);
}

Mqtt::~Mqtt()
{
}

void Mqtt::setup()
{
  clientMqtt.setServer(Configuration._mqttIpServer.c_str(), Configuration._mqttPortServer);
  clientMqtt.setCallback([this](char *topic, uint8_t *payload, unsigned int length) { this->callback(topic, payload, length); });
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

void Mqtt::reconnect()
{
  static unsigned long tick = 0;

  if (!clientMqtt.connected())
  {
    if ((millis() - tick) >= 5000)
    {
      Log.print("Attempting MQTT connection... ");
      // Create a random clientMqtt ID
      String clientId = "ESP8266Client-";
      clientId += String(random(0xffff), HEX);
      // Attempt to connect
      if (clientMqtt.connect(clientId.c_str(), 0, 1, 0, 0))
      {
        Log.println("connected !");
        // Once connected, publish an announcement...
        publish(String("version"), String(VERSION));
        publish(String("build"), String(String(__DATE__) + " " + String(__TIME__)));
        publish(String("ip"), WiFi.localIP().toString());
        publish(String("timeSendData"), String(Configuration._timeSendData));
        publish(String("timeSaveData"), String(Configuration._timeSaveData));
        publish(String("rollerShutterTimeout"), String(Configuration._rollerShutterTimeout));
        publish(String("rollerShutter"), String("stop"));
        publish(String("solenoidValve"), String("close"));
        publish(String("solenoidValveTimeout"), String(Configuration._solenoidValveTimeout));
        publish(String("solenoidValveMaxWaterQty"), String(Configuration._solenoidValveMaxWaterQty));
        publish(String("solenoidValveMaxWaterLevel"), String(Configuration._solenoidValveMaxWaterLevel));
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
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    ESP.restart();
  }
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
  else if (topicStr == String("rollerShutterTimeout"))
  {
    int timeout = data.toInt();
    Log.println("Set rollerShutterTimeout to: " + String(timeout) + " s");
    rollerShutter.setTimeout(timeout);
    Configuration._rollerShutterTimeout = timeout;
    Configuration.saveConfig();
    publish(String("rollerShutterTimeout"), String(Configuration._rollerShutterTimeout));
  }
  else if (topicStr == String("rollerShutter"))
  {
    Log.println("Set rollerShutter to: " + data);
    if (data == String("open"))
      rollerShutter.open();
    else if (data == String("stop"))
      rollerShutter.stop();
    else if (data == String("close"))
      rollerShutter.close();
  }
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
  else
  {
    Log.println("Unknow command");
  }
}

#if !defined(NO_GLOBAL_INSTANCES)
Mqtt MqttClient;
#endif
