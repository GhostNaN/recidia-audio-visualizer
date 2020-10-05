#ifndef RECIDIA_H 
#define RECIDIA_H

const int PULSE_INPUT = 0;
const int  PULSE_MONITOR = 1;
struct pulse_device_info {
    char *name;
    char *source_name;
    int rate;
    int mode;
    int _default;
    struct pulse_device_info *next;
};

struct port_device_info {
    char *name;
    int index;
    int rate;
    struct port_device_info *next;
};

typedef struct recidia_audio_data {
    unsigned int frame_index;
    unsigned int *buffer_size;
    unsigned int sample_rate;
    struct pulse_device_info *pulse_device;
    struct port_device_info *port_device;
    short *samples;
} recidia_audio_data;

// C code
#ifdef __cplusplus
extern "C" {
    struct pulse_device_info *get_pulse_devices_info();
    
    struct port_device_info *get_port_devices_info();
    
    void pulse_collect_audio_data(recidia_audio_data *audio_data);

    void port_collect_audio_data(recidia_audio_data *audio_data);
}
#endif

typedef struct recidia_setings {
    float MAX_PLOT_HEIGHT_CAP, plot_height_cap;
    unsigned int MAX_PLOT_WIDTH, plot_width;
    unsigned int MAX_GAP_WIDTH, gap_width;
    unsigned int savgol_filter[3];
    unsigned int MAX_INTEROPLATION, interp;               // Huge memory allocation
    unsigned int MAX_AUDIO_BUFFER_SIZE, audio_buffer_size;  // Huge memory allocation
    unsigned int MAX_FPS, fps;
    float chart_guide[6];
    int stats;
    
} recidia_setings;

typedef struct recidia_data {    
    unsigned int width, height;
    double time;
    float frame_time;
    float *plots;
    
} recidia_data;

typedef struct recidia_sync {   
    int status; 
    int inbound;
    int outbound;
    
} recidia_sync;

void get_settings(recidia_setings *settings);

void init_curses(recidia_setings *settings, recidia_data *data, recidia_sync *sync);

void init_processing(recidia_setings *settings, recidia_data *plot_data, recidia_audio_data *audio_data, recidia_sync *sync);

#endif
