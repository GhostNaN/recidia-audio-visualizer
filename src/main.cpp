#include <unistd.h>
#include <string>
#include <chrono>
#include <thread>
#include <vector>

#include <recidia.h>
#include <qt_window.h>

using namespace std;

static void init_main_loop(recidia_settings *settings, recidia_data *data, recidia_sync *proSync, recidia_sync *cursesSync) {
    while(1) {
        auto timerStart = chrono::high_resolution_clock::now();

        proSync->status = 0;
        while (!proSync->status) { usleep(1000); }
        if (cursesSync) {
            cursesSync->status = 0;
            while (!cursesSync->status) { usleep(1000); }
        }

        // Global timer
        chrono::duration<double> timerEnd = (chrono::high_resolution_clock::now() - timerStart) * 1000;
        double latency = timerEnd.count();

        int sleepTime = ((1000 / (double) settings->misc.fps) - latency) * 1000;
        if (sleepTime > 0)
            usleep(sleepTime);

        timerEnd = (chrono::high_resolution_clock::now() - timerStart) * 1000;
        data->frame_time = timerEnd.count();
    }
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
        system("clear");

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
    (void) argv;
    // GUI if any arg, else it's terminal
    int GUI = argc-1;

    // Get settings
    recidia_settings settings;
    get_settings(&settings, GUI);    

    // Init Audio Collection
    recidia_audio_data audioData;
    audioData.frame_index = 0;
    audioData.buffer_size = &settings.data.audio_buffer_size;
    audioData.samples = (short*) calloc(settings.data.AUDIO_BUFFER_SIZE.MAX, sizeof(short));

    get_audio_device(&audioData, GUI);

    // Data shared between threads
    recidia_data data;
    data.width = 10;
    data.height = 10;
    data.time = 0;
    data.frame_time = 0;
    data.plots = (float*) calloc(settings.data.AUDIO_BUFFER_SIZE.MAX / 2, sizeof(float));

    // Init processing
    recidia_sync proSync;
    proSync.status = 0;
    proSync.inbound = 0;
    proSync.outbound = 0;
    thread proThread(init_processing, &settings, &data, &audioData, &proSync);

    // Init curses
    recidia_sync cursesSync;
    cursesSync.status = 0;
    cursesSync.inbound = 0;
    cursesSync.outbound = 0;

    if (GUI) {
        thread loopThread(init_main_loop, &settings, &data, &proSync, nullptr);
        init_gui(argc, argv, &data, &settings);
    }
    else {
        thread loopThread(init_main_loop, &settings, &data, &proSync, &cursesSync);
        init_curses(&settings, &data, &cursesSync);
    }

    return 0;
}
