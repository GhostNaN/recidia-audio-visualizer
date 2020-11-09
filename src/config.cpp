#include <string>
#include <map>

#include <libconfig.h++>

#include <recidia.h>

using namespace std;

map<char, int> setttings_key_map;

// For Switch cases
constexpr uint str2int(const char* str, int h = 0) {
    return !str[h] ? 5381 : (str2int(str, h+1) * 33) ^ str[h];
}

void change_setting_by_key(recidia_settings *settings, char key) {

    switch (setttings_key_map[key]) {
        case PLOT_HEIGHT_CAP_DECREASE:
            if (settings->data.height_cap > 1) {
                settings->data.height_cap /= 1.5;

                if (settings->data.height_cap < 1)
                    settings->data.height_cap = 1;
            }
            break;
        case PLOT_HEIGHT_CAP_INCREASE:
            if (settings->data.height_cap < settings->data.HEIGHT_CAP.MAX) {
                settings->data.height_cap *= 1.5;

                if (settings->data.height_cap > settings->data.HEIGHT_CAP.MAX)
                    settings->data.height_cap = settings->data.HEIGHT_CAP.MAX;
            }
            break;

        case PLOT_WIDTH_DECREASE:
            if (settings->design.plot_width > 1)
                settings->design.plot_width -= 1;
            break;
        case PLOT_WIDTH_INCREASE:
            if (settings->design.plot_width < settings->design.PLOT_WIDTH.MAX)
                settings->design.plot_width += 1;
            break;

        case GAP_WIDTH_DECREASE:
            if (settings->design.gap_width > 0)
                settings->design.gap_width -= 1;
            break;
        case GAP_WIDTH_INCREASE:
            if (settings->design.gap_width < settings->design.GAP_WIDTH.MAX)
                settings->design.gap_width += 1;
            break;

        case SAVGOL_WINDOW_SIZE_DECREASE:
            if (settings->data.savgol_filter.window_size > 0)
                settings->data.savgol_filter.window_size -= 1;
            break;
        case SAVGOL_WINDOW_SIZE_INCREASE:
            if (settings->data.savgol_filter.window_size < 100)
                settings->data.savgol_filter.window_size += 1;
            break;

        case INTERPOLATION_DECREASE:
            if (settings->data.interp > 0)
                settings->data.interp -= 1;
            break;
        case INTERPOLATION_INCREASE:
            if (settings->data.interp < settings->data.INTERP.MAX)
                settings->data.interp += 1;
            break;

        case AUDIO_BUFFER_SIZE_DECREASE:
            if (settings->data.audio_buffer_size > 1024)
                settings->data.audio_buffer_size /= 2;
            break;
        case AUDIO_BUFFER_SIZE_INCREASE:
            if (settings->data.audio_buffer_size < settings->data.AUDIO_BUFFER_SIZE.MAX)
                settings->data.audio_buffer_size *= 2;
            break;

        case FPS_DECREASE:
            if (settings->misc.fps > 1)
                settings->misc.fps -= 1;
            break;
        case FPS_INCREASE:
            if (settings->misc.fps < settings->misc.FPS.MAX)
                settings->misc.fps += 1;
            break;

        case STATS_TOGGLE:
            if (settings->misc.stats)
                settings->misc.stats = 0;
            else
               settings->misc.stats = 1;
            break;
    }
}


template <typename T>
static void set_const_setting(recidia_const_setting<T> *setting, libconfig::Setting &conf_setting) {

    conf_setting.lookupValue("min", setting->MIN);
    conf_setting.lookupValue("max", setting->MAX);
    conf_setting.lookupValue("default", setting->DEFAULT);
}

static void set_const_key(libconfig::Setting &conf_setting, const char *control, int change) {
    string stringKey;
    conf_setting.lookupValue(control, stringKey);

    char key = stringKey.c_str()[0];
    setttings_key_map[key] = change;
}

