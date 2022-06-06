/*
 * BlueDisplayProtocol.h
 *
 * Defines all the protocol related constants and structures required for the client stubs.
 *
 *  SUMMARY
 *  Blue Display is an Open Source Android remote Display for Arduino etc.
 *  It receives basic draw requests from Arduino etc. over Bluetooth and renders it.
 *  It also implements basic GUI elements as buttons and sliders.
 *  GUI callback, touch and sensor events are sent back to Arduino.
 *
 *  Copyright (C) 2015  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of BlueDisplay https://github.com/ArminJo/android-blue-display.
 *
 *  BlueDisplay is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 *
 *
 * SEND PROTOCOL USED:
 * Message:
 * 1. Sync byte A5
 * 2. byte function token
 * 3. Short length (in bytes units -> always multiple of 2) of parameters
 * 4. Short n parameters
 *
 * Data (expected for messages with function code >= 0x60):
 * 1. Sync byte A5
 * 2. byte Data_Size_Type token (byte, short etc.) - only byte used now
 * 3. Short length of data in byte units
 * 4. (Length) items of data values
 *
 *
 * RECEIVE PROTOCOL USED:
 *
 * Touch/size message has 8 bytes:
 * 1 - Gross message length in bytes
 * 2 - Function code
 * 3 - X Position LSB
 * 4 - X Position MSB
 * 5 - Y Position LSB
 * 6 - Y Position MSB
 * 7 - Pointer index
 * 8 - Sync token
 *
 * Callback message has 15 bytes:
 * 1 - Gross message length in bytes
 * 2 - Function code
 * 16 bit button index
 * 16 bit filler for 32 bit alignment of next values
 * 32 bit callback address
 * 32 bit value
 * 13 - Sync token
 *
 */

#ifndef _BLUEDISPLAYPROTOCOL_H
#define _BLUEDISPLAYPROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

#define SYNC_TOKEN 0xA5

/**********************
 * Event codes
 *********************/
// eventType can be one of the following:
//see also android.view.MotionEvent
#define EVENT_TOUCH_ACTION_DOWN 0x00
#define EVENT_TOUCH_ACTION_UP   0x01
#define EVENT_TOUCH_ACTION_MOVE 0x02
#define EVENT_TOUCH_ACTION_ERROR 0xFF

// connection event sent after (re)connecting from host
#define EVENT_CONNECTION_BUILD_UP 0x10
// redraw event if canvas size was changed manually on host
#define EVENT_REDRAW 0x11
// reorientation event sent if orientation changed or requestMaxCanvasSize() was called
#define EVENT_REORIENTATION 0x12
// disconnect event sent if manually disconnected (does not cover out of range etc.)
#define EVENT_DISCONNECT 0x14

// command sizes
#define TOUCH_COMMAND_MAX_DATA_SIZE 15
#define RECEIVE_MAX_DATA_SIZE (TOUCH_COMMAND_MAX_DATA_SIZE - 3) // 15 - command, length and sync token
// events with a lower number have RECEIVE_TOUCH_OR_DISPLAY_DATA_SIZE
// events with a greater number have RECEIVE_CALLBACK_DATA_SIZE
#define EVENT_FIRST_CALLBACK_ACTION_CODE 0x20

// GUI elements (button, slider, get number) callback codes
#define EVENT_BUTTON_CALLBACK  0x20
#define EVENT_SLIDER_CALLBACK  0x21
#define EVENT_SWIPE_CALLBACK  0x22
#define EVENT_LONG_TOUCH_DOWN_CALLBACK  0x23

#define EVENT_NUMBER_CALLBACK 0x28
#define EVENT_INFO_CALLBACK  0x29

#define EVENT_TEXT_CALLBACK  0x2C

// NOP used for synchronizing
#define EVENT_NOP 0x2F

// Sensor callback codes
// Tag number is 0x30 + sensor type constant from android.hardware.Sensor
#define EVENT_FIRST_SENSOR_ACTION_CODE 0x30
#define EVENT_LAST_SENSOR_ACTION_CODE 0x3F

#define EVENT_REQUESTED_DATA_CANVAS_SIZE 0x60

