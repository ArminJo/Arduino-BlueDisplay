#include <stdlib.h>  // For rand()
#include "wifi_settings.h"
#include <BlueDisplay.hpp>
#define COLOR_BACKGROUND COLOR16_WHITE
#define COLOR_FOREGROUND COLOR16_BLACK
//#define DISPLAY_WIDTH 1776
//#define DISPLAY_HEIGHT 999
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
    
//    int y0 = displayHeight +16 ; //
//    int y1 = displayHeight +16*2; // 
//    int y2 = displayHeight +16*3; // 
//    int y3 = displayHeight +16*4; // 

    int y0 = 16 ; //
    int y1 = 16*2; // 
    int y2 = 16*3; // 
    int y3 = 16*4; // 


    // Clear the rectangle under the debug window
    BlueDisplay1.fillRect(x0, 0, 12*16, y3, COLOR_BACKGROUND);
    // frame 
    BlueDisplay1.drawRect(x0, 0, 12*16, y3, COLOR_FOREGROUND,1);

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
    sprintf(sStringBuffer,"%f", graph_min);    
    BlueDisplay1.drawText(xStart+graphWidth-16*8, yStart+graphHeight-8, sStringBuffer,16, COLOR_FOREGROUND, COLOR_BACKGROUND);
    BlueDisplay1.drawLine(xStart, yStart+graphHeight-1, graphWidth, yStart+graphHeight-1, COLOR16_RED);  //boundary

    sprintf(sStringBuffer,"%f", graph_max);    
    BlueDisplay1.drawText(xStart+graphWidth-16*8, yStart, sStringBuffer,16, COLOR_FOREGROUND, COLOR_BACKGROUND);
    BlueDisplay1.drawLine(xStart, yStart, graphWidth, yStart, COLOR16_RED);  //boundary

    sprintf(sStringBuffer,"%f", data[dataSize-1]);    // get last data point
    int nextValidY = yStart + graphHeight - (int)((data[dataSize-1] - graph_min) * yScale);
    BlueDisplay1.drawLine(xStart, nextValidY, graphWidth, nextValidY, COLOR16_LIGHT_GREY);  //gray line with current data
    BlueDisplay1.drawText(xStart+graphWidth-16*8, nextValidY, sStringBuffer,16, COLOR_FOREGROUND, COLOR_BACKGROUND);


}


// Function to add a line to the buffer
void bufferLine(int x1, int y1, int x2, int y2, color16_t color) {
    if (lineBufferIndex < MAX_LINES) {
        lineBuffer[lineBufferIndex].x1 = x1;
        lineBuffer[lineBufferIndex].y1 = y1;
        lineBuffer[lineBufferIndex].x2 = x2;
        lineBuffer[lineBufferIndex].y2 = y2;
        lineBuffer[lineBufferIndex].color = color;
        lineBufferIndex++;
    } else {
        // Handle buffer overflow (e.g., log an error or extend buffer size)
    }
}


// Function to draw a line from the buffer and mark it as drawn - not clearing underneath
void drawBufferedLineNoClr(int index) {
    if (index < lineBufferIndex && lineBuffer[index].color != DRAWN_MAGIC_NUMBER) {
        BlueDisplay1.drawLine(lineBuffer[index].x1, lineBuffer[index].y1, 
                              lineBuffer[index].x2, lineBuffer[index].y2, 
                              lineBuffer[index].color);
        lineBuffer[index].color = DRAWN_MAGIC_NUMBER;  // Mark the line as drawn
    }
}


// Function to draw a line from the buffer and mark it as drawn - clearing rect under the line (height is assumed to be graph height)
void drawBufferedLine(int index) {
    if (index < lineBufferIndex && lineBuffer[index].color != DRAWN_MAGIC_NUMBER) {
        uint16_t x1 = lineBuffer[index].x1;
        uint16_t y1 = lineBuffer[index].y1;
        uint16_t x2 = lineBuffer[index].x2;
        uint16_t y2 = lineBuffer[index].y2;
        
        // Determine the minimum and maximum x values for the rectangle
//        uint16_t xMin = x1 < x2 ? x1 : x2;
//        uint16_t xMax = x1 > x2 ? x1 : x2;
        
        // Define the rectangle width and use the global graph height for height
//        uint16_t rectWidth = xMax - xMin + 1;
        uint16_t rectHeight = globalGraphYPos+globalGraphHeight;  // Use the global graph height

        // Clear the rectangle before drawing the line
//        BlueDisplay1.fillRect(xMin, globalGraphYPos, xMax, rectHeight, COLOR_BACKGROUND);
        BlueDisplay1.fillRect(x1, globalGraphYPos, x2, rectHeight, COLOR_BACKGROUND);

        // Now draw the line
        BlueDisplay1.drawLine(x1, y1, x2, y2, lineBuffer[index].color);
        
        // Mark the line as drawn
        lineBuffer[index].color = DRAWN_MAGIC_NUMBER;
    }
}

