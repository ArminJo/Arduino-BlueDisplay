
// simple POC : send hello world string over UDP-multicast address every 2 seconds.
// a very simple way to connect IOT to local network without need of any server.
// author : Marc Quinton / feb 2019
// keywords : esp8266, arduino, multicast, IOT, example, POC

// this version is attempting to deep sleep and reconnect by using persistent wifi method of esp8266
// this method was documented in : https://johnmu.com/2022-esp8266-wifi-speed/

#include "wifi_settings.h"
#include "telemetry_frame.h"

// ADC pin
const int analogInPin = A0;

//LED pin

#define LED_PIN 2 // use GPIO2 (bootselector/LED pin)
//#define MODE_PIN 2 // use GPIO4 (i2c pin)
#define MODE_PIN 4 // use GPIO4 (i2c pin)

#ifdef PERSISTENT_WIFI
struct WIFI_SETTINGS_T _settings;
#endif //PERSISTENT_WIFI

//light sleep extras
#include "user_interface.h"
void fpm_wakup_cb_func(void) {
  Serial.println(F("Light sleep is over"));
  Serial.flush();  
  wifi_fpm_close();


}


void setup(void)
{
  Serial.begin(115200);
//  Serial.println("Startup");

#ifndef PERSISTENT_WIFI  // attempt to use tricks to quickly reconnect to wifi 
  WiFi.persistent(false); // no need to wear off the flash as we have all data in the sketch
  WiFi.mode(WIFI_STA);
 // WiFi.softAP(ssid, password, channel, hidden, max_connection);
  WiFi.begin(ssid, password);
#endif //PERSISTENT_WIFI

#ifdef PERSISTENT_WIFI
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
// blink led until connected  
    pinMode(LED_PIN, OUTPUT);
    while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_PIN,LOW);
    delay(200);
    digitalWrite(LED_PIN,HIGH);
    delay(200);
    }
    pinMode(LED_PIN, INPUT);
    pinMode(MODE_PIN, INPUT);

WiFi.persistent(true);
WiFi.mode(WIFI_STA);

  WiFi.begin(ssid,password, 0, NULL, true);
//  uint32_t timeout = millis() + 15000; // max 15s 
//  while ((WiFi.status() != WL_CONNECTED) && (millis()<timeout)) { delay(5); }

// blink led until connected (because this is sensor and can fail sometimes)
    pinMode(LED_PIN, OUTPUT);
    while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_PIN,LOW);
    delay(100);
    digitalWrite(LED_PIN,HIGH);
    delay(100);
    }
    pinMode(LED_PIN, INPUT);
    pinMode(MODE_PIN, INPUT);

  if (WiFi.status() == WL_CONNECTED) { // save wifi settings
    _settings.ip_address = WiFi.localIP();
    _settings.ip_gateway = WiFi.gatewayIP();
    _settings.ip_mask = WiFi.subnetMask();
    _settings.ip_dns1 = WiFi.dnsIP(0);
    _settings.ip_dns2 = WiFi.dnsIP(1);
    memcpy(_settings.wifi_bssid, WiFi.BSSID(), 6);
    _settings.wifi_channel = WiFi.channel();
  }

//uint32_t timeout = millis() + FAST_TIMEOUT;
//while ((WiFi.status() != WL_CONNECTED) && (millis()<timeout)) { delay(10); }

#endif //  PERSISTENT_WIFI


// connected or timed out
    /**
    * datasheet:
    *
   wifi_set_sleep_level():
   Set sleep level of modem sleep and light sleep
   This configuration should be called before calling wifi_set_sleep_type
   Modem-sleep and light sleep mode have minimum and maximum sleep levels.
   - In minimum sleep level, station wakes up at every DTIM to receive
     beacon.  Broadcast data will not be lost because it is transmitted after
     DTIM.  However, it can not save much more power if DTIM period is short,
     as specified in AP.
   - In maximum sleep level, station wakes up at every listen interval to
     receive beacon.  Broadcast data may be lost because station may be in sleep
     state at DTIM time.  If listen interval is longer, more power will be saved, but
     it’s very likely to lose more broadcast data.
   - Default setting is minimum sleep level.
   Further reading: https://routerguide.net/dtim-interval-period-best-setting/

   wifi_set_listen_interval():
   Set listen interval of maximum sleep level for modem sleep and light sleep
   It only works when sleep level is set as MAX_SLEEP_T
   forum: https://github.com/espressif/ESP8266_NONOS_SDK/issues/165#issuecomment-416121920
   default value seems to be 3 (as recommended by https://routerguide.net/dtim-interval-period-best-setting/)

   call order:
     wifi_set_sleep_level(MAX_SLEEP_T) (SDK3)
     wifi_set_listen_interval          (SDK3)
     wifi_set_sleep_type               (all SDKs)

    */

  
//  WiFi.setSleepMode(WIFI_LIGHT_SLEEP, 10);
//  WiFi.setSleepMode(WIFI_MODEM_SLEEP, 10);

  //WiFi.setPhyMode(WIFI_PHY_MODE_11B); // max range 
//  Serial.println("");

//  Serial.print("broadcast address : ");
//  Serial.print(broadcast);
//  Serial.print(":");
//  Serial.println(port);

