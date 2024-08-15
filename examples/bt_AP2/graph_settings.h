#define MINUTES_GRAPH_BUFFER_MAX 60*24 //1440 *4 = 5760 bytes

float minutes_buffer[MINUTES_GRAPH_BUFFER_MAX];
float minutes_buffer_min = 0 ; 
float minutes_buffer_max = 0 ; 
#define MINUTES_INTERVAL 1000*60 //each minute
//#define MINUTES_INTERVAL 1000  // each second
//#define MINUTES_INTERVAL 1000*10  // each 10

#define DEBUG_INTERVAL 5000 //each second
#define LASTVALUE_INTERVAL 1000*10 //each 10 seconds


uint32_t minutes_millis_last = MINUTES_INTERVAL;
uint32_t debug_millis_last = DEBUG_INTERVAL;
uint32_t lastvalue_millis_last = LASTVALUE_INTERVAL;

//#define GRAPH_WIDTH  DISPLAY_WIDTH   // Graph width in pixels (adjust as needed)
//#define GRAPH_HEIGHT DISPLAY_HEIGHT-128    // Graph height in pixels (adjust as needed)

#define LEGEND_LABEL_FONT_SIZE 16 // define font size of labels
#define LEGEND_LABEL_CHARS 4 // define how many characters per label

#define GRAPH_X  0      // initial position X
#define GRAPH_Y 128-64     // initial position Y
const int minutes_dataArraySize = sizeof(minutes_buffer) / sizeof(minutes_buffer[0]);

#define GRAPH_TEST // uncomment to fill with random values

#define MAX_LINES  MINUTES_GRAPH_BUFFER_MAX+10        // Define maximum buffer size for line storage
#define DRAWN_MAGIC_NUMBER 0xFFFF  // Magic number to mark the line as drawn

// Structure to store line parameters
typedef struct {
    uint16_t x1, y1, x2, y2;
    color16_t color;
} LineBuffer;

LineBuffer lineBuffer[MAX_LINES];
uint16_t lineBufferIndex = 0;
// Global variable to track the current position in the line buffer
uint16_t currentLineIndex = 0;

bool graphComplete = false; // indicates if graph buffer is completely drawn

// Global variable to store the graph height
uint16_t globalGraphHeight = 0;
// Global variable to store the graph Y position
uint16_t globalGraphYPos = 0;
