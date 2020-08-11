#include <LittleFS.h>

#include "JsonConfiguration.h"
#include "Logger.h"

/********************************************************/
/******************** Public Method *********************/
/********************************************************/

JsonConfiguration::JsonConfiguration()
{
}

JsonConfiguration::~JsonConfiguration()
{
}

void JsonConfiguration::setup(void)
{
  /* Initialize LittleFS */
  if (!LittleFS.begin())
  {
    Log.println("failed to initialize LittleFS, try to format...");
    if (LittleFS.format())
    {
      Log.println("failed to format !");
    }
    else
    {
      Log.println("Format success !");
      LittleFS.begin();
    }
  }

  if (!readConfig())
  {
    Log.println("Invalid configuration values, restoring default values");
    restoreDefault();
  }

  Log.println(String("    hostname: ") + _hostname);
  Log.println(String("    mqttIpServer: ") + _mqttIpServer);
  Log.println(String("    mqttPortServer: ") + String(_mqttPortServer));
  Log.println(String("    timeSaveData: ") + String(_timeSaveData));
  Log.println(String("    timeSendData: ") + String(_timeSendData));
  Log.println(String("    waterTemp: ") + String(_waterTemp));
  Log.println(String("    waterQty: ") + String(_waterQty));
  Log.println(String("    temp: ") + String(_temp));
  Log.println(String("    humidity: ") + String(_humidity));
}

bool JsonConfiguration::readConfig()
{
  Log.println("Read Configuration file from LittleFS...");

  // Open file
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile)
  {
    Log.println("Failed to open config file");
    return false;
  }

  uint16_t size = configFile.size();

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  configFile.readBytes(buf.get(), size);
  decodeJsonFromFile(buf.get());

  configFile.close();

  return true;
}

bool JsonConfiguration::saveConfig()
{
  DynamicJsonDocument doc(512);

  encodeToJson(doc);

  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile)
  {
    Log.println("Failed to open config file for writing");
    return false;
  }

  // Serialize JSON to file
  if (serializeJson(doc, configFile) == 0)
  {
    Log.println(F("Failed to write to file"));
    return false;
  }

  configFile.close();

  Log.println("Save config successfully");

  return true;
}

void JsonConfiguration::restoreDefault()
{
  _hostname = DEFAULT_HOSTNAME;
  _mqttIpServer = DEFAULT_MQTTIPSERVER;
  _mqttPortServer = DEFAULT_MQTTPORTSERVER;
  _timeSaveData = DEFAULT_SAVE_DATA_INTERVAL_SEC;
  _timeSendData = DEFAULT_SEND_DATA_INTERVAL_SEC;
  _waterQty = 0;
  _waterTemp = 0;
  _temp = 0;
  _humidity = 0;

  saveConfig();
  Log.println("configuration restored.");
}

void JsonConfiguration::encodeToJson(JsonDocument &_json)
{
  _json.clear();
  _json["hostname"] = _hostname;
  _json["mqttIpServer"] = _mqttIpServer;
  _json["mqttPortServer"] = _mqttPortServer;
  _json["timeSaveData"] = _timeSaveData;
  _json["timeSendData"] = _timeSendData;
  _json["waterQty"] = _waterQty;
  _json["waterTemp"] = _waterTemp;
  _json["temp"] = _temp;
  _json["humidity"] = _humidity;
}

uint8_t JsonConfiguration::decodeJsonFromFile(const char *input)
{
  DynamicJsonDocument doc(1024);
  doc.clear();

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, input);
  if (error)
  {
    Serial.print("Failed to deserialize JSON, error: ");
    Serial.println(error.c_str());
    // restoreDefault();
    return -1;
  }

  _hostname = doc["hostname"].as<String>();
  if (_hostname == NULL)
    _hostname = DEFAULT_HOSTNAME;
  _mqttIpServer = doc["mqttIpServer"].as<String>();
  if (_mqttIpServer == NULL)
    _mqttIpServer = DEFAULT_MQTTIPSERVER;
  _mqttPortServer = doc["mqttPortServer"].as<uint16_t>();
  _timeSaveData = doc["timeSaveData"].as<uint16_t>();
  _timeSendData = doc["timeSendData"].as<uint16_t>();
  _waterQty = doc["waterQty"].as<float>();
  _waterTemp = doc["waterTemp"].as<float>();  
  _temp = doc["temp"].as<float>();  
  _humidity = doc["humidity"].as<float>();  

  return 0;
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/

#if !defined(NO_GLOBAL_INSTANCES)
JsonConfiguration Configuration;
#endif