// blink led until connected  
    pinMode(LED_PIN, OUTPUT);
    while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_PIN,LOW);
    delay(200);
    digitalWrite(LED_PIN,HIGH);
    delay(200);
    }
    pinMode(LED_PIN, INPUT);
    pinMode(MODE_PIN, INPUT);

// deeper sleep pin - high makes device update less often and
// sleep in meantime
// low updates with 20ms intervals. 

  udp.begin(port);
wifi_fpm_set_wakeup_cb(fpm_wakup_cb_func);

}

int UDP_packets = 0 ; 
void loop(void)
{
telemetry_frame tframe ;
  tframe.voltage_ADC0 = analogRead(analogInPin) * (17.0/1024);

  udp.beginPacketMulticast(broadcast, port, WiFi.localIP());
  udp.write((byte*)&tframe,sizeof(tframe));
  udp.endPacket();

  delay (10); //wait enough time for the packet to be transmitted... TODO : fix this with something better? 
  
//removed for debug purposes
//  if (digitalRead(MODE_PIN) == HIGH) {  //only visualise RSSI if no high speed mode is used 
//                                        // as it introduces useless delay otherwise 
//  int8_t rssi = WiFi.RSSI();
//  uint16_t intensity_blue = map (rssi, -60,-95, 255,0);
//  intensity_blue = constrain(intensity_blue, 0, 255);
//    pinMode(LED_PIN, OUTPUT);
//    digitalWrite(LED_PIN,LOW);
//    delay(intensity_blue); // signal 
//    digitalWrite(LED_PIN,HIGH);
//    pinMode(MODE_PIN, INPUT);
//    delay(20);
//  }  

  UDP_packets++;

  if (digitalRead(MODE_PIN) == HIGH){

//   WiFi.setSleepMode(WIFI_LIGHT_SLEEP, 10);
//   WiFi.forceSleepBegin();  // Wifi Off
//     WiFi.disconnect(); 
//     delay(100);
     Serial.flush();
     WiFi.mode(WIFI_OFF);
//    delay (100);
//     delay(1000*2); // sleep for 10 seconds
//     WiFi.mode(WIFI_ON);
//   WiFi.forceSleepWake();  // Wifi On

//light sleep mode example
extern os_timer_t *timer_list;
timer_list = nullptr; 

wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
wifi_fpm_open();

//wifi_fpm_set_wakeup_cb(fpm_wakup_cb_func);
//optional : more wakeup sources
//gpio_pin_wakeup_enable(MODE_PIN,GPIO_PIN_INTR_HILEVEL);
//defined by level.

long sleepTimeMilliSeconds = 10*1000;
//light_sleep function requires microseconds
wifi_fpm_do_sleep(sleepTimeMilliSeconds * 1000); esp_delay(sleepTimeMilliSeconds+1);

//timed light sleep mode is only entered when the sleep command is followed
//by a delay() that os at least 1ms longer than the sleep 
//delay(5000);
//delay(sleepTimeMilliSeconds + 1);
// code should continue here.


//  uint32_t timeout = millis() + 2000; // max 15s
//      while ((WiFi.status() != WL_CONNECTED) && (millis()<timeout)) { 
//      while (WiFi.status() != WL_CONNECTED) {
//        pinMode(LED_PIN, OUTPUT);
//        digitalWrite(LED_PIN,LOW);
//        delay(2);
//        digitalWrite(LED_PIN,HIGH);
//        delay(80);
//        pinMode(LED_PIN, INPUT);
//      }

//        delay(20); 

  if (WiFi.status() != WL_CONNECTED) { // if connection got lost - reconnect
  WiFi.persistent(true); //should be set to true but we try to save the flash and verify if it still works without 
  WiFi.mode(WIFI_STA);
  WiFi.config(_settings.ip_address, _settings.ip_gateway, _settings.ip_mask);
  WiFi.begin(ssid,password, _settings.wifi_channel, _settings.wifi_bssid, true);
  uint32_t timeout = millis() + 20000; // max 20s 
//  while ((WiFi.status() != WL_CONNECTED) && (millis()<timeout)) { delay(5); }
      while ((WiFi.status() != WL_CONNECTED) && (millis()<timeout)) { 
//      while (WiFi.status() != WL_CONNECTED) {
        pinMode(LED_PIN, OUTPUT);
        digitalWrite(LED_PIN,LOW);
        delay(1);
        digitalWrite(LED_PIN,HIGH);
        delay(10);
        pinMode(LED_PIN, INPUT);
      }
    }
  //  FIXME -- it all does not work, after few reconnects
  //  with weak signal, it ends up disconnected. 
  //  reconnects eat up more energy than expected too 
  
  }
  else {
    delay (10); // no sleep mode, send packets as quick as reasonably possible. 
    //3ms with ESP32 as AP . not bad!
    //5ms with ESP32 as AP is most reliable. 
    
    //esp8266 as AP has some weird hiccups and while esp8266 AP itself gets the packets fine down to 10ms range,
      //clients of esp8266 AP get packets in bursts dependable on beacon interval. the rate for clients is still much slower than 100ms
      
    
  }
  
}
