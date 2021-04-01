#include <string>
#include <cstring>
#include <map>

#include <libconfig.h++>

#include <recidia.h>

using namespace std;

static const char VERSION[] = "0.3";

map<char, int> setttings_key_map;

int get_setting_change(char key) {
    return setttings_key_map[key];
}

// Because templates didn't work
void limit_setting(float &setting, float min, float max) {
    if (setting > max)
        setting = max;
    else if (setting < min)
        setting = min;
}
void limit_setting(int &setting, int min, int max) {
    if (setting > max)
        setting = max;
    else if (setting < min)
        setting = min;
}
void limit_setting(uint &setting, uint min, uint max) {
    if (setting > max)
        setting = max;
    else if (setting < min)
        setting = min;
}

void change_setting_by_key(char key) {

    switch (setttings_key_map[key]) {
        case PLOT_HEIGHT_CAP_DECREASE:
            if (recidia_settings.data.height_cap > 1) {
                recidia_settings.data.height_cap /= 1.5;

                if (recidia_settings.data.height_cap < 1)
                    recidia_settings.data.height_cap = 1;
            }
            break;
        case PLOT_HEIGHT_CAP_INCREASE:
            if (recidia_settings.data.height_cap < recidia_settings.data.HEIGHT_CAP.MAX) {
                recidia_settings.data.height_cap *= 1.5;

                if (recidia_settings.data.height_cap > recidia_settings.data.HEIGHT_CAP.MAX)
                    recidia_settings.data.height_cap = recidia_settings.data.HEIGHT_CAP.MAX;
            }
            break;

        case PLOT_WIDTH_DECREASE:
            if (recidia_settings.design.plot_width > 1)
                recidia_settings.design.plot_width -= 1;
            break;
        case PLOT_WIDTH_INCREASE:
            if (recidia_settings.design.plot_width < recidia_settings.design.PLOT_WIDTH.MAX)
                recidia_settings.design.plot_width += 1;
            break;

        case GAP_WIDTH_DECREASE:
            if (recidia_settings.design.gap_width > 0)
                recidia_settings.design.gap_width -= 1;
            break;
        case GAP_WIDTH_INCREASE:
            if (recidia_settings.design.gap_width < recidia_settings.design.GAP_WIDTH.MAX)
                recidia_settings.design.gap_width += 1;
            break;

        case SAVGOL_WINDOW_SIZE_DECREASE:
            if (recidia_settings.data.savgol_filter.window_size > 0.0) {
                recidia_settings.data.savgol_filter.window_size -= 0.01;

                if (recidia_settings.data.savgol_filter.window_size < 0.0)
                    recidia_settings.data.savgol_filter.window_size = 0.0;
            }
            break;
        case SAVGOL_WINDOW_SIZE_INCREASE:
            if (recidia_settings.data.savgol_filter.window_size < 1.0)
                recidia_settings.data.savgol_filter.window_size += 0.01;
            else
                recidia_settings.data.savgol_filter.window_size = 1.0;
            break;

        case INTERPOLATION_DECREASE:
            if (recidia_settings.data.interp > 0)
                recidia_settings.data.interp -= 1;
            break;
        case INTERPOLATION_INCREASE:
            if (recidia_settings.data.interp < recidia_settings.data.INTERP.MAX)
                recidia_settings.data.interp += 1;
            break;

        case AUDIO_BUFFER_SIZE_DECREASE:
            if (recidia_settings.data.audio_buffer_size > 1024)
                recidia_settings.data.audio_buffer_size /= 2;
            break;
        case AUDIO_BUFFER_SIZE_INCREASE:
            if (recidia_settings.data.audio_buffer_size < recidia_settings.data.AUDIO_BUFFER_SIZE.MAX)
                recidia_settings.data.audio_buffer_size *= 2;
            break;

        case POLL_RATE_DECREASE:
            if (recidia_settings.data.poll_rate > 1)
                recidia_settings.data.poll_rate -= 1;
            break;
        case POLL_RATE_INCREASE:
            if (recidia_settings.data.poll_rate < recidia_settings.data.POLL_RATE.MAX)
                recidia_settings.data.poll_rate += 1;
            break;

        case FPS_CAP_DECREASE:
            if (recidia_settings.design.fps_cap > 1)
                recidia_settings.design.fps_cap -= 1;
            break;
        case FPS_CAP_INCREASE:
            if (recidia_settings.design.fps_cap < recidia_settings.design.FPS_CAP.MAX)
                recidia_settings.design.fps_cap += 1;
            break;

        case DRAW_MODE_TOGGLE:
            if (recidia_settings.design.draw_mode == 0)
                recidia_settings.design.draw_mode = 1;
            else if (recidia_settings.design.draw_mode == 1)
                recidia_settings.design.draw_mode = 0;
            break;

        case STATS_TOGGLE:
            if (recidia_settings.data.stats)
                recidia_settings.data.stats = false;
            else
               recidia_settings.data.stats = true;
            break;
    }
}

