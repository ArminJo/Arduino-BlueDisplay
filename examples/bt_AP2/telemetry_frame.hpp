#pragma pack(push,1)
//typedef struct telemetry_frame {
struct telemetry_frame {

 float voltage_ADC0 = 12.1;
};
//} tframe;
#pragma pack(pop)

bool new_packet = false ; // new packet flag
uint32_t total_packets = 0 ; // packets count