void get_settings(recidia_settings *settings, int GUI) {

    // Get location of settings.cfg
    libconfig::Config cfg;
    string homeDir = getenv("HOME");
    string configFileLocations[] = {"",
                                    "../",
                                    homeDir + "/.config/recidia/",
                                    "/etc/recidia/"};
    for (uint i=0; i < 4; i++) {
        try {
            cfg.readFile((configFileLocations[i] + "settings.cfg").c_str());
            break;
        }
        catch(const libconfig::FileIOException &fioex) {
            if (i == 3) {
                fprintf(stderr, "\nCould not find settings.cfg file\n"
                "Please check settings.cfg for suitable locations\n\n");
            }
        }
    }

    // Set settings from settings.cfg
    libconfig::Setting &root = cfg.getRoot();

    string settingsList[2] = {"shared_settings"};
    if (GUI)
        settingsList[1] = "gui_settings";
    else
        settingsList[1] = "terminal_settings";

    for(uint i=0; i < sizeof(settingsList)/sizeof(settingsList[0]); i++) {
        const libconfig::Setting &settingGroup = root[settingsList[i]];

        for(uint j=0; j < (uint) settingGroup.getLength(); j++) {
            libconfig::Setting &confSetting = settingGroup[j];

            string name;
            confSetting.lookupValue("name", name);

            switch (str2int(name.c_str())) {

                case str2int("Data Height Cap"):
                    confSetting.lookupValue("default", settings->data.height_cap);
                    set_const_setting(&settings->data.HEIGHT_CAP, confSetting);
                    set_const_key(confSetting, "decrease_key", PLOT_HEIGHT_CAP_DECREASE);
                    set_const_key(confSetting, "increase_key", PLOT_HEIGHT_CAP_INCREASE);
                    break;

                case str2int("Design Height"):
                    set_const_setting(&settings->design.HEIGHT, confSetting);
                    break;

                case str2int("Plot Width"):
                    confSetting.lookupValue("default", settings->design.plot_width);
                    set_const_setting(&settings->design.PLOT_WIDTH, confSetting);
                    set_const_key(confSetting, "decrease_key", PLOT_WIDTH_DECREASE);
                    set_const_key(confSetting, "increase_key", PLOT_WIDTH_INCREASE);
                    break;

                case str2int("Gap Width"):
                    confSetting.lookupValue("default", settings->design.gap_width);
                    set_const_setting(&settings->design.GAP_WIDTH, confSetting);
                    set_const_key(confSetting, "decrease_key", GAP_WIDTH_DECREASE);
                    set_const_key(confSetting, "increase_key", GAP_WIDTH_INCREASE);
                    break;

                case str2int("SavGol Filter"):
                    confSetting.lookupValue("window_size", settings->data.savgol_filter.window_size);
                    confSetting.lookupValue("poly_order", settings->data.savgol_filter.poly_order);
                    set_const_key(confSetting, "decrease_key", SAVGOL_WINDOW_SIZE_DECREASE);
                    set_const_key(confSetting, "increase_key", SAVGOL_WINDOW_SIZE_INCREASE);
                    break;

                case str2int("Interpolation"):
                    confSetting.lookupValue("default", settings->data.interp);
                    set_const_setting(&settings->data.INTERP, confSetting);
                    set_const_key(confSetting, "decrease_key", INTERPOLATION_DECREASE);
                    set_const_key(confSetting, "increase_key", INTERPOLATION_INCREASE);
                    break;

                case str2int("Audio Buffer Size"):
                    confSetting.lookupValue("default", settings->data.audio_buffer_size);
                    set_const_setting(&settings->data.AUDIO_BUFFER_SIZE, confSetting);
                    set_const_key(confSetting, "decrease_key", AUDIO_BUFFER_SIZE_DECREASE);
                    set_const_key(confSetting, "increase_key", AUDIO_BUFFER_SIZE_INCREASE);
                    break;

                case str2int("FPS"):
                    confSetting.lookupValue("default", settings->misc.fps);
                    set_const_setting(&settings->misc.FPS, confSetting);
                    set_const_key(confSetting, "decrease_key", FPS_DECREASE);
                    set_const_key(confSetting, "increase_key", FPS_INCREASE);
                    break;

                case str2int("Main Color"):
                    confSetting.lookupValue("red", settings->design.main_color.red);
                    confSetting.lookupValue("blue", settings->design.main_color.blue);
                    confSetting.lookupValue("green", settings->design.main_color.green);
                    confSetting.lookupValue("alpha", settings->design.main_color.alpha);
                    break;

                case str2int("Background Color"):
                    confSetting.lookupValue("red", settings->design.back_color.red);
                    confSetting.lookupValue("blue", settings->design.back_color.blue);
                    confSetting.lookupValue("green", settings->design.back_color.green);
                    confSetting.lookupValue("alpha", settings->design.back_color.alpha);
                    break;

                case str2int("Plot Chart Guide"):
                    confSetting.lookupValue("start_freq", settings->data.chart_guide.start_freq);
                    confSetting.lookupValue("start_ctrl", settings->data.chart_guide.start_ctrl);
                    confSetting.lookupValue("mid_freq", settings->data.chart_guide.mid_freq);
                    confSetting.lookupValue("mid_pos", settings->data.chart_guide.mid_pos);
                    confSetting.lookupValue("end_ctrl", settings->data.chart_guide.end_ctrl);
                    confSetting.lookupValue("end_freq", settings->data.chart_guide.end_freq);
                    break;

                case str2int("Stats"):
                    confSetting.lookupValue("enabled", settings->misc.stats);
                    set_const_key(confSetting, "toggle_key", STATS_TOGGLE);
                    break;
            }
        }
    }
}
