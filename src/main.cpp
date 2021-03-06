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
    uint i, j;

    vector<string> deviceNames;

    // Get pulseaudio devices to choose from
    struct pulse_device_info *pulseHead;
    pulseHead = get_pulse_devices_info();

    struct pulse_device_info *tempPulseHead;
    tempPulseHead = pulseHead;

    // Sort output/input
    int pulseDefaultIndex = 0;
    vector<uint> pulseIndexes;
    i=0; j=0;
    while (tempPulseHead != NULL) {

        if (tempPulseHead->mode == PULSE_MONITOR) {
            deviceNames.insert(deviceNames.begin() + i, "Output: " + (string) tempPulseHead->name);
            pulseIndexes.insert(pulseIndexes.begin() + i, j);
            // Index default device
            if (tempPulseHead->_default)
                pulseDefaultIndex = i;

            i++;
        }
        else {
            deviceNames.push_back("Input: " + (string) tempPulseHead->name);
            pulseIndexes.push_back(j);
        }

        tempPulseHead = tempPulseHead->next;
        j++;
    }

    // Get portaudio devices to choose from
    vector<uint> portIndexes;

    struct port_device_info *portHead;
    portHead = get_port_devices_info();

    struct port_device_info *tempPortHead;
    tempPortHead = portHead;

    i = 0;
    while (tempPortHead != NULL) {
        deviceNames.push_back("Input: " + (string) tempPortHead->name);
        portIndexes.push_back(i);

        tempPortHead = tempPortHead->next;
        i++;
    }

    // Get device index
    uint deviceIndex = 0;
    int d = 0;
    if (pulseIndexes.size()) {
        d = 1; // d as in default
    }
    if (!GUI) {
        i = 0;  
        if (pulseHead) {
            printf("|||PulseAudio Devices|||\n");
            printf("[0] Default Output\n");
            for(i=0; i < pulseIndexes.size(); i++) {
                printf("[%i] %s\n", i+d, deviceNames[i].c_str());
            }
            printf("\n");
        }

        if (portHead) {
            printf("|||PortAudio Devices|||\n");
            for(j=0; j < portIndexes.size(); j++) {
                printf("[%i] %s\n", j+i+d, deviceNames[j+i].c_str());
            }
        }

        // Get input or enter bad/nothing for default
        printf("\nEnter device index:\n");
        char devBuffer[4];
        fgets(devBuffer, 4, stdin);

        deviceIndex = atoi(devBuffer); // If fail = 0 aka default pulse
    }
    else {
        deviceIndex = display_audio_devices(deviceNames, pulseIndexes, portIndexes);
    }

    if (deviceIndex == 0)
        deviceIndex = pulseDefaultIndex;
    else {
        // Subtract to account for default device being 0
        deviceIndex -= d;
    }

    if (deviceIndex >= pulseIndexes.size() + portIndexes.size()) {
        fprintf(stderr, "Error: Bad device index\n");
        exit(EXIT_FAILURE);
    }


    // Find the device selected
    i = 0;

    struct pulse_device_info *pulseDevice = NULL;
    while (pulseHead != NULL) {

        if (i == pulseIndexes[deviceIndex])
            pulseDevice = pulseHead;
        else
            free(pulseHead);

        pulseHead = pulseHead->next;
        i++;
    }

    struct port_device_info *portDevice = NULL;
    while (portHead != NULL) {

        if (i == deviceIndex)
            portDevice = portHead;
        else
            free(portHead);

        portHead = portHead->next;
        i++;
    }

    // Begin collecting audio data
    if (pulseDevice) {
        audio_data->pulse_device = pulseDevice;
        audio_data->sample_rate = audio_data->pulse_device->rate;
        pulse_collect_audio_data(audio_data);
    }
    else {
        audio_data->port_device = portDevice;
        audio_data->sample_rate = audio_data->port_device->rate;
        port_collect_audio_data(audio_data);
    }
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
