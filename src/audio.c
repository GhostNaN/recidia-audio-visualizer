#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#ifdef PIPEWIRE
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#endif

#include <recidia.h>

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
    device = malloc(sizeof(struct pipe_device_info));

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
                    PW_KEY_NODE_NAME, "recidia",
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

