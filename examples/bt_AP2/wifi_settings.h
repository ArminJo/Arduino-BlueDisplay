#include <WiFi.h>
#include <AsyncUDP.h>
#include <esp_pm.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
// to disable wifi sleep mode

// Declare a global variable to store the asyncUDP object
AsyncUDP udp;

const char* ssid = "voltage";
const char* password = "irrolling12";
const uint8_t channel = 1; 
const bool hidden = 0;
const uint8_t max_connection = 8; 
const uint16_t beacon_interval = 100; 

uint16_t multicastPort = 5683;  // local port to listen on
IPAddress multicastIP(224,0,1,187);
