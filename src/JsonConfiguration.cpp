#include <SPIFFS.h>

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
  /* Initialize SPIFFS */
  if (!SPIFFS.begin(true))
  {
    Log.println("failed to initialize SPIFFS");
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
  Log.println(String("    waterQtyA: ") + String(_waterQtyA));
  Log.println(String("    waterQtyB: ") + String(_waterQtyB));
  Log.println(String("    waterLevel: ") + String(_waterLevel));
  Log.println(String("    waterTemp: ") + String(_waterTemp));
  Log.println(String("    intTemp: ") + String(_intTemp));
  Log.println(String("    intHumidity: ") + String(_intHumidity));
  Log.println(String("    extTemp: ") + String(_extTemp));
  Log.println(String("    extHumidity: ") + String(_extHumidity));
  Log.println(String("    rollerShutterTimeout: ") + String(_rollerShutterTimeout));
  Log.println(String("    solenoidValveTimeout: ") + String(_solenoidValveTimeout));
  Log.println(String("    solenoidValveWaterQty: ") + String(_solenoidValveMaxWaterQty));
  Log.println(String("    solenoidValveWaterLevel: ") + String(_solenoidValveMaxWaterLevel));
  Log.println(String("    pumpTimeout: ") + String(_pumpTimeout));
  Log.println(String("    lampTimeout: ") + String(_lampTimeout));
}

bool JsonConfiguration::readConfig()
{
  Log.println("Read Configuration file from SPIFFS...");

  // Open file
  File configFile = SPIFFS.open("/config.json", "r");
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

  File configFile = SPIFFS.open("/config.json", "w");
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
  _waterQtyA = _waterQtyB = 0;
  _waterLevel = _waterTemp;
  _intTemp = _extTemp = 0;
  _intHumidity = _extHumidity = 0;
  _rollerShutterTimeout = DEFAULT_ROLLER_SHUTTER_TIMEOUT;
  _solenoidValveTimeout = DEFAULT_SOLENOID_VALVE_TIMEOUT;
  _solenoidValveMaxWaterQty = DEFAULT_SOLENOID_VALVE_MAX_QTY_WATER;
  _solenoidValveMaxWaterLevel = DEFAULT_SOLENOID_VALVE_MAX_LEVEL_WATER;
  _pumpTimeout = _lampTimeout = 0;

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
  _json["waterQtyA"] = _waterQtyA;
  _json["waterQtyB"] = _waterQtyB;
  _json["waterLevel"] = _waterLevel;
  _json["waterTemp"] = _waterTemp;
  _json["intTemp"] = _intTemp;
  _json["intHumidity"] = _intHumidity;
  _json["extTemp"] = _extTemp;
  _json["extHumidity"] = _extHumidity;
  _json["rollerShutterTimeout"] = _rollerShutterTimeout;
  _json["solenoidValveTimeout"] = _solenoidValveTimeout;
  _json["solenoidValveMaxQtyWater"] = _solenoidValveMaxWaterQty;
  _json["solenoidValveMaxLevelWater"] = _solenoidValveMaxWaterLevel;
  _json["pumpTimeout"] = _pumpTimeout;
  _json["lampTimeout"] = _lampTimeout;
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
  _waterQtyA = doc["waterQtyA"].as<float>();
  _waterQtyB = doc["waterQtyB"].as<float>();  
  _waterLevel = doc["waterLevel"].as<float>();  
  _waterTemp = doc["waterTemp"].as<float>();  
  _intTemp = doc["intTemp"].as<float>();  
  _intHumidity = doc["intHumidity"].as<float>();  
  _extTemp = doc["extTemp"].as<float>();  
  _extHumidity = doc["extHumidity"].as<float>();  
  _rollerShutterTimeout = doc["rollerShutterTimeout"].as<uint16_t>();
  _solenoidValveTimeout = doc["solenoidValveTimeout"].as<uint16_t>();
  _solenoidValveMaxWaterQty = doc["solenoidValveMaxQtyWater"].as<uint16_t>();
  _solenoidValveMaxWaterLevel = doc["solenoidValveMaxLevelWater"].as<float>();
  _pumpTimeout = doc["pumpTimeout"].as<uint16_t>();
  _lampTimeout = doc["lampTimeout"].as<uint16_t>();

  return 0;
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/

#if !defined(NO_GLOBAL_INSTANCES)
JsonConfiguration Configuration;
#endif