// Modified plotGraph function with buffer reset and storing global graph height
// note that there is only one buffer so if you wish to draw two plots at the same time you need to 
// invent new way to create multiple buffers. 
// for now this is simply not implemented and if you want to plot two graphs either use unbuffered plotgraph to plot another
// implement semaphores to make sure new buffer is created once old graph is fully drawn
// or implement more buffers and passing buffer indexes to all the functions. 

void plotGraph_buffered(float *data, uint16_t dataSize, uint16_t graphPosX, uint16_t graphPosY,
               uint16_t graphWidth, uint16_t graphHeight, float graph_min, float graph_max) {
    // Reset the line buffer index
    lineBufferIndex = 0;    
    // Reset the current line index for drawing the buffer
    currentLineIndex = 0; 
   
    // Store the graph height in the global variable
    globalGraphHeight = graphHeight;
    // Store the graph Y position in the global variable 
    globalGraphYPos = graphPosY; 

    uint16_t xStart = graphPosX;
    uint16_t yStart = graphPosY;

    int pointsToPlot = graphWidth;
    int startIndex = dataSize > pointsToPlot ? dataSize - pointsToPlot : 0;

    float xScale = (float)graphWidth / (float)(pointsToPlot - 1);
    float yScale = (float)graphHeight / (graph_max - graph_min);

    int lastX = -1;
    int lastY = -1;
    bool lastValid = false;

    color16_t whiteColor = COLOR16_BLACK;
    color16_t grayColor = COLOR16_BLUE;

    for (int i = startIndex; i < dataSize; i++) {
        if (!isnan(data[i])) {  // Check if the current data point is valid
            int x = xStart + (int)((i - startIndex) * xScale);
            int y = yStart + graphHeight - (int)((data[i] - graph_min) * yScale);

            if (lastValid) {
                bufferLine(lastX, lastY, x, y, whiteColor);  // Buffer the line instead of drawing it immediately
            }

            lastX = x;
            lastY = y;
            lastValid = true;
        } else {
            if (lastValid) {
                int j = i + 1;
                while (j < dataSize && isnan(data[j])) {
                    j++;
                }

                if (j < dataSize) {
                    int nextValidX = xStart + (int)((j - startIndex) * xScale);
                    int nextValidY = yStart + graphHeight - (int)((data[j] - graph_min) * yScale);
                    bufferLine(lastX, lastY, nextValidX, nextValidY, grayColor);  // Buffer the line for missing data
                }
            }
            lastValid = false;
        }
    }

    // Draw the axis boundaries and graph labels
    sprintf(sStringBuffer, "%f", graph_min);    
    BlueDisplay1.drawText(xStart + graphWidth - 16 * 8, yStart + graphHeight - 8, sStringBuffer, 16, COLOR_FOREGROUND, COLOR_BACKGROUND);
//    bufferLine(xStart, yStart + graphHeight - 1, xStart + graphWidth, yStart + graphHeight - 1, COLOR16_RED);
    BlueDisplay1.drawLine(xStart, yStart + graphHeight - 1, xStart + graphWidth, yStart + graphHeight - 1, COLOR16_RED);

    sprintf(sStringBuffer, "%f", graph_max);    
    BlueDisplay1.drawText(xStart + graphWidth - 16 * 8, yStart, sStringBuffer, 16, COLOR_FOREGROUND, COLOR_BACKGROUND);
//    bufferLine(xStart, yStart, xStart + graphWidth, yStart, COLOR16_RED);
    BlueDisplay1.drawLine(xStart, yStart, xStart + graphWidth, yStart, COLOR16_RED);

    // last value 

    uint16_t lastDataY = yStart + graphHeight - (uint16_t)((data[dataSize - 2] - graph_min) * yScale);
    BlueDisplay1.drawLine(xStart, lastDataY, xStart + graphWidth, lastDataY, COLOR_BACKGROUND);  // clear last data line


    sprintf(sStringBuffer, "%f", data[dataSize - 1]);    // Get the last data point
    lastDataY = yStart + graphHeight - (uint16_t)((data[dataSize - 1] - graph_min) * yScale);
    BlueDisplay1.drawText(xStart + graphWidth - 16 * 8, lastDataY, sStringBuffer, 16, COLOR_FOREGROUND, COLOR_BACKGROUND);
//    bufferLine(xStart, lastDataY, xStart + graphWidth, lastDataY, COLOR16_LIGHT_GREY);
    BlueDisplay1.drawLine(xStart, lastDataY, xStart + graphWidth, lastDataY, COLOR16_LIGHT_GREY);
}


