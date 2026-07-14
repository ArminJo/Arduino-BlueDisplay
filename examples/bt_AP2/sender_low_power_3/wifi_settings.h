//WIFI and UDP multicast includes
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const char* ssid = "voltage";
const char* password = "irrolling12";
const char channel = 1; 
const char hidden = 0;
const char max_connection = 8; 
const int beacon_interval = 500; 
const int timeout_period = 1000; // expected delay inbetween packets (for accounting display) 
//#define AP_mode_on 1 ; // set to 1 to compile AP node

WiFiUDP udp;
const int port = 5683;

// IPAddress broadcast;
IPAddress broadcast=IPAddress(224, 0, 1, 187);

#define PERSISTENT_WIFI  // attempt to use tricks to quickly reconnect to wifi 

#ifdef PERSISTENT_WIFI
struct WIFI_SETTINGS_T {
  uint16_t magic;
  uint32_t ip_address;
  uint32_t ip_gateway;
  uint32_t ip_mask;
  uint32_t ip_dns1;
  uint32_t ip_dns2;
  char wifi_ssid[50];
  char wifi_auth[50];
  uint8_t wifi_bssid[6];
  uint16_t wifi_channel;
}; // size = 132 bytes
#endif //PERSISTENT_WIFI