#define EVENT_NO_EVENT 0xFF

/*********************
 * Event structures
 *********************/
struct XYSize {
    uint16_t XWidth;
    uint16_t YHeight;
};

struct XYPosition {
    uint16_t PosX;
    uint16_t PosY;
};

struct TouchEvent {
    struct XYPosition TouchPosition;
    uint8_t TouchPointerIndex;
};

struct DisplaySizeAndUnixTimestamp {
    struct XYSize DisplaySize;
    uint32_t UnixTimestamp;
};

struct Swipe {
    bool SwipeMainDirectionIsX; // true if TouchDeltaXAbs >= TouchDeltaYAbs
    uint8_t Filler;
    uint16_t Free;
    uint16_t TouchStartX;
    uint16_t TouchStartY;
    int16_t TouchDeltaX;
    int16_t TouchDeltaY;
    uint16_t TouchDeltaAbsMax; // max of TouchDeltaXAbs and TouchDeltaYAbs to easily decide if swipe is large enough to be accepted
};

// Union to speed up the combination of low and high bytes to a word
// it is not optimal since the compiler still generates 2 unnecessary moves
// but using  -- value = (high << 8) | low -- gives 5 unnecessary instructions
union ByteShortLongFloatUnion {
    unsigned char byteValues[4];
    uint16_t uint16Values[2];
    uint32_t uint32Value;
    float floatValue;
};

struct GuiCallback {
    uint16_t ObjectIndex;
    uint16_t Free;
#if defined(AVR)
    void * Handler;
    void * Handler_upperWord; // not used on  <= 17 bit address cpu, since pointer to functions are address_of_function >> 1
#else
    void * Handler;
#endif
    union ByteShortLongFloatUnion ValueForGuiHandler;
};

/*
 * Values received from accelerator sensor range from -10 to 10
 */
struct SensorCallback {
    float ValueX;
    float ValueY;
    float ValueZ;
};

struct IntegerInfoCallback {
    uint8_t SubFunction;
    uint8_t ByteInfo;
    uint16_t ShortInfo;
#if defined(AVR)
    void * Handler;
    void * Handler_upperWord; // not used on  <= 17 bit address cpu, since pointer to functions are address_of_function >> 1
#else
    void * Handler;
#endif
    union ByteShortLongFloatUnion LongInfo;
};

struct BluetoothEvent {
    uint8_t EventType;
    union EventData {
        unsigned char ByteArray[RECEIVE_MAX_DATA_SIZE]; // To copy data from input buffer
        struct TouchEvent TouchEventInfo; // for EVENT_TOUCH_ACTION_*
        struct XYSize DisplaySize;
        uint32_t UnixTimestamp;
        struct DisplaySizeAndUnixTimestamp DisplaySizeAndTimestamp;
        struct GuiCallback GuiCallbackInfo; // EVENT_*_CALLBACK
        struct Swipe SwipeInfo;
        struct SensorCallback SensorCallbackInfo;
        struct IntegerInfoCallback IntegerInfoCallbackData;
    } EventData;
};

/**********************
 * Data field types
 *********************/
const int DATAFIELD_TAG_BYTE = 0x01;
const int DATAFIELD_TAG_SHORT = 0x02;
//const int DATAFIELD_TAG_INT = 0x03;
//const int DATAFIELD_TAG_LONG = 0x04;
//const int DATAFIELD_TAG_FLOAT = 0x05;
//const int DATAFIELD_TAG_DOUBLE = 0x06;
const int LAST_FUNCTION_TAG_DATAFIELD = 0x07;

/**********************
 * Internal functions
 *********************/
static const int INDEX_FIRST_FUNCTION_WITH_DATA = 0x60;

static const int FUNCTION_GLOBAL_SETTINGS = 0x08;
// Sub functions for GLOBAL_SETTINGS
static const int SUBFUNCTION_GLOBAL_SET_FLAGS_AND_SIZE = 0x00;
static const int SUBFUNCTION_GLOBAL_SET_CODEPAGE = 0x01;
static const int SUBFUNCTION_GLOBAL_SET_CHARACTER_CODE_MAPPING = 0x02;
static const int SUBFUNCTION_GLOBAL_SET_LONG_TOUCH_DOWN_TIMEOUT = 0x08;
static const int SUBFUNCTION_GLOBAL_SET_SCREEN_ORIENTATION_LOCK = 0x0C;

