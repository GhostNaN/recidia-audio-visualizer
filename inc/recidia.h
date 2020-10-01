#ifndef RECIDIA_H 
#define RECIDIA_H

#include <fftw3.h>
#include <gsl/gsl_blas.h>

typedef struct pa_data {
    unsigned int frame_index;
    unsigned int *buffer_size;
    unsigned int sample_rate;
    short *samples;
} pa_data;

// C code
#ifdef __cplusplus
extern "C" {
    void get_audio_devices(char ***device_names, char ***pulse_monitors, char ***pulse_defaults, int **pa_indexes);

    void collect_audio_data(struct pa_data *audio_data, int device_index);
    
    int pulse_monitor_to_port_index(const char *pulse_monitor);
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

void init_processing(recidia_setings *settings, recidia_data *plot_data, pa_data *audio_data, recidia_sync *sync);

#endif