// If all else fails, hard coded default settings
void init_recidia_settings(int GUI) {

    if (GUI) {
        recidia_settings.design.plot_width = 14;
        recidia_settings.design.PLOT_WIDTH.MAX = 100;
        recidia_settings.design.gap_width = 6;
        recidia_settings.design.GAP_WIDTH.MAX = 100;
    }
    else {
        recidia_settings.design.plot_width = 1;
        recidia_settings.design.PLOT_WIDTH.MAX = 25;
        recidia_settings.design.gap_width = 1;
        recidia_settings.design.GAP_WIDTH.MAX = 25;
    }

    recidia_settings.misc.settings_menu = true;
    recidia_settings.misc.frameless = false;
    recidia_settings.design.draw_x = -1.0;
    recidia_settings.design.draw_y = -1.0;
    recidia_settings.design.draw_width = 1.0;
    recidia_settings.design.draw_height = 1.0;
    recidia_settings.design.min_plot_height = 0.0;
    recidia_settings.design.draw_mode = 0;
    recidia_settings.design.main_color = {255, 255, 255, 255};
    recidia_settings.design.back_color = {50, 50, 50, 150};
    recidia_settings.design.draw_chars = NULL;
    recidia_settings.data.height_cap = 500.0;
    recidia_settings.data.HEIGHT_CAP.MAX = 32768.0;
    recidia_settings.data.savgol_filter = {0.0, 3};
    recidia_settings.data.interp = 8;
    recidia_settings.data.INTERP.MAX = 32;
    recidia_settings.data.audio_buffer_size = 4096;
    recidia_settings.data.AUDIO_BUFFER_SIZE.MAX = 16384;
    recidia_settings.data.chart_guide = {0.0, 1.0, 1000.0, 0.66, 1.0, 12000.0};
    recidia_settings.data.poll_rate = 10;
    recidia_settings.data.POLL_RATE.MAX = 100;
    recidia_settings.design.fps_cap = 150;
    recidia_settings.design.FPS_CAP.MAX = 1000;
    recidia_settings.data.stats = false;
}


template <typename T>
static void set_const_setting(recidia_const_setting<T> *setting, libconfig::Setting &conf_setting) {
    string stringKey;

    conf_setting.lookupValue("min", setting->MIN);
    conf_setting.lookupValue("max", setting->MAX);
    conf_setting.lookupValue("default", setting->DEFAULT);

    conf_setting.lookupValue("decrease_key", stringKey);
    setting->KEY_DECREASE = stringKey.c_str()[0];
    conf_setting.lookupValue("increase_key", stringKey);
    setting->KEY_INCREASE = stringKey.c_str()[0];
}

static void set_const_key(libconfig::Setting &conf_setting, const char *control, int change) {
    string stringKey;
    conf_setting.lookupValue(control, stringKey);

    char key = stringKey.c_str()[0];
    setttings_key_map[key] = change;
}

// For Switch cases
constexpr uint str2int(const char* str, int h = 0) {
    return !str[h] ? 5381 : (str2int(str, h+1) * 33) ^ str[h];
}

