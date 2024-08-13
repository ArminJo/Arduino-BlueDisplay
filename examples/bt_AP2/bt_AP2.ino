#include "wifi_settings.h"
#include <BlueDisplay.hpp>
#define COLOR_BACKGROUND COLOR16_WHITE
#define COLOR_FOREGROUND COLOR16_BLACK
#define DISPLAY_WIDTH 1776
#define DISPLAY_HEIGHT 999
#define DO_NOT_NEED_BASIC_TOUCH_EVENTS    // Disables basic touch events like down, move and up. Saves 620 bytes program memory and 36 bytes RAM


// a string buffer for any purpose...
char sStringBuffer[128];

// BlueDisplay object
//BlueDisplay bluedisplay;

#include "telemetry_frame.hpp" // make sure to sync that with sender
                                //hpp to enable #pragma tags


//struct telemetry_frame tframe ; // define a global variable to store telemetry frame
telemetry_frame tframe ; // define a global variable to store telemetry frame

#include "graph_settings.h"


// Declare a function to handle the UDP packet
void handlePacket(AsyncUDPPacket packet);

// Function to update the display with dynamic positioning
void DisplayDebug() {
    int displayWidth = BlueDisplay1.getDisplayWidth();
    int displayHeight = BlueDisplay1.getDisplayHeight();

    int x0 = 0;
    
    int y0 = displayHeight +16 ; //
    int y1 = displayHeight +16*2; // 
    int y2 = displayHeight +16*3; // 
    int y3 = displayHeight +16*4; // 

    BlueDisplay1.drawText(x0, y0, "ESP32 SoftAP", 16, COLOR_FOREGROUND, COLOR_BACKGROUND);
    sprintf(sStringBuffer,"x:%u, y:%u",displayWidth,displayHeight);
    BlueDisplay1.drawText(x0, y1, sStringBuffer,16,COLOR_FOREGROUND, COLOR_BACKGROUND);
//    BlueDisplay1.drawText(x0, y1, "SSID: " + String(ssid),16,COLOR_FOREGROUND, COLOR_BACKGROUND);

//    sprintf(sStringBuffer,"Status: %s", status);
//    BlueDisplay1.drawText(x0, y2, sStringBuffer,16, COLOR_FOREGROUND, COLOR_BACKGROUND);

    if(new_packet) {Serial.println(tframe.voltage_ADC0);}
    
    sprintf(sStringBuffer,"Voltage: %f", tframe.voltage_ADC0);
    BlueDisplay1.drawText(x0, y2, sStringBuffer,16, COLOR_FOREGROUND, COLOR_BACKGROUND);

//     sprintf_P(sStringBuffer, PSTR("%02d"), ip);
        //sStringBuffer = WiFi.localIP().toString().c_str();
       sprintf(sStringBuffer,"IP:%s", WiFi.softAPIP().toString());
       BlueDisplay1.drawText(x0, y3, sStringBuffer,16,COLOR_FOREGROUND, COLOR_BACKGROUND);

    //bluedisplay.flushDisplay();
}

void setup() {
    Serial.begin(115200);

    // Initialize BlueDisplay
//    bluedisplay.begin("ESP32 BlueDisplay");
#if defined(ESP32)
    Serial.begin(115200);
 //   Serial.println(StartMessage);
    initSerial("voltage");
    Serial.println("Start ESP32 BT-client with name \"ESP-BD_Example\"");
#else
    initSerial();
#endif

    BlueDisplay1.initCommunication(&initDisplay, &drawGui);
    checkAndHandleEvents(); // this copies the display size and time from remote device

    delay(500);
    // Start the SoftAP
    Serial.println("Starting SoftAP...");
    WiFi.softAP(ssid, password, channel, hidden, max_connection, beacon_interval);
//    WiFi.softAP(ssid, password);

    delay (200);
    //esp_wifi_set_ps(WIFI_PS_NONE); // disable wifi power saving so packets do not get deferred
                                    //this breaks bluetooth coexistence
    
  // Initialize the asyncUDP object
  if (udp.listenMulticast(multicastIP, multicastPort)) {
    Serial.println("UDP listening");
  delay(500);
    // Set the callback function to handle the UDP packet
    udp.onPacket(handlePacket);
  }



    minutes_millis_last = millis() ; 
    
#ifdef GRAPH_TEST
  for (int i = 0; i < (MINUTES_GRAPH_BUFFER_MAX); i++) {
    // initalize array with bogus values
    minutes_buffer[i]=random(17000)*0.001;
    if (minutes_buffer[i]>14.0){minutes_buffer[i]=NAN;}
    if (minutes_buffer[i]<10.0){minutes_buffer[i]=NAN;}

  }
  minutes_buffer_min= 10.0;
  minutes_buffer_max= 17.0; 
#endif // GRAPH_TEST
}