// results in a reorientation (+redraw) callback
static const int FUNCTION_REQUEST_MAX_CANVAS_SIZE = 0x09;

/**********************
 * Sensors
 *********************/
static const int FUNCTION_SENSOR_SETTINGS = 0x0A;

/**********************
 * Miscellaneous functions
 *********************/
static const int FUNCTION_GET_NUMBER = 0x0C;
static const int FUNCTION_GET_TEXT = 0x0D;
static const int FUNCTION_GET_INFO = 0x0E;
// Sub functions for FUNCTION_GET_INFO
static const int SUBFUNCTION_GET_INFO_LOCAL_TIME = 0x00;
static const int SUBFUNCTION_GET_INFO_UTC_TIME = 0x01;

static const int FUNCTION_PLAY_TONE = 0x0F;

// Function with variable data size
// used for Sync
const int FUNCTION_NOP = 0x7F;

/**********************
 * Display functions
 *********************/
const int FUNCTION_CLEAR_DISPLAY = 0x10;
const int FUNCTION_DRAW_DISPLAY = 0x11;
const int FUNCTION_CLEAR_DISPLAY_OPTIONAL = 0x12;
// 3 parameter
const int FUNCTION_DRAW_PIXEL = 0x14;
// 6 parameter
const int FUNCTION_DRAW_CHAR = 0x16;
// 5 parameter
const int FUNCTION_DRAW_LINE_REL = 0x20;
const int FUNCTION_DRAW_LINE = 0x21;
const int FUNCTION_DRAW_RECT_REL = 0x24;
const int FUNCTION_FILL_RECT_REL = 0x25;
const int FUNCTION_DRAW_RECT = 0x26;
const int FUNCTION_FILL_RECT = 0x27;

const int FUNCTION_DRAW_CIRCLE = 0x28;
const int FUNCTION_FILL_CIRCLE = 0x29;

const int FUNCTION_DRAW_VECTOR_DEGREE = 0x2C;
const int FUNCTION_DRAW_VECTOR_RADIAN = 0x2D;

const int FUNCTION_WRITE_SETTINGS = 0x34;
// Flags for WRITE_SETTINGS
const int FLAG_WRITE_SETTINGS_SET_SIZE_AND_COLORS_AND_FLAGS = 0x00;
const int FLAG_WRITE_SETTINGS_SET_POSITION = 0x01;
const int FLAG_WRITE_SETTINGS_SET_LINE_COLUMN = 0x02;

const int INDEX_LAST_FUNCTION_WITHOUT_DATA = 0x5F;
// Function with variable data size
const int FUNCTION_DRAW_STRING = 0x60;
const int FUNCTION_DEBUG_STRING = 0x61;
const int FUNCTION_WRITE_STRING = 0x62;

const int FUNCTION_GET_NUMBER_WITH_SHORT_PROMPT = 0x64;
const int FUNCTION_GET_TEXT_WITH_SHORT_PROMPT = 0x65;

const int FUNCTION_DRAW_PATH = 0x68;
const int FUNCTION_FILL_PATH = 0x69;
const int FUNCTION_DRAW_CHART = 0x6A;
const int FUNCTION_DRAW_CHART_WITHOUT_DIRECT_RENDERING = 0x6B;

/**********************
 * Button functions
 *********************/
