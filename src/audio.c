#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#ifdef PIPEWIRE
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#endif

#ifdef PULSEAUDIO
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#endif

#ifdef PORTAUDIO
#include <portaudio.h>
#endif

#include <recidia.h>

#ifdef PIPEWIRE
struct pw_data {
    struct pw_stream *stream;
    recidia_audio_data *audio_data;
};

struct pipe_device_info *pipe_head = NULL;

static void registry_event_global(void *data, uint32_t id, uint32_t permissions, const char *type, uint32_t version, const struct spa_dict *props) {

    if (props == NULL)      
        return;
    // We only want nodes
    if (strcmp(type, PW_TYPE_INTERFACE_Node) != 0)
        return;

    // And only audio nodes
    const char *pw_media_class = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
    if (pw_media_class) {
        if (strstr(pw_media_class, "Audio") == NULL)
            return;
    }
    else {
        return;
    }

    const char *pw_node_name = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);
    if (pw_node_name == NULL)
        pw_node_name = spa_dict_lookup(props, PW_KEY_NODE_NICK);
    if (pw_node_name == NULL)
        pw_node_name = spa_dict_lookup(props, PW_KEY_NODE_NAME);
    if (pw_node_name == NULL)
        return;

    struct pipe_device_info *device;
    device = malloc(sizeof(struct pulse_device_info));

    device->id = id;

    device->name = calloc((strlen(pw_node_name) + 1) , sizeof(char));
    strcpy(device->name, pw_node_name);

    device->next = pipe_head;
    pipe_head = device;
}
 
static const struct pw_registry_events registry_events = {
    .version = PW_VERSION_REGISTRY_EVENTS,
    .global = registry_event_global,
};

static void core_event_done( void *data, uint32_t id, int seq) {
    struct pw_main_loop *loop = data;
    
    if (id == PW_ID_CORE) {
        pw_main_loop_quit(loop);
    }
}


static const struct pw_core_events core_events = {
    .version = PW_VERSION_CORE_EVENTS,
    .done = core_event_done,
};

struct pipe_device_info *get_pipe_devices_info() {
    struct pw_main_loop *loop;
    struct pw_context *context;
    struct pw_core *core;
    struct spa_hook core_listener;
    struct pw_registry *registry;
    struct spa_hook registry_listener;

    pw_init(NULL, NULL);

    loop = pw_main_loop_new(NULL /* properties */);
    context = pw_context_new(pw_main_loop_get_loop(loop),
                    NULL /* properties */,
                    0 /* user_data size */);

    core = pw_context_connect(context,
                    NULL /* properties */,
                    0 /* user_data size */);

    spa_zero(core_listener);
    pw_core_add_listener(core, &core_listener, &core_events, loop);

    registry = pw_core_get_registry(core, PW_VERSION_REGISTRY,
                    0 /* user_data size */);

    spa_zero(registry_listener);
    pw_registry_add_listener(registry, &registry_listener,
                                    &registry_events, NULL);

    pw_core_sync(core, PW_ID_CORE, 0);
    pw_main_loop_run(loop);

    pw_proxy_destroy((struct pw_proxy*)registry);
    pw_core_disconnect(core);
    pw_context_destroy(context);
    pw_main_loop_destroy(loop);
    pw_deinit();

    return pipe_head;
}

static void on_process(void *userdata) {
    struct pw_data *data = userdata;
    struct pw_buffer *pw_buffer;

    if ((pw_buffer = pw_stream_dequeue_buffer(data->stream)) == NULL) {
        pw_log_warn("out of buffers: %m");
        return;
    }
    short int *samples = pw_buffer->buffer->datas[0].data;
    if (samples == NULL)
        return;
    
    int sample_size = SPA_MIN(pw_buffer->buffer->datas[0].chunk->size, pw_buffer->buffer->datas[0].maxsize);
    sample_size = sample_size / sizeof(short int); // Sample size is in bytes so correct to short int

    // Copy data for processing
    int sample_offset = 0;
    int memcpy_size = 0;
    while(sample_size > 0) {

        int buffer_free = *data->audio_data->buffer_size - data->audio_data->frame_index;

        if (buffer_free >= sample_size)
            memcpy_size = sample_size;
        else 
            memcpy_size = buffer_free;
        
        memcpy(data->audio_data->samples+data->audio_data->frame_index, samples+sample_offset, memcpy_size * sizeof(samples[0]));
        sample_size -= memcpy_size;
        sample_offset += memcpy_size;

        data->audio_data->frame_index += memcpy_size;
        if (data->audio_data->frame_index >= *data->audio_data->buffer_size)
            data->audio_data->frame_index = 0;
    }
    

    pw_stream_queue_buffer(data->stream, pw_buffer);
}

static const struct pw_stream_events stream_events = {
        .version = PW_VERSION_STREAM_EVENTS,
        .process = on_process,
};

static void pipe_quit(void *userdata, int signal_number) {
        struct pw_main_loop *loop = userdata;
        pw_main_loop_quit(loop);
}


static void *init_pipe_audio_collection(void* data) {
    recidia_audio_data *audio_data = data;
    struct pw_data pw_data;
    pw_data.audio_data = audio_data;

    pw_init(NULL, NULL);

    struct pw_main_loop *loop = pw_main_loop_new(NULL);

    pw_loop_add_signal(pw_main_loop_get_loop(loop), SIGINT, pipe_quit, loop);
    pw_loop_add_signal(pw_main_loop_get_loop(loop), SIGTERM, pipe_quit, loop);

    pw_data.stream = pw_stream_new_simple(
            pw_main_loop_get_loop(loop),
            "recidia capture",
            pw_properties_new(
                    PW_KEY_MEDIA_TYPE, "Audio",
                    PW_KEY_MEDIA_CATEGORY, "Capture",
                    NULL),
            &stream_events,
            &pw_data);

    short int buffer[4096];
    struct spa_pod_builder pod_builder = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

    const struct spa_pod *params[1];
    params[0] = spa_format_audio_raw_build(&pod_builder, SPA_PARAM_EnumFormat,
            &SPA_AUDIO_INFO_RAW_INIT(
                    .format = SPA_AUDIO_FORMAT_S16,
                    .channels = 1,
                    .rate = audio_data->sample_rate));

    pw_stream_connect(pw_data.stream,
            PW_DIRECTION_INPUT,
            audio_data->pipe_device->id,
            PW_STREAM_FLAG_AUTOCONNECT |
            PW_STREAM_FLAG_MAP_BUFFERS,
            params, 1);

    pw_main_loop_run(loop);

    pw_stream_destroy(pw_data.stream);
    pw_main_loop_destroy(loop);
    pw_deinit();

    pthread_exit(NULL);
}

void pipe_collect_audio_data(recidia_audio_data *audio_data) {

    pthread_t thread;
    pthread_create(&thread, NULL, &init_pipe_audio_collection, audio_data);
}
#else
struct pulse_device_info *get_pipe_devices_info() {return NULL;}
void pipe_collect_audio_data(recidia_audio_data *audio_data) {return;}
#endif


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

static void *init_pulse_audio_collection(void* data) {
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

    pthread_exit(NULL);
}

void pulse_collect_audio_data(recidia_audio_data *audio_data) {

    pthread_t thread;
    pthread_create(&thread, NULL, &init_pulse_audio_collection, audio_data);
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
