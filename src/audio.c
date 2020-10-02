#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef PULSE
#include <pulse/pulseaudio.h>
#endif
#include <portaudio.h>

#include <recidia.h>

struct alloc_info {
    int device_name;
    int pulse_monitor;
    int port_index;
};
struct alloc_info alloc_size;

#ifdef PULSE
static pa_context *context = NULL;
static pa_mainloop_api *mainloop_api = NULL;
static pa_proplist *proplist = NULL;

struct pulse_monitors_info {
    char *name;
    char *device_name;
    struct pulse_monitors_info *next;

};
struct pulse_monitors_info *pulse_head = NULL;

char *default_monitor;
char *default_input;

static void get_source_info_callback(pa_context *c, const pa_source_info *i, int is_last, void *userdata) {
    (void) c;
    (void) userdata;

    if (is_last || is_last < 0) {
        mainloop_api->quit(mainloop_api, 1);
        return;
    }
    if (!i->monitor_of_sink_name) {
        return;
    }


    struct pulse_monitors_info *monitor;
    monitor = malloc(sizeof(struct pulse_monitors_info));

    const char *name = i->name;
    monitor->name = malloc((strlen(name) + 1) * sizeof(char*));
    strcpy(monitor->name, name);

    char *device_name = (char*) pa_proplist_gets(i->proplist, "alsa.card_name");
    if (!device_name) {
        device_name = (char*) i->monitor_of_sink_name;
    }
    monitor->device_name = malloc((strlen("Output: ") + strlen(device_name) + 1) * sizeof(char*));
    strcpy(monitor->device_name, "Output: ");
    strcat(monitor->device_name, device_name);

    monitor->next = pulse_head;
    pulse_head = monitor;

    alloc_size.device_name += (strlen(monitor->device_name) + 1) * sizeof(char*);
    alloc_size.pulse_monitor += (strlen(monitor->name) + 1) * sizeof(char*);
}

static void get_server_info_callback(pa_context *c, const pa_server_info *i, void *userdata) {
    (void) c;
    (void) userdata;

    char *sink = (char*) i->default_sink_name;
    default_monitor = malloc((strlen(sink) + 9) * sizeof (char*));
    strcpy(default_monitor, sink);
    strcat(default_monitor, ".monitor");

    default_input = (char*) i->default_source_name;
}

static void context_state_callback(pa_context *c, void *userdata) {
    (void) userdata;

    if (pa_context_get_state(c) == PA_CONTEXT_READY) {
        pa_context_get_server_info(c, get_server_info_callback, NULL);
        pa_context_get_source_info_list(c, get_source_info_callback, NULL);
    }

}

static void get_pulse_info() {
    pa_mainloop *m = NULL;
    int ret = 1;
    char *server = NULL;

    proplist = pa_proplist_new();
    m = pa_mainloop_new();

    mainloop_api = pa_mainloop_get_api(m);

    context = pa_context_new_with_proplist(mainloop_api, NULL, proplist);
    pa_context_set_state_callback(context, context_state_callback, NULL);
    pa_context_connect(context, server, 0, NULL);

    pa_mainloop_run(m, &ret);

    pa_context_unref(context);
    pa_mainloop_free(m);
    pa_xfree(server);
    pa_proplist_free(proplist);
}
#endif


struct port_device_info {
    char *name;
    int index;
    struct port_device_info *next;
};
struct port_device_info *port_head = NULL;

static void get_port_info() {
    Pa_Initialize();

    int pa_device_num = Pa_GetDeviceCount();

    int p = 0;
    for(int i=0; i < pa_device_num; i++) {
        const PaDeviceInfo* device_info = Pa_GetDeviceInfo(i);

        if (device_info->maxInputChannels > 0) {
            struct port_device_info *device;
            device = malloc(sizeof(struct port_device_info));

            char *device_name = (char*) device_info->name;
            device->name = malloc((strlen("Input: ") + strlen(device_name) + 1) * sizeof(char*));
            strcpy(device->name, "Input: ");
            strcat(device->name, device_name);

            device->index = i;

            device->next = port_head;
            port_head = device;

            alloc_size.device_name += (strlen(device->name) + 1) * sizeof(char*);
            alloc_size.port_index += sizeof(int*);
            p++;
        }
    }

    Pa_Terminate();
}

