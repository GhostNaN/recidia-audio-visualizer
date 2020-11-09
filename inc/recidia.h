#ifndef RECIDIA_H 
#define RECIDIA_H

// Settings changes with keyboard
const int PLOT_HEIGHT_CAP_DECREASE = 1;
const int PLOT_HEIGHT_CAP_INCREASE = 2;

const int PLOT_WIDTH_DECREASE = 3;
const int PLOT_WIDTH_INCREASE = 4;

const int GAP_WIDTH_DECREASE = 5;
const int GAP_WIDTH_INCREASE = 6;

const int SAVGOL_WINDOW_SIZE_DECREASE = 7;
const int SAVGOL_WINDOW_SIZE_INCREASE = 8;

const int INTERPOLATION_DECREASE = 9;
const int INTERPOLATION_INCREASE = 10;

const int AUDIO_BUFFER_SIZE_DECREASE = 11;
const int AUDIO_BUFFER_SIZE_INCREASE = 12;

const int FPS_DECREASE = 13;
const int FPS_INCREASE = 14;

const int STATS_TOGGLE = 15;


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

template <typename T> 
struct recidia_const_setting { 
    T MIN;
    T MAX;
    T DEFAULT;
    char KEY_DECREASE;
    char KEY_INCREASE;
};

struct recidia_data_settings {
    float height_cap;
    recidia_const_setting<float> HEIGHT_CAP;
    unsigned int interp;   
    recidia_const_setting<unsigned int> INTERP;             
    unsigned int audio_buffer_size;  
    recidia_const_setting<unsigned int> AUDIO_BUFFER_SIZE; 
    
    struct savgol_filter { 
        unsigned int window_size; 
        unsigned int poly_order; 
    } savgol_filter;
    
    struct chart_guide {
        float start_freq;
        float start_ctrl;
        float mid_freq;
        float mid_pos;
        float end_ctrl;
        float end_freq;
    } chart_guide;
};

struct float_rgba_color {
    float red;
    float green;
    float blue;
    float alpha;
};

struct recidia_design_settings {
    recidia_const_setting<float> HEIGHT;
    unsigned int plot_width;
    recidia_const_setting<unsigned int> PLOT_WIDTH;
    unsigned int gap_width;
    recidia_const_setting<unsigned int> GAP_WIDTH;
    
    float_rgba_color main_color;
    float_rgba_color back_color;
};

struct recidia_misc_settings {
    unsigned int fps;
    recidia_const_setting<unsigned int> FPS;
    
    int stats;
};

typedef struct recidia_settings {
    struct recidia_data_settings data;
    struct recidia_design_settings design;
    struct recidia_misc_settings misc;
    
} recidia_settings;

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

void change_setting_by_key(recidia_settings *settings, char key);

void get_settings(recidia_settings *settings, int GUI);

void init_curses(recidia_settings *settings, recidia_data *data, recidia_sync *sync);

void init_processing(recidia_settings *settings, recidia_data *plot_data, recidia_audio_data *audio_data, recidia_sync *sync);
#endif

#endif
