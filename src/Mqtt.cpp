#include "Mqtt.h"

#include "JsonConfiguration.h"

#include <WiFi.h>
#include <WiFiManager.h>
#include "Logger.h"

WiFiClient espClient;

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
      if (clientMqtt.connect(clientId.c_str()))
      {
        Log.println("connected !");
        // Once connected, publish an announcement...

        publish(String("version"), String(VERSION));
        publish(String("build"), String(String(__DATE__) + " " + String(__TIME__)));
        publish(String("ip"), WiFi.localIP().toString());
        publish(String("timeSendData"), String(Configuration._timeSendData));
        publish(String("timeSaveData"), String(Configuration._timeSaveData));
        publish(String("relay1"), String(digitalRead(RELAY_1_PIN)));
        publish(String("relay2"), String(digitalRead(RELAY_2_PIN)));
        publish(String("relay3"), String(digitalRead(RELAY_3_PIN)));
        publish(String("relay4"), String(digitalRead(RELAY_4_PIN)));
        publish(String("waterQty1"), String(Configuration._waterQtyA));
        publish(String("waterQty2"), String(Configuration._waterQtyB));
        // ... and resubscribe
        clientMqtt.subscribe(String(Configuration._hostname + "/set/#").c_str());
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
    Log.print(String(payload[i]));
    data += (char)payload[i];
  }
  Log.println();

  String topicStr(topic);
  topicStr.remove(0, topicStr.lastIndexOf('/') + 1);

  if (topicStr == String("relay1"))
  {
    int status = data.toInt();
    digitalWrite(RELAY_1_PIN, status);
    Log.println(String("set relay 1 to: ") + String(status));
    publish("relay1", String(status));
  }
  else if (topicStr == String("relay2"))
  {
    int status = data.toInt();
    digitalWrite(RELAY_2_PIN, status);
    Log.println(String("set relay 2 to: ") + String(status));
    publish("relay2", String(status));
  }
  else if (topicStr == String("relay3"))
  {
    int status = data.toInt();
    digitalWrite(RELAY_3_PIN, status);
    Log.println(String("set relay 3 to: ") + String(status));
    publish("relay3", String(status));
  }
  else if (topicStr == String("relay4"))
  {
    int status = data.toInt();
    digitalWrite(RELAY_4_PIN, status);
    Log.println(String("set relay 4 to ") + String(status));
    publish("relay4", String(status));
  }
  else if (topicStr == String("timeSendData"))
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
  else
  {
    Log.println("Unknow command");
  }
}

#if !defined(NO_GLOBAL_INSTANCES)
Mqtt MqttClient;
#endif