void get_audio_devices(char ***device_names, char ***pulse_monitors, char ***pulse_defaults, int **port_indexes) {

#ifdef PULSE
    get_pulse_info();
#endif
    get_port_info();

    *device_names = malloc(alloc_size.device_name + sizeof(NULL));
    int i = 0;

#ifdef PULSE
    *pulse_defaults = malloc((strlen(default_monitor) + strlen(default_input) + 1) * sizeof(char*));
    (*pulse_defaults)[0] = default_monitor;
    (*pulse_defaults)[1] = default_input;

    *pulse_monitors = malloc(alloc_size.pulse_monitor + sizeof(NULL));

    struct pulse_monitors_info *pulse_ptr;

    while (pulse_head != NULL) {
        pulse_ptr = pulse_head;

        (*device_names)[i] = pulse_head->device_name;
        (*pulse_monitors)[i] = pulse_head->name;
        pulse_head = pulse_head->next;

        free(pulse_ptr);
        i++;
    }
    (*pulse_monitors)[i] = NULL; // End marker
#endif

    *port_indexes = malloc(alloc_size.port_index + sizeof(int*));

    struct port_device_info *port_ptr;

    int p = 0;
    while (port_head != NULL) {
        port_ptr = port_head;

        (*device_names)[i] = port_head->name;
        (*port_indexes)[p] = port_head->index;
        port_head = port_head->next;

        free(port_ptr);
        i++;
        p++;
    }
    (*port_indexes)[p] = -1; // End marker

    (*device_names)[i] = NULL; // End marker

    system("clear");
}

static int recordCallback( const void *inputBuffer, void *outputBuffer,
        unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags, void *userData ) {

    (void) outputBuffer;
    (void) framesPerBuffer;
    (void) timeInfo;
    (void) statusFlags;

    struct pa_data *data = userData;

    short *input = (short*) inputBuffer;
    data->samples[data->frame_index] = (input[0] + input[1]) / 2; // Avg. of left [0] and right [1]

    if (data->frame_index < *data->buffer_size) {
        data->frame_index++;
    }
    else {
        data->frame_index = 0;
    }

    return 0;
}

void collect_audio_data(struct pa_data *audio_data, int device_index) {

    Pa_Initialize();

    const PaDeviceInfo* device = Pa_GetDeviceInfo(device_index);

    PaStreamParameters input_parameters;
    input_parameters.device = device_index;
    input_parameters.channelCount = 2;
    input_parameters.sampleFormat = paInt16;
    input_parameters.suggestedLatency = device->defaultLowInputLatency;
    input_parameters.hostApiSpecificStreamInfo = NULL;

    audio_data->sample_rate = device->defaultSampleRate;

    PaStream *stream;
    Pa_OpenStream(
        &stream,
        &input_parameters,
        NULL,                  /* &outputParameters, */
        audio_data->sample_rate,
        1,
        paClipOff,      /* we won't output out of range samples so don't bother clipping them */
        recordCallback,
        audio_data );

    Pa_StartStream(stream);

}

// Check/Clean lines for an old "pcm.revidia_capture" device
void clean_asound() {

    char *home_dir = getenv("HOME");
    char *asound_file_path = malloc((strlen(home_dir) + strlen("/.asoundrc") + 1) * sizeof(char*));
    strcpy(asound_file_path, home_dir);
    strcat(asound_file_path, "/.asoundrc");

    FILE *asound_file;
    FILE *temp;

    asound_file = fopen(asound_file_path, "a+");

    temp = fopen("asoundtemp", "w+");

    char line[128];

    int skip = 0;
    while (fgets(line, sizeof(line), asound_file)) {
        if (!skip) {
            if (strcmp(line, "pcm.recidia_capture {\n") == 0) {
                skip = 1;
            }
            else {
                fprintf(temp, "%s", line);
            }
        }
        else {
            if (strcmp(line, "}\n") == 0) {
                skip = 0;
            }
        }
    }
    fclose(temp);
    fclose(asound_file);
    // Overwrite .asoundrc
    rename("asoundtemp", asound_file_path);

    free(asound_file_path);
}

int pulse_monitor_to_port_index(const char *pulse_monitor) {

    clean_asound();

    char *home_dir = getenv("HOME");
    char *asound_file_path = malloc((strlen(home_dir) + strlen("/.asoundrc") + 1) * sizeof(char*));
    strcpy(asound_file_path, home_dir);
    strcat(asound_file_path, "/.asoundrc");

    FILE *asound_file;

    // Create ALSA device to connect to PulseAudio
    asound_file = fopen(asound_file_path, "a");

    char *recidiaAlsaDevice = malloc((strlen(pulse_monitor) + 50) * sizeof(char*));
    strcpy(recidiaAlsaDevice,   "pcm.recidia_capture {\n"
                                "\ttype pulse\n"
                                "\tdevice ");
    strcat(recidiaAlsaDevice, pulse_monitor);
    strcat(recidiaAlsaDevice, "\n}");

    fputs(recidiaAlsaDevice, asound_file);

    fclose(asound_file);

    // Get portaudio device index
    Pa_Initialize();
    uint portDeviceIndex = 0;

    uint paDeviceNum = Pa_GetDeviceCount();
    for(uint i=0; i < paDeviceNum; i++) {
        const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);
        char *paDeviceName = (char*) deviceInfo->name;
        if (strcmp(paDeviceName, "recidia_capture") == 0) {
            portDeviceIndex = i;
            break;
        }
    }

    Pa_Terminate();

    free(asound_file_path);
    free(recidiaAlsaDevice);

    return portDeviceIndex;
}
