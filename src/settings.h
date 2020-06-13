#pragma once

// Version
#define VERSION "V1.0.0"

#define DEFAULT_HOSTNAME "ESP_Pool"
#define DEFAULT_MQTTIPSERVER "192.168.1.201"
#define DEFAULT_MQTTPORTSERVER 1883

#define DEFAULT_NTP_UPDATE_INTERVAL_SEC (1 * 3600) // Update time from NTP server every 1 hours
#define DEFAULT_SEND_DATA_INTERVAL_SEC 5           // Log data every 5 secondes

// Flow Meter (YF-B10)
#define FLOW_1_PIN 14
#define FLOW_2_PIN 12
#define FLOW_CALIB_VALUE 7.5

// Temperature (DS18B20)
#define TEMP_1_PIN 25
#define TEMP_2_PIN 27

// Relay
#define RELAY_1_PIN 23
#define RELAY_2_PIN 22
#define RELAY_3_PIN 19
#define RELAY_4_PIN 18

// DHT
#define DHT_PIN 16

// Water level
#define WATER_LEVEL_PIN 36
#define WATER_LEVEL_COEF_A 0.0048
#define WATER_LEVEL_COEF_B -8.867

// LED
#define LED_PIN 20           //LED_BUILTIN
#define LED_TIME_NOMQTT 0.1f // in seconds
#define LED_TIME_WORK 0.5f   // in seconds

// Timezone
#define UTC_OFFSET +1

// change for different ntp (time servers)
#define NTP_SERVERS "0.fr.pool.ntp.org", "time.nist.gov", "pool.ntp.org"
