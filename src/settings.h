#pragma once

// Version
#define VERSION "V1.2.0"

#define DEFAULT_HOSTNAME "ESP_Pool"
#define DEFAULT_MQTTIPSERVER "192.168.1.201"
#define DEFAULT_MQTTPORTSERVER 1883

#define DEFAULT_SAVE_DATA_INTERVAL_SEC (1 * 3600)   // in seconds, Update time from NTP server and save data every 1 hours
#define DEFAULT_SEND_DATA_INTERVAL_SEC 15           // in seconds, Log data every 5 secondes
#define DEFAULT_ROLLER_SHUTTER_TIMEOUT 60           // in seconds, 1 minutes timeout
#define DEFAULT_SOLENOID_VALVE_TIMEOUT (5 * 60)     // in seconds, 5 minutes timeout
#define DEFAULT_SOLENOID_VALVE_MAX_QTY_WATER 200    // in L, 200 L max
#define DEFAULT_SOLENOID_VALVE_MAX_LEVEL_WATER 10.f // in cm, 10 cm max

// Flow Meter (YF-B10)
#define FLOW_1_PIN 14
#define FLOW_2_PIN 12
#define FLOW_COEF_A 1.f / (6.f * 4.5f)
#define FLOW_COEF_B 8.f / 6.f

// Temperature :
// DS18B20
#define DS18B20_PIN 16
// AM2301
#define DHT_1_PIN 25
#define DHT_2_PIN 27

// Relay
#define RELAY_1_PIN 17
#define RELAY_2_PIN 21
#define RELAY_3_PIN 23
#define RELAY_4_PIN 22
#define RELAY_5_PIN 19
#define RELAY_6_PIN 18

// Water level
#define WATER_LEVEL_PIN 36
#define WATER_LEVEL_COEF_A 0.0048
#define WATER_LEVEL_COEF_B -8.867

// PWM
#define PWM_LAMP_EXT_PIN 13
#define PWM_FREQUENCY 1000 // in Hz

// LED
#define LED_PIN 20           // LED_BUILTIN
#define LED_TIME_NOMQTT 0.1f // in seconds
#define LED_TIME_WORK 0.5f   // in seconds

// Timezone
#define UTC_OFFSET +1

// change for different ntp (time servers)
#define NTP_SERVERS "0.fr.pool.ntp.org", "time.nist.gov", "pool.ntp.org"
#define EPOCH_1_1_2019 1546300800