const int FUNCTION_BUTTON_DRAW = 0x40;
const int FUNCTION_BUTTON_DRAW_CAPTION = 0x41;
const int FUNCTION_BUTTON_SETTINGS = 0x42;
// Flags for BUTTON_SETTINGS
const int SUBFUNCTION_BUTTON_SET_BUTTON_COLOR = 0x00;
const int SUBFUNCTION_BUTTON_SET_BUTTON_COLOR_AND_DRAW = 0x01;
const int SUBFUNCTION_BUTTON_SET_CAPTION_COLOR = 0x02;
const int SUBFUNCTION_BUTTON_SET_CAPTION_COLOR_AND_DRAW = 0x03;
const int SUBFUNCTION_BUTTON_SET_VALUE = 0x04;
const int SUBFUNCTION_BUTTON_SET_VALUE_AND_DRAW = 0x05;
const int SUBFUNCTION_BUTTON_SET_COLOR_AND_VALUE = 0x06;
const int SUBFUNCTION_BUTTON_SET_COLOR_AND_VALUE_AND_DRAW = 0x07;
const int SUBFUNCTION_BUTTON_SET_POSITION = 0x08;
const int SUBFUNCTION_BUTTON_SET_POSITION_AND_DRAW = 0x09;
const int SUBFUNCTION_BUTTON_SET_ACTIVE = 0x10;
const int SUBFUNCTION_BUTTON_RESET_ACTIVE = 0x11;
const int SUBFUNCTION_BUTTON_SET_AUTOREPEAT_TIMING = 0x12;

const int FUNCTION_BUTTON_REMOVE = 0x43;

// static functions
const int FUNCTION_BUTTON_ACTIVATE_ALL = 0x48;
const int FUNCTION_BUTTON_DEACTIVATE_ALL = 0x49;
const int FUNCTION_BUTTON_GLOBAL_SETTINGS = 0x4A;

// Function with variable data size
const int FUNCTION_BUTTON_CREATE = 0x70;
const int FUNCTION_BUTTON_INIT = 0x70;
const int FUNCTION_BUTTON_SET_CAPTION_FOR_VALUE_TRUE = 0x71;
const int FUNCTION_BUTTON_SET_CAPTION = 0x72;
const int FUNCTION_BUTTON_SET_CAPTION_AND_DRAW_BUTTON = 0x73;

/**********************
 * Slider functions
 *********************/
static const int FUNCTION_SLIDER_CREATE = 0x50;
static const int FUNCTION_SLIDER_INIT = 0x50;
static const int FUNCTION_SLIDER_DRAW = 0x51;
static const int FUNCTION_SLIDER_SETTINGS = 0x52;
static const int FUNCTION_SLIDER_DRAW_BORDER = 0x53;

// Flags for SLIDER_SETTINGS
static const int SUBFUNCTION_SLIDER_SET_COLOR_THRESHOLD = 0x00;
static const int SUBFUNCTION_SLIDER_SET_COLOR_BAR_BACKGROUND = 0x01;
static const int SUBFUNCTION_SLIDER_SET_COLOR_BAR = 0x02;
static const int SUBFUNCTION_SLIDER_SET_VALUE_AND_DRAW_BAR = 0x03;
static const int SUBFUNCTION_SLIDER_SET_POSITION = 0x04;
static const int SUBFUNCTION_SLIDER_SET_ACTIVE = 0x05;
static const int SUBFUNCTION_SLIDER_RESET_ACTIVE = 0x06;
static const int SUBFUNCTION_SLIDER_SET_SCALE_FACTOR = 0x07;

static const int SUBFUNCTION_SLIDER_SET_CAPTION_PROPERTIES = 0x08;
static const int SUBFUNCTION_SLIDER_SET_VALUE_STRING_PROPERTIES = 0x09;

static const int SUBFUNCTION_SLIDER_SET_VALUE = 0x0C;

// static slider functions
static const int FUNCTION_SLIDER_ACTIVATE_ALL = 0x58;
static const int FUNCTION_SLIDER_DEACTIVATE_ALL = 0x59;
static const int FUNCTION_SLIDER_GLOBAL_SETTINGS = 0x5A;

// Flags for SLIDER_BLOBAL_SETTINGS
static const int SUBFUNCTION_SLIDER_SET_DEFAULT_COLOR_THRESHOLD = 0x01;

// Function with variable data size
const int FUNCTION_SLIDER_SET_CAPTION = 0x78;
const int FUNCTION_SLIDER_PRINT_VALUE = 0x79;
const int FUNCTION_SLIDER_SET_VALUE_UNIT_STRING = 0x7A;
const int FUNCTION_SLIDER_SET_VALUE_FORMAT_STRING = 0x7B;

#endif // _BLUEDISPLAYPROTOCOL_H
