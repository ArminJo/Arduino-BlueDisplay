#include <Arduino.h>
#include "BlueDisplay.hpp"

#define AMOUNT_OF_REQUESTS_PROCESSED 32 // how many touch requests per one loop iteration
                                      //as touch requests come in chunks, this prevents deferred requests.
                                      
#define ARRAY_SIZE 20
#define ARRAY_CLAMP_VALUE 255 // max possible value of array item
#define BAR_COLOR COLOR16_BLACK
#define SELECTED_BAR_COLOR COLOR16_RED
#define BAR_BOUNDARY COLOR16_RED
#define BACKGROUND_COLOR COLOR16_WHITE
#define LABEL_FONT_SIZE 24 //  size of the font of the labels

// Bar plot positioning and size as percentages
float plotXPercent = 10.0;   // X position as a percentage of screen width
float plotYPercent = 10.0;   // Y position as a percentage of screen height
float plotWidthPercent = 80.0;  // Width as a percentage of screen width
float plotHeightPercent = 80.0; // Height as a percentage of screen height
float sensitivity  ; // sensitivity of the input
float touchedDelta =0 ; // last delta of the touched spot - accumulated since touch down
#define SENSITIVITY_MULTIPLIER 0.1 // define multiplier for dynamic sensitivity 
  // dynamic sensitivity means than the longer path got moved in-between updates (the faster finger moves) the more effect it has on change of value.
  

//uint8_t dataArray[ARRAY_SIZE] = {20, 40, 60, 80, 100, 80, 60, 40, 20, 10};
uint8_t dataArray[ARRAY_SIZE] ;
//uint8_t lastArray[ARRAY_SIZE];

char sStringBuffer[128];

void initDisplay(void);
void drawBars();
void updateBars(uint8_t index, uint8_t newValue);
void handleTouchMove(struct TouchEvent *const aCurrentPositionPtr);

uint16_t plotX, plotY, plotWidth, plotHeight;
uint16_t barWidth;
uint8_t touchedIndex; // remember to se the type to reflect array size
int touchEventIndex = -1 ; // -1 means none
int lastTouchedIndex = -1 ; // for changing color of last touched bar

static struct XYPosition sLastPos;

void setup() {
    Serial0.begin(115200);
    initSerial("array_example");
    BlueDisplay1.initCommunication(&initDisplay, &drawGui);
    registerTouchMoveCallback(&handleTouchMove);
    registerTouchDownCallback(&handleTouchDown);
    
}

void drawArray(uint16_t chunk) {

    if (lastTouchedIndex != touchedIndex && lastTouchedIndex > -1 ){
      updateBars(lastTouchedIndex,BAR_COLOR);
      lastTouchedIndex=-1; 
      
    }
    if (touchEventIndex > -1) {
      updateBars(touchEventIndex,SELECTED_BAR_COLOR);
      lastTouchedIndex = touchEventIndex ; 
      touchEventIndex = -1 ;   
    }

    
//    for (uint8_t i = 0; i < ARRAY_SIZE; i++) {
//        if (dataArray[i] != lastArray[i]) {
//            //updateBars(i, dataArray[i]);
//            updateBars(i,BAR_COLOR);
//            lastArray[i] = dataArray[i];
//        }            
//    }  
}

void loop() {
    for (uint8_t i =0 ; i<AMOUNT_OF_REQUESTS_PROCESSED ; i++){
      checkAndHandleEvents();
//      drawArray(8); // fixme : chunk not implemented.
    }
      drawArray(8); // fixme : chunk not implemented.

    //delay(100); // for testing
    delay(20); // remove before use

    // Periodically update the GUI
}

