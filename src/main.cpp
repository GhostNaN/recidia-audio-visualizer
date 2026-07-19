#include <unistd.h>
#include <string>
#include <chrono>
#include <thread>
#include <vector>

#include <recidia.h>
#include <qt_window.hpp>

using namespace std;

// Set globals
struct recidia_settings_struct recidia_settings;
struct recidia_data_struct recidia_data;

u_int64_t utime_now() {
    auto high_res_time = chrono::high_resolution_clock::now();
    auto duration = high_res_time.time_since_epoch();
    auto time = std::chrono::duration_cast< std::chrono::microseconds>(duration);
    return time.count();
}

void get_audio_device(recidia_audio_data *audio_data, int GUI) {
    uint i;

    vector<string> deviceNames;

    // Get pipewire devices to choose from
    struct pipe_device_info *pipeHead;
    pipeHead = get_pipe_devices_info();

    struct pipe_device_info *tempPipeHead;
    tempPipeHead = pipeHead;

    vector<uint> pipeIndexes;
    i = 0;
    while (tempPipeHead != NULL) {
        deviceNames.push_back(tempPipeHead->name);
        pipeIndexes.push_back(i);

        tempPipeHead = tempPipeHead->next;
        i++;
    }

    // Get device index
    uint deviceIndex = 0;
    if (!GUI) {
        i = 0;  

        printf("|||Pipewire Nodes|||\n");
        for(i=0; i < pipeIndexes.size(); i++) {
            printf("[%i] %s\n", i, deviceNames[i].c_str());
        }
        printf("\n");

        // Get input or enter bad/nothing for default
        printf("\nEnter audio device index:\n");
        char devBuffer[4];
        char *fgetout = fgets(devBuffer, 4, stdin);
        if (fgetout == devBuffer)
            deviceIndex = atoi(devBuffer); // If fail = 0 aka default pulse
    }
    else {
        deviceIndex = display_audio_devices(deviceNames, pipeIndexes);
    }

    if (deviceIndex >= pipeIndexes.size()) {
        fprintf(stderr, "Error: Bad device index\n");
        exit(EXIT_FAILURE);
    }

    // Find the device selected
    i = 0;
    struct pipe_device_info *pipeDevice = NULL;

    while (pipeHead != NULL) {
        struct pipe_device_info *next = pipeHead->next; // Save the next pointer first

        if (i == deviceIndex) {
            pipeDevice = pipeHead;
        } else {
            free(pipeHead);
        }

        pipeHead = next;
        i++;
    }

    // Begin collecting audio data
    audio_data->pipe_device = pipeDevice;
    // audio_data->sample_rate = audio_data->pipe_device->rate;
    audio_data->sample_rate = 48000; // placeholder for now
    pipe_collect_audio_data(audio_data);

}

int main(int argc, char **argv) {
    // GUI if any arg, else it's terminal
    int GUI = argc-1;

    // Get settings
    recidia_settings = {};
    init_recidia_settings(GUI);
    get_config_settings(GUI);

    // Init Audio Collection
    recidia_audio_data audioData;
    audioData.frame_index = 0;
    audioData.buffer_size = &recidia_settings.data.audio_buffer_size;
    audioData.samples = (short*) calloc(recidia_settings.data.AUDIO_BUFFER_SIZE.MAX, sizeof(short));
    get_audio_device(&audioData, GUI);

    // Apply limits to settings that needed audioData info
    float maxFreq = (float) audioData.sample_rate / 2;
    limit_setting(recidia_settings.data.chart_guide.start_freq, 0.0, maxFreq);
    limit_setting(recidia_settings.data.chart_guide.mid_freq, 0.0, maxFreq);
    limit_setting(recidia_settings.data.chart_guide.end_freq, 0.0, maxFreq);

    // Data shared between threads
    recidia_data = {};
    recidia_data.width = 10;
    recidia_data.height = 10;
    recidia_data.start_time = 0;
    recidia_data.frame_time = 0;
    recidia_data.plots_count = (recidia_data.width / (recidia_settings.design.plot_width + recidia_settings.design.gap_width));
    recidia_data.plots = (float*) calloc(recidia_settings.data.AUDIO_BUFFER_SIZE.MAX / 2, sizeof(float));

    // Init processing
    thread proThread(init_processing, &audioData);

    if (GUI) {
        init_gui(argc, argv);
    }
    else {
        init_curses();
    }

    return 0;
}