// Function to handle the UDP packet
IRAM_ATTR void handlePacket(AsyncUDPPacket packet) {
  // Copy the UDP packet data to the pulse data variable
  memcpy((byte*)&tframe, packet.data(), sizeof(tframe));
  new_packet = true; 
  
}

void update_minute_buffer () {
  minutes_buffer_min = minutes_buffer_max; // set graphMin to last graphMax value
  minutes_buffer_max = 0;
  for (int i = 0; i < (MINUTES_GRAPH_BUFFER_MAX - 1); i++) {

    minutes_buffer[i] = minutes_buffer[i + 1];
    if (minutes_buffer[i] > minutes_buffer_max) {
      minutes_buffer_max = minutes_buffer[i];
      }
    if (minutes_buffer[i] < minutes_buffer_min) {
      minutes_buffer_min = minutes_buffer[i];
    }
    // graphMax_oled = max(graphMax_oled, rollingBuffer_oled[i]); 
    // or use that instead 
  }
    // Add the pulse length to the rolling buffer
    if(new_packet) {
      minutes_buffer[MINUTES_GRAPH_BUFFER_MAX-1] = tframe.voltage_ADC0;
      new_packet = false; 
    } else {
      minutes_buffer[MINUTES_GRAPH_BUFFER_MAX-1] = NAN ;      
    }
}

// Function to plot the graph
void plotGraph(float *data, uint16_t dataSize,uint16_t graphPosX,uint16_t graphPosY,
               uint16_t graphWidth, uint16_t graphHeight, float graph_min, float graph_max) {
  uint16_t xStart = graphPosX;
  uint16_t yStart = graphPosY;

  // Calculate the number of data points that can fit within the graph width
  int pointsToPlot = graphWidth;
  int startIndex = dataSize > pointsToPlot ? dataSize - pointsToPlot : 0;

  float xScale = (float)graphWidth / (float)(pointsToPlot - 1);
  float yScale = (float)graphHeight / (graph_max - graph_min);

  int lastX = -1;
  int lastY = -1;
  bool lastValid = false;

  color16_t whiteColor = COLOR16_BLACK;
  color16_t grayColor =  COLOR16_BLUE;

    sprintf(sStringBuffer,"%f", graph_min);    
    BlueDisplay1.drawText(xStart, yStart+graphHeight-8, sStringBuffer,16, COLOR_FOREGROUND, COLOR_BACKGROUND);
    BlueDisplay1.drawLine(xStart, yStart+graphHeight-1, graphWidth, yStart+graphHeight-1, COLOR16_RED);  //boundary

    sprintf(sStringBuffer,"%f", graph_max);    
    BlueDisplay1.drawText(xStart, yStart, sStringBuffer,16, COLOR_FOREGROUND, COLOR_BACKGROUND);
    BlueDisplay1.drawLine(xStart, yStart, graphWidth, yStart, COLOR16_RED);  //boundary


  for (int i = startIndex; i < dataSize; i++) {
    if (!isnan(data[i])) {  // Check if the current data point is valid
      int x = xStart + (int)((i - startIndex) * xScale);
      int y = yStart + graphHeight - (int)((data[i] - graph_min) * yScale);

      if (lastValid) {
        BlueDisplay1.drawLine(lastX, lastY, x, y, whiteColor);  // Draw a line between the last valid point and the current point using white color
      }

      lastX = x;
      lastY = y;
      lastValid = true;
    } else {
      if (lastValid) {
        // Draw a gray line to mark missing data if the last point was valid
        int j = i + 1;
        while (j < dataSize && isnan(data[j])) {
          j++;
        }

        if (j < dataSize) {
          int nextValidX = xStart + (int)((j - startIndex) * xScale);
          int nextValidY = yStart + graphHeight - (int)((data[j] - graph_min) * yScale);
          BlueDisplay1.drawLine(lastX, lastY, nextValidX, nextValidY, grayColor);  // Use gray color for missing data indicator
        }
      }
      lastValid = false;
    }
  }
}