void initDisplay(void) {
    // Initialize display size and flags
    uint16_t displayWidth = BlueDisplay1.getMaxDisplayWidth();
    uint16_t displayHeight = BlueDisplay1.getMaxDisplayHeight();    
    //BlueDisplay1.setFlagsAndSize(BD_FLAG_FIRST_RESET_ALL | BD_FLAG_USE_MAX_SIZE | BD_FLAG_TOUCH_BASIC_DISABLE, DISPLAY_WIDTH,
//    DISPLAY_HEIGHT);
#ifdef DO_NOT_NEED_BASIC_TOUCH_EVENTS 
    BlueDisplay1.setFlagsAndSize(BD_FLAG_FIRST_RESET_ALL | BD_FLAG_USE_MAX_SIZE | BD_FLAG_TOUCH_BASIC_DISABLE, displayWidth,displayHeight);
#else //#ifdef DO_NOT_NEED_BASIC_TOUCH_EVENTS 
    BlueDisplay1.setFlagsAndSize(BD_FLAG_FIRST_RESET_ALL | BD_FLAG_USE_MAX_SIZE , displayWidth,displayHeight);
#endif //#ifdef DO_NOT_NEED_BASIC_TOUCH_EVENTS 

    // Initialize button position, size, colors etc.
//    TouchButtonBlinkStartStop.init((DISPLAY_WIDTH - BUTTON_WIDTH_2) / 2, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_2,
//    BUTTON_HEIGHT_4, COLOR16_BLUE, "Start", 44, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN, doBlink,
//            &doBlinkStartStop);
//    TouchButtonBlinkStartStop.setCaptionForValueTrue("Stop");

//    BlueDisplay1.debug(StartMessage);
}

void drawGui() {
    // Get the maximum display size
    struct XYSize* maxSize = BlueDisplay1.getMaxDisplaySize();
    uint16_t maxDisplayWidth = maxSize->XWidth;
    uint16_t maxDisplayHeight = maxSize->YHeight;

    // Set up plot dimensions based on percentages
    plotX = (maxDisplayWidth * plotXPercent) / 100;
    plotY = (maxDisplayHeight * plotYPercent) / 100;
    plotWidth = (maxDisplayWidth * plotWidthPercent) / 100;
    plotHeight = (maxDisplayHeight * plotHeightPercent) / 100;

    // Calculate the width of each bar
    barWidth = plotWidth / (ARRAY_SIZE)+1  ;

    // Calculate sensitivity 

    sensitivity = ((float)ARRAY_CLAMP_VALUE/maxDisplayHeight)*(float)SENSITIVITY_MULTIPLIER; 
    
    // Clear the display with background color
    BlueDisplay1.clearDisplay(BACKGROUND_COLOR);
    drawBars();
}

void drawBars() {
    for (uint8_t i = 0; i < ARRAY_SIZE; i++) {
      updateBars(i,BAR_COLOR);
    }
}

//void updateBars(uint8_t index, uint8_t newValue) {
void updateBars(uint8_t index,color16_t bar_color) {
    uint16_t xPos = plotX + index * barWidth;
    //BlueDisplay1.fillRect(xPos, plotY, barWidth, plotHeight, BACKGROUND_COLOR);  // Clear the old bar
    //BlueDisplay1.fillRect(xPos, plotY + plotHeight, xPos+barWidth, plotY, BACKGROUND_COLOR);

    //uint16_t barHeight = map(newValue, 0, MAX_BAR_HEIGHT, 0, plotHeight);
    uint16_t barHeight = map(dataArray[index], 0, ARRAY_CLAMP_VALUE, 0, plotHeight);

    //BlueDisplay1.fillRect(xPos, plotY + plotHeight - barHeight, barWidth, barHeight, BAR_COLOR);
    BlueDisplay1.fillRect(xPos, plotY + plotHeight-barHeight, xPos+barWidth-1, plotY, BACKGROUND_COLOR);
    BlueDisplay1.fillRect(xPos, plotY + plotHeight, xPos+barWidth-1, plotY+plotHeight-barHeight, bar_color);

    sprintf(sStringBuffer,"%d", dataArray[index]);
    BlueDisplay1.drawText((xPos+(barWidth/2))-LABEL_FONT_SIZE*1, plotY+plotHeight, sStringBuffer,LABEL_FONT_SIZE, bar_color, BACKGROUND_COLOR);
    //sprintf(sStringBuffer,"%d", index);
    //BlueDisplay1.drawText((xPos+(barWidth/2))-LABEL_FONT_SIZE*1, plotY+plotHeight, sStringBuffer,LABEL_FONT_SIZE, bar_color, BACKGROUND_COLOR);
}

