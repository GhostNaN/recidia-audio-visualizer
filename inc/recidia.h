#ifndef RECIDIA_H 
#define RECIDIA_H

// Settings changes with keyboard
enum setting_changes {
    SETTINGS_MENU_TOGGLE = 1, // Start at 1
    
    FRAMELESS_TOGGLE,
    
    PLOT_HEIGHT_CAP_DECREASE,
    PLOT_HEIGHT_CAP_INCREASE,

    PLOT_WIDTH_DECREASE,
    PLOT_WIDTH_INCREASE,

    GAP_WIDTH_DECREASE,
    GAP_WIDTH_INCREASE,

    SAVGOL_WINDOW_SIZE_DECREASE,
    SAVGOL_WINDOW_SIZE_INCREASE,

    INTERPOLATION_DECREASE,
    INTERPOLATION_INCREASE,

    AUDIO_BUFFER_SIZE_DECREASE,
    AUDIO_BUFFER_SIZE_INCREASE,

    POLL_RATE_DECREASE,
    POLL_RATE_INCREASE,

    FPS_CAP_DECREASE,
    FPS_CAP_INCREASE,

    STATS_TOGGLE,

    DRAW_MODE_TOGGLE,
};


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
    
    unsigned int poll_rate;
    recidia_const_setting<unsigned int> POLL_RATE;
    
    bool stats;
};

struct rgba_color {
    unsigned int red;
    unsigned int green;
    unsigned int blue;
    unsigned int alpha;
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
    char **draw_chars;
    unsigned int fps_cap;
    recidia_const_setting<unsigned int> FPS_CAP;
    
    rgba_color main_color;
    rgba_color back_color;
};

struct shader_setting {
    char *vertex;
    char *frag;
    uint loop_time;
    float power;
    float power_mod_range[2];
};

struct recidia_misc_settings {
    bool settings_menu;
    bool frameless;
    shader_setting main_shader;
    shader_setting back_shader;
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
    float latency;
    float frame_time;
    unsigned int plots_count;
    float *plots;
};
extern struct recidia_data_struct recidia_data;

int get_setting_change(char key);
void change_setting_by_key(char key);
void limit_setting(float &setting, float min, float max);
void limit_setting(int &setting, int min, int max);
void limit_setting(unsigned int &setting, unsigned int min, unsigned int max);

void init_recidia_settings(int GUI);
void get_config_settings(int GUI);

void init_curses();

void init_processing(recidia_audio_data *audio_data);

u_int64_t utime_now();
#endif

#endif
