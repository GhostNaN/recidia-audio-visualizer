#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#ifdef PULSEAUDIO
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#endif

#ifdef PORTAUDIO
#include <portaudio.h>
#endif

#include <recidia.h>

#ifdef PULSEAUDIO
static pa_mainloop_api *mainloop_api = NULL;
struct pulse_device_info *pulse_head = NULL;
static char *default_monitor;

static void get_source_info_callback(pa_context *c, const pa_source_info *i, int is_last, void *userdata) {
    (void) c;
    (void) userdata;

    if (is_last || is_last < 0) {
        mainloop_api->quit(mainloop_api, 1);
        return;
    }
    struct pulse_device_info *device;

    device = malloc(sizeof(struct pulse_device_info));

    char *device_name = (char*) pa_proplist_gets(i->proplist, "alsa.card_name");
    if (!device_name) {
        device_name = (char*) i->name;
    }
    device->name = malloc((strlen(device_name) + 1) * sizeof(char*));
    strcpy(device->name, device_name);

    const char *name = i->name;
    device->source_name = malloc((strlen(name) + 1) * sizeof(char*));
    strcpy(device->source_name, name);

    device->rate = i->sample_spec.rate;

    if (i->monitor_of_sink_name) {
        device->mode = PULSE_MONITOR;
    }
    else {
        device->mode = PULSE_INPUT;
    }

    device->next = pulse_head;
    pulse_head = device;
}

static void get_server_info_callback(pa_context *c, const pa_server_info *i, void *userdata) {
    (void) c;
    (void) userdata;

    char *sink = (char*) i->default_sink_name;
    default_monitor = malloc((strlen(sink) + strlen(".monitor") + 1) * sizeof (char*));
    strcpy(default_monitor, sink);
    strcat(default_monitor, ".monitor");
}

static void context_state_callback(pa_context *c, void *userdata) {
    (void) userdata;

    if (pa_context_get_state(c) == PA_CONTEXT_READY) {
        pa_context_get_server_info(c, get_server_info_callback, NULL);
        pa_context_get_source_info_list(c, get_source_info_callback, NULL);
    }

}

struct pulse_device_info *get_pulse_devices_info() {

    pa_mainloop *m = NULL;
    pa_context *context = NULL;
     pa_proplist *proplist = NULL;
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

    // Find default output device
    struct pulse_device_info *temp_pulse_head;
    temp_pulse_head = pulse_head;

    while (temp_pulse_head != NULL) {
        if (strcmp(default_monitor, temp_pulse_head->source_name) == 0)
            temp_pulse_head->_default = 1;
        else
            temp_pulse_head->_default = 0;

        temp_pulse_head = temp_pulse_head->next;
    }

    return pulse_head;
}

static void *init_audio_collection(void* data) {
    recidia_audio_data *audio_data = data;

    pa_sample_spec sample_spec;
    sample_spec.format = PA_SAMPLE_S16LE;
    sample_spec.rate = audio_data->pulse_device->rate;
    sample_spec.channels = 2;

    pa_buffer_attr buffer_attr;
    buffer_attr.maxlength = -1;
    buffer_attr.fragsize = 0;

    int error;
    pa_simple *simple = NULL;

    simple = pa_simple_new( NULL,  // Default server
                            "recidia_capture",
                            PA_STREAM_RECORD,
                            audio_data->pulse_device->source_name,
                            "recidia_capture",
                            &sample_spec,
                            NULL, // No mapped channels
                            &buffer_attr,
                            &error
                            );
    if (!simple) {
        fprintf(stderr, "pa_simple_new() failed: %s\n", pa_strerror(error));
        exit(EXIT_FAILURE);
    }

    short buffer[2];
    short sample;
    while (1) {
        // Read data
        if (pa_simple_read(simple, &buffer, sizeof(buffer), &error) < 0) {
            fprintf(stderr, ": pa_simple_read() failed: %s\n", pa_strerror(error));
            exit(EXIT_FAILURE);
        }

        // Store data for processing
        sample = (buffer[0] + buffer[1]) / 2; // Avg. of left [0] and right [1]
        audio_data->samples[audio_data->frame_index] = sample;

        audio_data->frame_index += 1;
        if (audio_data->frame_index > *audio_data->buffer_size)
            audio_data->frame_index = 0;
    }
}

void pulse_collect_audio_data(recidia_audio_data *audio_data) {

    pthread_t thread;
    pthread_create(&thread, NULL, &init_audio_collection, audio_data);
}
#else
struct pulse_device_info *get_pulse_devices_info() {return NULL;}
void pulse_collect_audio_data(recidia_audio_data *audio_data) {return;}
#endif


#ifdef PORTAUDIO
struct port_device_info *port_head = NULL;

struct port_device_info *get_port_devices_info() {

    // Prevent portaudio debug logs
    fclose(stderr);
    Pa_Initialize();
    freopen("/dev/tty", "w", stderr);

    int pa_device_num = Pa_GetDeviceCount();

    int p = 0;
    for(int i=0; i < pa_device_num; i++) {
        const PaDeviceInfo* device_info = Pa_GetDeviceInfo(i);

        if (device_info->maxInputChannels > 0) {
            struct port_device_info *device;
            device = malloc(sizeof(struct port_device_info));

            char *device_name = (char*) device_info->name;
            device->name = malloc((strlen(device_name) + 1) * sizeof(char*));
            strcpy(device->name, device_name);

            device->index = i;

            device->rate = device_info->defaultSampleRate;

            device->next = port_head;
            port_head = device;

            p++;
        }
    }

    Pa_Terminate();

    return port_head;
}

static int port_record_callback( const void *inputBuffer, void *outputBuffer,
        unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags, void *userData ) {

    (void) outputBuffer;
    (void) framesPerBuffer;
    (void) timeInfo;
    (void) statusFlags;

    recidia_audio_data *audio_data = userData;

    short *input = (short*) inputBuffer;
    audio_data->samples[audio_data->frame_index] = (input[0] + input[1]) / 2; // Avg. of left [0] and right [1]

    if (audio_data->frame_index < *audio_data->buffer_size) {
        audio_data->frame_index++;
    }
    else {
        audio_data->frame_index = 0;
    }

    return 0;
}

void port_collect_audio_data(recidia_audio_data *audio_data) {
    // Prevent portaudio debug logs
    fclose(stderr);
    Pa_Initialize();
    freopen("/dev/tty", "w", stderr);

    const PaDeviceInfo* device = Pa_GetDeviceInfo(audio_data->port_device->index);

    PaStreamParameters input_parameters;
    input_parameters.device = audio_data->port_device->index;
    input_parameters.channelCount = 2;
    input_parameters.sampleFormat = paInt16;
    input_parameters.suggestedLatency = device->defaultLowInputLatency;
    input_parameters.hostApiSpecificStreamInfo = NULL;

    PaStream *stream;
    Pa_OpenStream(
        &stream,
        &input_parameters,
        NULL, // Output disabled
        audio_data->sample_rate,
        1,
        paNoFlag,
        port_record_callback,
        audio_data );

    Pa_StartStream(stream);
}
#else
struct port_device_info *get_port_devices_info() {return NULL;}
void port_collect_audio_data(recidia_audio_data *audio_data) {return;}
#endif
