#pragma once

// Version
#define VERSION                     "V1.0.0"

#define DEFAULT_HOSTNAME            "ESP_Pool"
#define DEFAULT_MQTTIPSERVER        "192.168.1.201"
#define DEFAULT_MQTTPORTSERVER      1883

#define DEFAULT_NTP_UPDATE_INTERVAL_SEC     (1 * 3600)      // Update time from NTP server every 1 hours
#define DEFAULT_SEND_DATA_INTERVAL_SEC      5               // Log data every 5 secondes

// Flow Meter (YF-B10)
#define FLOW_1_PIN      22
#define FLOW_2_PIN      21

// Temperature (DS18B20)
#define TEMP_1_PIN      23
#define TEMP_2_PIN      34

// Relay
#define RELAY_1_PIN     18
#define RELAY_2_PIN     19
#define RELAY_3_PIN     33
#define RELAY_4_PIN     35

// LED
#define LED_PIN             LED_BUILTIN
#define LED_TIME_NOMQTT     0.1f    // in seconds
#define LED_TIME_WORK       0.5f    // in seconds

// Timezone
#define UTC_OFFSET + 1

// change for different ntp (time servers)
#define NTP_SERVERS "0.fr.pool.ntp.org", "time.nist.gov", "pool.ntp.org"
