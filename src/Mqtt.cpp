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

void Mqtt::publishData(String topic, String payload)
{
  clientMqtt.publish(String(Configuration._hostname + '/' + topic).c_str(), String(payload).c_str());
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
        clientMqtt.publish(String(Configuration._hostname + "/relay_1").c_str(), String(digitalRead(RELAY_1_PIN)).c_str());
        clientMqtt.publish(String(Configuration._hostname + "/relay_2").c_str(), String(digitalRead(RELAY_2_PIN)).c_str());
        clientMqtt.publish(String(Configuration._hostname + "/relay_3").c_str(), String(digitalRead(RELAY_3_PIN)).c_str());
        clientMqtt.publish(String(Configuration._hostname + "/relay_4").c_str(), String(digitalRead(RELAY_4_PIN)).c_str());
        clientMqtt.publish(String(Configuration._hostname + "/timeIntervalUpdate").c_str(), String(Configuration._timeSendData).c_str());
        clientMqtt.publish(String(Configuration._hostname + "/version").c_str(), String(VERSION).c_str());
        clientMqtt.publish(String(Configuration._hostname + "/build").c_str(), String(String(__DATE__) + " " + String(__TIME__)).c_str());
        clientMqtt.publish(String(Configuration._hostname + "/ip").c_str(), WiFi.localIP().toString().c_str());
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

  if (topicStr == String("relay_1"))
  {
    int status = data.toInt();
    digitalWrite(RELAY_1_PIN, status);
    Log.println(String("set relay 1 to ") + String(status));
    clientMqtt.publish(String(Configuration._hostname + "/relay_1").c_str(), String(status).c_str());
  }
  else if (topicStr == String("relay_2"))
  {
    int status = data.toInt();
    digitalWrite(RELAY_2_PIN, status);
    Log.println(String("set relay 2 to ") + String(status));
    clientMqtt.publish(String(Configuration._hostname + "/relay_2").c_str(), String(status).c_str());
  }
  else if (topicStr == String("relay_3"))
  {
    int status = data.toInt();
    digitalWrite(RELAY_3_PIN, status);
    Log.println(String("set relay 3 to ") + String(status));
    clientMqtt.publish(String(Configuration._hostname + "/relay_3").c_str(), String(status).c_str());
  }
  else if (topicStr == String("relay_4"))
  {
    int status = data.toInt();
    digitalWrite(RELAY_4_PIN, status);
    Log.println(String("set relay 4 to ") + String(status));
    clientMqtt.publish(String(Configuration._hostname + "/relay_4").c_str(), String(status).c_str());
  }
  else if (topicStr == String("timeIntervalUpdate"))
  {
    int time = data.toInt();
    Log.println(String("set timeSendData to ") + String(time));
    Configuration._timeSendData = time;
    Configuration.saveConfig();
    clientMqtt.publish(String(Configuration._hostname + "/timeIntervalUpdate").c_str(), String(Configuration._timeSendData).c_str());
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
  else
  {
    Log.println("Unknow command");
  }
}

#if !defined(NO_GLOBAL_INSTANCES)
Mqtt MqttClient;
#endif