void handleTouchMove(struct TouchEvent *const aCurrentPositionPtr) {
    //if (aCurrentPositionPtr->TouchPosition.PositionX >= plotX && aCurrentPositionPtr->TouchPosition.PositionX <= plotX + plotWidth && aCurrentPositionPtr->TouchPosition.PositionY >= plotY && aCurrentPositionPtr->TouchPosition.PositionY <= plotY + plotHeight) {
//        touchedIndex = (aCurrentPositionPtr->TouchPosition.PositionX - plotX) / barWidth; // do not set touched index here to avoid changing adjacent values during moving finger up and down
        
        if (touchedIndex < ARRAY_SIZE) {
            //uint8_t newBarValue = map(plotHeight + plotY - aCurrentPositionPtr->TouchPosition.PositionY, 0, plotHeight, 0, MAX_BAR_HEIGHT);
            touchedDelta =+ (sLastPos.PositionY-aCurrentPositionPtr->TouchPosition.PositionY) 
             *(sensitivity* abs(sLastPos.PositionY-aCurrentPositionPtr->TouchPosition.PositionY) );
             
            //int delta_Y = dataArray[touchedIndex] + (sLastPos.PositionY-aCurrentPositionPtr->TouchPosition.PositionY) 
            // *(sensitivity* abs(sLastPos.PositionY-aCurrentPositionPtr->TouchPosition.PositionY));    
            int delta_Y = dataArray[touchedIndex] + touchedDelta;  
            
            dataArray[touchedIndex] = constrain(delta_Y, 0, ARRAY_CLAMP_VALUE  );      
        }
    //}
    sLastPos.PositionX = aCurrentPositionPtr->TouchPosition.PositionX;
    sLastPos.PositionY = aCurrentPositionPtr->TouchPosition.PositionY;
    touchEventIndex = touchedIndex; 
}
void handleTouchDown(struct TouchEvent *const aCurrentPositionPtr) {
    //sprintf(sStringBuffer,"%d", aCurrentPositionPtr->TouchPosition.PositionX-plotX);
    //BlueDisplay1.drawText(128, 128, sStringBuffer,LABEL_FONT_SIZE, BAR_COLOR, BACKGROUND_COLOR);
    //sprintf(sStringBuffer,"%d", plotWidth);
    //BlueDisplay1.drawText(128, 128+LABEL_FONT_SIZE, sStringBuffer,LABEL_FONT_SIZE, BAR_COLOR, BACKGROUND_COLOR);
    
    if (aCurrentPositionPtr->TouchPosition.PositionX >= plotX && aCurrentPositionPtr->TouchPosition.PositionX <= plotX + plotWidth  && aCurrentPositionPtr->TouchPosition.PositionY >= plotY && aCurrentPositionPtr->TouchPosition.PositionY <= plotY + plotHeight) {
     // BlueDisplay1.drawLine(aCurrentPositionPtr->TouchPosition.PositionX,0,aCurrentPositionPtr->TouchPosition.PositionX,plotHeight+plotY,BAR_COLOR);

        if ((aCurrentPositionPtr->TouchPosition.PositionX - plotX) / barWidth < ARRAY_SIZE) { // double check against barWidth rounding errors.
         touchedIndex = (aCurrentPositionPtr->TouchPosition.PositionX - plotX) / barWidth;  // set touchedIndex on touch down to avoid changing adjacent values when moving finger up and down
         touchEventIndex = touchedIndex;
         touchedDelta = 0 ; // reset the delta 
        }
    }
    int x = aCurrentPositionPtr->TouchPosition.PositionX;
    int y = aCurrentPositionPtr->TouchPosition.PositionY;
//    BlueDisplay1.drawPixel(x, y, BACKGROUND_COLOR);
    sLastPos.PositionX = x;
    sLastPos.PositionY = y;
}
