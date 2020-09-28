#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
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
        deviceNames.push_back(" Default PulseAudio Output");
        deviceNames.push_back(" Default PulseAudio Input");
    }

    i = 0;
    while (deviceNamesPtr[i]) {
        deviceNames.push_back(deviceNamesPtr[i]);
        i++;
    }
    free(deviceNamesPtr);

    vector<string> pulseMonitors;
    i = 0;
    if (pulseMonitorsPtr) {
        while (pulseMonitorsPtr[i]) {
            pulseMonitors.push_back(pulseMonitorsPtr[i]);
            i++;
        }
    }
    free(pulseMonitorsPtr);

    vector<int> portIndexes;
    i = 0;
    while (portIndexesPtr[i]) {
        portIndexes.push_back(portIndexesPtr[i]);
        i++;
    }
    free(portIndexesPtr);

    // Print Devices
    for (i=0; i < deviceNames.size(); i++) {

        if (i < pulseDefaults) {    // Pulse Defaults
            cout << to_string(i) + deviceNames[i] + "\n";
        }
        else if (i < pulseMonitors.size() + pulseDefaults) {    // Outputs
            cout << to_string(i) + " Output: " + deviceNames[i] + "\n";
        }
        else {  // Inputs
            cout << to_string(i) + " Input: " + deviceNames[i] + "\n";
        }
    }

    // Get input
    uint deviceIndex = 0;
    printf("Enter device index:\n");
    cin >> deviceIndex;

    int portDeviceIndex = 0;
    if (deviceIndex < (uint) pulseMonitors.size() + pulseDefaults ) { // Pulseaudio
        string pulseDevice;

        if (deviceIndex < pulseDefaults) {
            pulseDevice = pulseDefaultsPtr[deviceIndex];
        }
        else {
            pulseDevice = pulseMonitors[deviceIndex - pulseDefaults];
        }

        string homeDir = getenv("HOME");
        ifstream fileRead(homeDir + "/.asoundrc");

        // Check/Clean lines for an old "pcm.revidia_capture" device
        if (fileRead.is_open()) {
            string cleanAsoundrc;
            string line;
            int skip = 0;
            while (getline(fileRead, line)) {
                if (!skip) {
                    if (line.find("pcm.recidia_capture") != string::npos) {
                        skip = 1;
                    }
                    else {
                        cleanAsoundrc += line + "\n";
                    }
                }
                else {
                    if (line.find("}") != string::npos) {
                        skip = 0;
                    }
                }
            }
            fileRead.close();

            ofstream fileWrite(homeDir + "/.asoundrc");
            fileWrite << cleanAsoundrc;
        }

        // Create ALSA device to connect to PulseAudio
        ofstream fileAppend(homeDir + "/.asoundrc", ios::out | ios::app);
        if (fileAppend.is_open()) {

            string recidiaAlsaDevice =
                    "pcm.recidia_capture {\n"
                    "\ttype pulse\n"
                    "\tdevice " + pulseDevice + "\n" +
                    "}";
            fileAppend << recidiaAlsaDevice;
        }
        fileAppend.close();


        // Get portaudio device index
        Pa_Initialize();

        uint paDeviceNum = Pa_GetDeviceCount();

        for(i=0; i < paDeviceNum; i++) {
            const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);
            string paDeviceName = deviceInfo->name;
            if (paDeviceName.find("recidia_capture") != string::npos) {
                portDeviceIndex = i;
                break;
            }
        }

        Pa_Terminate();
    }
    else { // Portaudio
        int index = deviceIndex - (pulseMonitors.size() + pulseDefaults);
        portDeviceIndex = portIndexes[index];
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
