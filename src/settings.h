#pragma once

// Version
#define VERSION "V1.1.3"

#define DEFAULT_HOSTNAME "ESP_Pool"
#define DEFAULT_MQTTIPSERVER "192.168.1.201"
#define DEFAULT_MQTTPORTSERVER 1883

#define DEFAULT_SAVE_DATA_INTERVAL_SEC  (1 * 3600)      // in seconds, Update time from NTP server and save data every 1 hours
#define DEFAULT_SEND_DATA_INTERVAL_SEC  15              // in seconds, Log data every 5 secondes

// Flow Meter (YF-B10)
#define FLOW_PIN D5
#define FLOW_COEF_A  1.f / (6.f * 4.5f)
#define FLOW_COEF_B  8.f / 6.f

// Temperature :
// DS18B20
#define DS18B20_PIN D3
// AM2301
#define DHT_PIN D6

// LED
#define LED_PIN LED_BUILTIN
#define LED_TIME_NOMQTT 0.1f // in seconds
#define LED_TIME_WORK 0.5f   // in seconds

// Timezone
#define UTC_OFFSET +1

// change for different ntp (time servers)
#define NTP_SERVERS "0.fr.pool.ntp.org", "time.nist.gov", "pool.ntp.org"
