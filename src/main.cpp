#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cmath>
#include <vector>
#include <ncurses.h>
#include <locale.h>
#include <unistd.h>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <thread>

#include <portaudio.h>
#include <fftw3.h>

#include <recidia.h>

using namespace std;

int main() {
    uint i;

    char **deviceNamesPtr;
    char **pulseMonitorsPtr;
    char **pulseDefaultsPtr;
    int *portIndexesPtr;

    get_audio_devices(&deviceNamesPtr, &pulseMonitorsPtr, &pulseDefaultsPtr, &portIndexesPtr);

    // C Pointers to C++ Vectors
    uint pulseDefaults = 0;
    vector<string> deviceNames;
    if (pulseDefaultsPtr) {
        pulseDefaults = 2;
        deviceNames.push_back("Default PulseAudio Output");
        deviceNames.push_back("Default PulseAudio Input");
    }

    i = 0;
    while (deviceNamesPtr[i]) {
        deviceNames.push_back(deviceNamesPtr[i]);
        i++;
    }
    free(deviceNamesPtr);

    // Print Devices
    for (i=0; i < deviceNames.size(); i++) {
        printf("%i %s\n", i, deviceNames[i].c_str());
    }

    // Get input
    uint deviceIndex = 0;
    printf("Enter device index:\n");
    scanf("%i", &deviceIndex);


    int portDeviceIndex = 0;
    if (deviceIndex < (uint) (sizeof(pulseMonitorsPtr) / sizeof(*pulseMonitorsPtr[0])) + pulseDefaults ) { // Pulseaudio
        char *pulseDevice;

        if (deviceIndex < pulseDefaults) {
            pulseDevice = pulseDefaultsPtr[deviceIndex];
        }
        else {
            pulseDevice = pulseMonitorsPtr[deviceIndex - pulseDefaults];
        }

        portDeviceIndex = pulse_monitor_to_port_index(pulseDevice);
    }
    else { // Portaudio
        int index = deviceIndex - ((sizeof(pulseMonitorsPtr) / sizeof(*pulseMonitorsPtr[0])) + pulseDefaults);
        portDeviceIndex = portIndexesPtr[index];
    }

    // Get settings
    recidia_setings settings;
    get_settings(&settings);

    // Init Audio Collection
    pa_data *audioData;
    audioData = (pa_data*) malloc(sizeof(struct pa_data));
    audioData->frame_index = 0;
    audioData->buffer_size = &settings.audio_buffer_size;
    audioData->samples = (short*) calloc(settings.MAX_AUDIO_BUFFER_SIZE, sizeof(short));
    collect_audio_data(audioData, portDeviceIndex);

    // Data shared between threads
    recidia_data data;
    data.width = 10;
    data.height = 10;
    data.time = 0;
    data.frame_time = 0;
    data.plots = (float*) calloc(settings.MAX_AUDIO_BUFFER_SIZE/2, sizeof(float));

    // Init processing
    recidia_sync proSync;
    proSync.status = 0;
    proSync.inbound = 0;
    proSync.outbound = 0;
    thread proThread(init_processing, &settings, &data, audioData, &proSync);

    // Init curses
    recidia_sync cursesSync;
    cursesSync.status = 0;
    cursesSync.inbound = 0;
    cursesSync.outbound = 0;
    thread cursesThread(init_curses, &settings, &data, &cursesSync);

    // Main Loop
    while(1) {
        auto timerStart = chrono::high_resolution_clock::now();

        // Fake multithreading for now for lower latency, but worse performance.
        proSync.status = 0;
        while (!proSync.status) { usleep(1000); }
        cursesSync.status = 0;
        while (!cursesSync.status) { usleep(1000); }

        // Global timer
        chrono::duration<double> timerEnd = (chrono::high_resolution_clock::now() - timerStart) * 1000;
        double latency = timerEnd.count();

        int sleepTime = ((1000 / (double) settings.fps) - latency) * 1000;
        if (sleepTime > 0)
            usleep(sleepTime);

        timerEnd = (chrono::high_resolution_clock::now() - timerStart) * 1000;
        data.frame_time = timerEnd.count();
    }
    return 0;
}