// Function to draw all lines from the buffer
void drawAllBufferedLines() {
    for (int i = 0; i < lineBufferIndex; i++) {
        drawBufferedLine(i);
    }
}

// Function to draw a random subset of lines from the buffer
void drawRandomLines(uint16_t numLinesToDraw) {
    if (numLinesToDraw > lineBufferIndex) {
        numLinesToDraw = lineBufferIndex;  // Limit to available lines
    }

    for (uint16_t i = 0; i < numLinesToDraw; i++) {
      uint16_t randomIndex ; 
        for (uint16_t in = 0 ; in < lineBufferIndex; in++){ // retry as many times as lines in buffer 
          randomIndex = rand() % lineBufferIndex;  // Get a random line index
          if (lineBuffer[randomIndex].color != DRAWN_MAGIC_NUMBER) {
            break;    // break if found
          }
        }
        drawBufferedLine(randomIndex);
    }
}

// Function to draw a random subset of lines from the buffer
void drawRandomLinesFast(uint16_t numLinesToDraw) {
    if (numLinesToDraw > lineBufferIndex) {
        numLinesToDraw = lineBufferIndex;  // Limit to available lines
    }

    for (uint16_t i = 0; i < numLinesToDraw; i++) {
      uint16_t randomIndex ; 
          randomIndex = rand() % lineBufferIndex;  // Get a random line index
//          if (lineBuffer[randomIndex].color != DRAWN_MAGIC_NUMBER) {
//            break;    // break if found
//          }       
        drawBufferedLine(randomIndex);
    }
}

// Function to draw a specified number of lines sequentially from the buffer
void drawSequentialLines(int numLinesToDraw) {
    // Draw lines sequentially from the current position
    for (int i = 0; i < numLinesToDraw; i++) {
        if (currentLineIndex < lineBufferIndex) {
            drawBufferedLine(currentLineIndex);
            currentLineIndex++;
        } else {
            // If we reach the end of the buffer, stop drawing
            break;
        }
    }
}

void loop() {
//  struct telemetry_frame tframe ;
 //   BlueDisplay1.clearDisplay();
     if (debug_millis_last<millis()) {      
      DisplayDebug();
      debug_millis_last=millis()+DEBUG_INTERVAL;
     } 
     
    if (minutes_millis_last<millis()) {
      update_minute_buffer();      
      minutes_millis_last=millis()+MINUTES_INTERVAL;
    uint16_t displayWidth = BlueDisplay1.getDisplayWidth();
    uint16_t displayHeight = BlueDisplay1.getDisplayHeight();
//    plotGraph(minutes_buffer, minutes_dataArraySize, GRAPH_X, GRAPH_Y, GRAPH_WIDTH, GRAPH_HEIGHT, minutes_buffer_min, minutes_buffer_max);
    plotGraph_buffered(minutes_buffer, minutes_dataArraySize, GRAPH_X, GRAPH_Y, displayWidth-GRAPH_X, displayHeight-GRAPH_Y, minutes_buffer_min, minutes_buffer_max);
    }
 

//    drawSequentialLines(int numLinesToDraw)
    drawSequentialLines(4);
//    drawRandomLines(4);
    drawRandomLinesFast(4);

    checkAndHandleEvents();
 //   delay(20); // Update delay
//    delay(5000); // Update every 5 seconds
//   delay(10000); // Update every 10 seconds

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
    plotGraph_buffered(minutes_buffer, minutes_dataArraySize, GRAPH_X, GRAPH_Y, displayWidth-GRAPH_X, displayHeight-GRAPH_Y, minutes_buffer_min, minutes_buffer_max);

//    TouchButtonBlinkStartStop.drawButton();
}
