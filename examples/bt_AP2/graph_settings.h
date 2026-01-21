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

// Define the number of iterations for the LFSR
//#define LFSR_MAX_ITERATIONS 2048  // We will work with 2047 iterations
//#define LFSR_POLYNOMIAL 0x0600 ;  // Use the 11bit polynomial
        //  (0x0600 corresponds to the polynomial  x^11 + x^9 + 1)

//#define LFSR_MAX_ITERATIONS 2048  // We will work with 2047 iterations
//#define LFSR_POLYNOMIAL 0x0E04 ;  // Use the 11bit polynomial
        //  (0x0600 corresponds to the polynomial  x^11 + x^10 + x^9 + x^3 + 1)

#define LFSR_MAX_ITERATIONS 4096  // We will work with 4095 iterations
#define LFSR_POLYNOMIAL 0xD008 ;  // Use the 12bit polynomial
        //  (0xD008 corresponds to the polynomial x^12 + x^11 + x^10 + x^4 + 1)

//#define LFSR_MAX_ITERATIONS 65535  // 16bit polynomial with 65535 iterations
//#define LFSR_POLYNOMIAL 0xB400 ;  // 16bit polynomial (0xB400 corresponds to the polynomial x^16 + x^14 + x^13 + x^11 + 1)

// Variables to store the current state of the LFSR and maximum lines
uint16_t lfsr = 1;  // Initial seed value for the LFSR
//int maxLines = MAX_LINES;  // Set this to the maximum number of lines in the buffer
int linesDrawn = 0;  // Track how many lines have been drawn



// Structure to store line parameters
typedef struct {
    uint16_t x1, y1, x2, y2;
    uint16_t color;
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
