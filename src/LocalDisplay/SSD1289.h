/*
 * SSD1289.h
 *
 * @date  14.02.2012
 * @author  Armin Joachimsmeyer
 * armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 */

#ifndef SSD1289_h
#define SSD1289_h

/** @addtogroup Graphic_Library
 * @{
 */
/** @addtogroup HY32D_basic
 * @{
 */

// Landscape format
#define LOCAL_DISPLAY_HEIGHT    240
#define LOCAL_DISPLAY_WIDTH     320
// Portrait format
//#define LOCAL_DISPLAY_HEIGHT    320
//#define LOCAL_DISPLAY_WIDTH     240

extern uint8_t sCurrentBacklightPercent;
extern uint8_t sLastBacklightPercentBeforeDimming; //! for state of backlight before dimming
extern int sLCDDimDelay; //actual dim delay

/*
 * Backlight values in percent
 */
#define BACKLIGHT_START_BRIGHTNESS_VALUE     50
#define BACKLIGHT_MAX_BRIGHTNESS_VALUE      100
#define BACKLIGHT_DIM_VALUE                   7
#define BACKLIGHT_DIM_DEFAULT_DELAY_MILLIS 120000 // Two minutes

#ifdef __cplusplus
class SSD1289 { // @suppress("Class has a virtual method and non-virtual destructor")

public:
    SSD1289();
    ~SSD1289();
    void init(void);
    void setBacklightBrightness(uint8_t aBrightnessPercent);  // 0-100

    void clearDisplay(uint16_t color);
    void drawPixel(uint16_t aXPos, uint16_t aYPos, color16_t aColor);
    void drawCircle(uint16_t aXCenter, uint16_t aYCenter, uint16_t aRadius, color16_t aColor);
    void fillCircle(uint16_t aXCenter, uint16_t aYCenter, uint16_t aRadius, color16_t aColor);
    void fillRectRel(uint16_t aXStart, uint16_t aYStart, uint16_t aWidth, uint16_t aHeight, color16_t aColor);
    void drawTextVertical(uint16_t aXPos, uint16_t aYPos, const char *aStringPointer, uint8_t aSize, color16_t aColor,
            uint16_t aBackgroundColor);
    void drawLine(uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, color16_t aColor);
    void drawLineRel(uint16_t aStartX, uint16_t aStartY, int16_t aDeltaX, int16_t aDeltaY, color16_t aColor);
    void drawLineFastOneX(uint16_t x0, uint16_t y0, uint16_t y1, color16_t color);
    void drawRect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, color16_t color);

    /*
     * Basic hardware functions required for the draw functions below
     */
    void fillRect(uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY, color16_t aColor);
    void setArea(uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY);
    void drawStart();
    void draw(color16_t aColor);
    void drawStop();

    void setCursor(uint16_t aXStart, uint16_t aYStart);
    uint16_t getDisplayWidth();
    uint16_t getDisplayHeight();

    uint16_t readPixel(uint16_t aXPos, uint16_t aYPos);
    uint16_t* fillDisplayLineBuffer(uint16_t *aBufferPtr, uint16_t yLineNumber);

private:

};

#endif // __cplusplus

extern bool isInitializedSSD1289;
extern volatile uint32_t sDrawLock;

void setDimdelay(int32_t aTimeMillis);
void resetBacklightTimeout(void);
void callbackLCDDimming(void);
int clipBrightnessValue(int aBrightnessValue);

uint16_t getDisplayWidth(void);
uint16_t getDisplayHeight(void);

int drawNText(uint16_t x, uint16_t y, const char *s, int aNumberOfCharacters, uint8_t size, uint16_t color, uint16_t bg_color);

uint16_t drawInteger(uint16_t x, uint16_t y, int val, uint8_t base, uint8_t size, uint16_t color, uint16_t bg_color);

extern "C" void storeScreenshot(void);
/*
 * fast divide by 11 for SSD1289 driver arguments
 */
uint16_t getFontScaleFactorFromTextSize(uint16_t aTextSize);

// Tests
void initalizeDisplay2(void);
void setGamma(int aIndex);
void writeCommand(int aRegisterAddress, int aRegisterValue);

/** @} */
/** @} */

#endif //SSD1289_h