void get_config_settings(int GUI) {

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
                "Please check settings.cfg for suitable locations\n"
                "Using hard coded defaults...\n\n");
                return;
            }
        }
    }

    // Check settings version number
    try {
        string version = cfg.lookup("version");
        if (version != VERSION)
            fprintf(stderr, "settings.cfg version mismatch!\n"
                            "Current: %s\n"
                            "Required: %s\n\n", version.c_str(), VERSION);
    }
    catch(const libconfig::SettingNotFoundException &nfex) {
        fprintf(stderr, "settings.cfg version number not found!\n"
                        "Expect the unexpected\n\n");
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

                case str2int("Settings Menu"):
                    confSetting.lookupValue("default", recidia_settings.misc.settings_menu);
                    set_const_key(confSetting, "toggle_key", SETTINGS_MENU_TOGGLE);
                    break;

                case str2int("Frameless"):
                    confSetting.lookupValue("default", recidia_settings.misc.frameless);
                    set_const_key(confSetting, "toggle_key", FRAMELESS_TOGGLE);
                    break;

                case str2int("Draw X"):
                    confSetting.lookupValue("default", recidia_settings.design.draw_x);
                    limit_setting(recidia_settings.design.draw_x, -1.0, 1.0);
                    break;

                case str2int("Draw Y"):
                    confSetting.lookupValue("default", recidia_settings.design.draw_y);
                    limit_setting(recidia_settings.design.draw_y, -1.0, 1.0);
                    break;

                case str2int("Draw Width"):
                    confSetting.lookupValue("default", recidia_settings.design.draw_width);
                    limit_setting(recidia_settings.design.draw_width, 0.0, 1.0);
                    break;

                case str2int("Draw Height"):
                    confSetting.lookupValue("default", recidia_settings.design.draw_height);
                    limit_setting(recidia_settings.design.draw_height, 0.0, 1.0);
                    break;

                case str2int("Min Plot Height"):
                    confSetting.lookupValue("default", recidia_settings.design.min_plot_height);
                    limit_setting(recidia_settings.design.min_plot_height, 0.0, 1.0);
                    break;

                case str2int("Plot Width"):
                    confSetting.lookupValue("default", recidia_settings.design.plot_width);
                    set_const_setting(&recidia_settings.design.PLOT_WIDTH, confSetting);
                    limit_setting(recidia_settings.design.plot_width, 1, recidia_settings.design.PLOT_WIDTH.MAX);
                    set_const_key(confSetting, "decrease_key", PLOT_WIDTH_DECREASE);
                    set_const_key(confSetting, "increase_key", PLOT_WIDTH_INCREASE);
                    break;

                case str2int("Gap Width"):
                    confSetting.lookupValue("default", recidia_settings.design.gap_width);
                    set_const_setting(&recidia_settings.design.GAP_WIDTH, confSetting);
                    limit_setting(recidia_settings.design.gap_width, 0, recidia_settings.design.GAP_WIDTH.MAX);
                    set_const_key(confSetting, "decrease_key", GAP_WIDTH_DECREASE);
                    set_const_key(confSetting, "increase_key", GAP_WIDTH_INCREASE);
                    break;

                case str2int("Plot Chars"):
                {
                    const libconfig::Setting &plotCharsSetting = confSetting.lookup("chars");
                    int charsCount = plotCharsSetting.getLength();

                    if (charsCount >= 2) { // Must be at least 2 chars              
                        recidia_settings.design.draw_chars = new char*[charsCount + 1];
                        for (int i=0; i < charsCount; i++) {
                            recidia_settings.design.draw_chars[i] = new char[strlen(plotCharsSetting[i].c_str())];
                            strcpy(recidia_settings.design.draw_chars[i], plotCharsSetting[i].c_str());
                        }
                        recidia_settings.design.draw_chars[charsCount] = new char[1];
                        strcpy(recidia_settings.design.draw_chars[charsCount], "\0");
                    }
                    break;
                }

                case str2int("Data Height Cap"):
                    confSetting.lookupValue("default", recidia_settings.data.height_cap);
                    set_const_setting(&recidia_settings.data.HEIGHT_CAP, confSetting);
                    limit_setting(recidia_settings.data.height_cap, 1.0, recidia_settings.data.HEIGHT_CAP.MAX);
                    set_const_key(confSetting, "decrease_key", PLOT_HEIGHT_CAP_DECREASE);
                    set_const_key(confSetting, "increase_key", PLOT_HEIGHT_CAP_INCREASE);
                    break;

                case str2int("SavGol Filter"):
                    confSetting.lookupValue("window_size", recidia_settings.data.savgol_filter.window_size);
                    limit_setting(recidia_settings.data.savgol_filter.window_size, 0.0, 1.0);
                    confSetting.lookupValue("poly_order", recidia_settings.data.savgol_filter.poly_order);
                    set_const_key(confSetting, "decrease_key", SAVGOL_WINDOW_SIZE_DECREASE);
                    set_const_key(confSetting, "increase_key", SAVGOL_WINDOW_SIZE_INCREASE);
                    break;

                case str2int("Interpolation"):
                    confSetting.lookupValue("default", recidia_settings.data.interp);
                    set_const_setting(&recidia_settings.data.INTERP, confSetting);
                    limit_setting(recidia_settings.data.interp, 0, recidia_settings.data.INTERP.MAX);
                    set_const_key(confSetting, "decrease_key", INTERPOLATION_DECREASE);
                    set_const_key(confSetting, "increase_key", INTERPOLATION_INCREASE);
                    break;

                case str2int("Audio Buffer Size"):
                    confSetting.lookupValue("default", recidia_settings.data.audio_buffer_size);
                    set_const_setting(&recidia_settings.data.AUDIO_BUFFER_SIZE, confSetting);
                    limit_setting(recidia_settings.data.audio_buffer_size, 0, recidia_settings.data.AUDIO_BUFFER_SIZE.MAX);
                    set_const_key(confSetting, "decrease_key", AUDIO_BUFFER_SIZE_DECREASE);
                    set_const_key(confSetting, "increase_key", AUDIO_BUFFER_SIZE_INCREASE);
                    break;

                case str2int("Poll Rate"):
                    confSetting.lookupValue("default", recidia_settings.data.poll_rate);
                    set_const_setting(&recidia_settings.data.POLL_RATE, confSetting);
                    limit_setting(recidia_settings.data.poll_rate, 1, recidia_settings.data.POLL_RATE.MAX);
                    set_const_key(confSetting, "decrease_key", POLL_RATE_DECREASE);
                    set_const_key(confSetting, "increase_key", POLL_RATE_INCREASE);
                    break;

                case str2int("FPS Cap"):
                    confSetting.lookupValue("default", recidia_settings.design.fps_cap);
                    set_const_setting(&recidia_settings.design.FPS_CAP, confSetting);
                    limit_setting(recidia_settings.design.fps_cap, 1, recidia_settings.design.FPS_CAP.MAX);
                    set_const_key(confSetting, "decrease_key", FPS_CAP_DECREASE);
                    set_const_key(confSetting, "increase_key", FPS_CAP_INCREASE);
                    break;

                case str2int("Main Color"):
                    confSetting.lookupValue("red", recidia_settings.design.main_color.red);
                    limit_setting(recidia_settings.design.main_color.red, 0, 255);
                    confSetting.lookupValue("green", recidia_settings.design.main_color.green);
                    limit_setting(recidia_settings.design.main_color.green, 0, 255);
                    confSetting.lookupValue("blue", recidia_settings.design.main_color.blue);
                    limit_setting(recidia_settings.design.main_color.blue, 0, 255);
                    confSetting.lookupValue("alpha", recidia_settings.design.main_color.alpha);
                    limit_setting(recidia_settings.design.main_color.alpha, 0, 255);
                    break;

                case str2int("Background Color"):
                    confSetting.lookupValue("red", recidia_settings.design.back_color.red);
                    limit_setting(recidia_settings.design.back_color.red, 0, 255);
                    confSetting.lookupValue("green", recidia_settings.design.back_color.green);
                    limit_setting(recidia_settings.design.back_color.green, 0, 255);
                    confSetting.lookupValue("blue", recidia_settings.design.back_color.blue);
                    limit_setting(recidia_settings.design.back_color.blue, 0, 255);
                    confSetting.lookupValue("alpha", recidia_settings.design.back_color.alpha);
                    limit_setting(recidia_settings.design.back_color.alpha, 0, 255);
                    break;

                case str2int("Plot Chart Guide"):
                    confSetting.lookupValue("start_freq", recidia_settings.data.chart_guide.start_freq);
                    confSetting.lookupValue("start_ctrl", recidia_settings.data.chart_guide.start_ctrl);
                    confSetting.lookupValue("mid_freq", recidia_settings.data.chart_guide.mid_freq);
                    confSetting.lookupValue("mid_pos", recidia_settings.data.chart_guide.mid_pos);
                    limit_setting(recidia_settings.data.chart_guide.mid_pos, 0.0, 1.0);
                    confSetting.lookupValue("end_ctrl", recidia_settings.data.chart_guide.end_ctrl);
                    confSetting.lookupValue("end_freq", recidia_settings.data.chart_guide.end_freq);
                    break;

                case str2int("Draw Mode"):
                    confSetting.lookupValue("mode", recidia_settings.design.draw_mode);
                    limit_setting(recidia_settings.design.draw_mode, 0, 1);
                    set_const_key(confSetting, "toggle_key", DRAW_MODE_TOGGLE);
                    break;

                case str2int("Stats"):
                    confSetting.lookupValue("enabled", recidia_settings.data.stats);
                    set_const_key(confSetting, "toggle_key", STATS_TOGGLE);
                    break;

                default:
                    fprintf(stderr, "Cannot recognize setting: \"%s\"\n", name.c_str());
            }
        }
    }
}
