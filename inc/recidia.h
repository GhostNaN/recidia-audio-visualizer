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

const int FPS_CAP_DECREASE = 13;
const int FPS_CAP_INCREASE = 14;

const int STATS_TOGGLE = 15;

const int DRAW_MODE_TOGGLE = 16;


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
        float window_size; 
        uint poly_order; 
    } savgol_filter;
    
    struct chart_guide {
        float start_freq;
        float start_ctrl;
        float mid_freq;
        float mid_pos;
        float end_ctrl;
        float end_freq;
    } chart_guide;
    
    float poll_rate;
    recidia_const_setting<float> POLL_RATE;
};

struct rgba_color {
    uint red;
    uint green;
    uint blue;
    uint alpha;
};

struct recidia_design_settings {
    float draw_x;
    float draw_y;
    float draw_width;
    float draw_height;
    float min_plot_height;
    unsigned int plot_width;
    recidia_const_setting<unsigned int> PLOT_WIDTH;
    unsigned int gap_width;
    recidia_const_setting<unsigned int> GAP_WIDTH;
    int draw_mode;
    unsigned int fps_cap;
    recidia_const_setting<unsigned int> FPS_CAP;
    
    rgba_color main_color;
    rgba_color back_color;
};

struct recidia_misc_settings {
    int stats;
};

// Global settings/data because it's used EVERYWHERE, passing is stupid
struct recidia_settings_struct {
    struct recidia_data_settings data;
    struct recidia_design_settings design;
    struct recidia_misc_settings misc;
};
extern struct recidia_settings_struct recidia_settings;

struct recidia_data_struct {    
    unsigned int width, height;
    double time;
    float frame_time;
    float *plots;
};
extern struct recidia_data_struct recidia_data;


void change_setting_by_key(char key);

void get_settings(int GUI);

void init_curses();

void init_processing(recidia_audio_data *audio_data);

u_int64_t utime_now();
#endif

#endif