void loop() {
//  struct telemetry_frame tframe ;
    BlueDisplay1.clearDisplay();
    DisplayDebug();
    if (minutes_millis_last<millis()) {
      update_minute_buffer();      
      minutes_millis_last=millis()+MINUTES_INTERVAL;
    }
    uint16_t displayWidth = BlueDisplay1.getDisplayWidth();
    uint16_t displayHeight = BlueDisplay1.getDisplayHeight();
//    plotGraph(minutes_buffer, minutes_dataArraySize, GRAPH_X, GRAPH_Y, GRAPH_WIDTH, GRAPH_HEIGHT, minutes_buffer_min, minutes_buffer_max);
//    checkAndHandleEvents();
    plotGraph(minutes_buffer, minutes_dataArraySize, GRAPH_X, GRAPH_Y, displayWidth-GRAPH_X, displayHeight-GRAPH_Y, minutes_buffer_min, minutes_buffer_max);
    checkAndHandleEvents();


//    delay(1000); // Update every 1 seconds
//    delay(5000); // Update every 5 seconds
    delay(10000); // Update every 10 seconds

}

void initDisplay(void) {
    // Initialize display size and flags
    uint16_t displayWidth = BlueDisplay1.getMaxDisplayWidth();
    uint16_t displayHeight = BlueDisplay1.getMaxDisplayHeight();    
    //BlueDisplay1.setFlagsAndSize(BD_FLAG_FIRST_RESET_ALL | BD_FLAG_USE_MAX_SIZE | BD_FLAG_TOUCH_BASIC_DISABLE, DISPLAY_WIDTH,
//    DISPLAY_HEIGHT);
    BlueDisplay1.setFlagsAndSize(BD_FLAG_FIRST_RESET_ALL | BD_FLAG_USE_MAX_SIZE | BD_FLAG_TOUCH_BASIC_DISABLE, displayWidth,displayHeight);

    // Initialize button position, size, colors etc.
//    TouchButtonBlinkStartStop.init((DISPLAY_WIDTH - BUTTON_WIDTH_2) / 2, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_2,
//    BUTTON_HEIGHT_4, COLOR16_BLUE, "Start", 44, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN, doBlink,
//            &doBlinkStartStop);
//    TouchButtonBlinkStartStop.setCaptionForValueTrue("Stop");

//    BlueDisplay1.debug(StartMessage);
}

/*
 * Function is called for resize + connect too
 */
void drawGui(void) {
    uint16_t displayWidth = BlueDisplay1.getMaxDisplayWidth();
    uint16_t displayHeight = BlueDisplay1.getMaxDisplayHeight();    
    //BlueDisplay1.setFlagsAndSize(BD_FLAG_FIRST_RESET_ALL | BD_FLAG_USE_MAX_SIZE | BD_FLAG_TOUCH_BASIC_DISABLE, DISPLAY_WIDTH,
//    DISPLAY_HEIGHT);
    BlueDisplay1.setFlagsAndSize(BD_FLAG_FIRST_RESET_ALL | BD_FLAG_USE_MAX_SIZE | BD_FLAG_TOUCH_BASIC_DISABLE, displayWidth,displayHeight);

    BlueDisplay1.clearDisplay(COLOR16_BLUE);
//    TouchButtonBlinkStartStop.drawButton();
}
