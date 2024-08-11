#define MINUTES_GRAPH_BUFFER_MAX 60*24 //1440 *4 = 5760 bytes

float minutes_buffer[MINUTES_GRAPH_BUFFER_MAX];
float minutes_buffer_min = 0 ; 
float minutes_buffer_max = 0 ; 
#define MINUTES_INTERVAL 1000*60 //each minute
//#define MINUTES_INTERVAL 1000  // each second
//#define MINUTES_INTERVAL 1000*10  // each 10

uint32_t minutes_millis_last = MINUTES_INTERVAL

; 
#define GRAPH_WIDTH  DISPLAY_WIDTH   // Graph width in pixels (adjust as needed)
#define GRAPH_HEIGHT DISPLAY_HEIGHT-128    // Graph height in pixels (adjust as needed)

#define GRAPH_X  0      // initial position X
#define GRAPH_Y 128     // initial position Y
const int minutes_dataArraySize = sizeof(minutes_buffer) / sizeof(minutes_buffer[0]);

#define GRAPH_TEST // uncomment to fill with random values
