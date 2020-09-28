#include <string>

#include <libconfig.h++>

#include <recidia.h>

using namespace std;
using namespace libconfig;

// For Switch cases
constexpr uint str2int(const char* str, int h = 0) {
    return !str[h] ? 5381 : (str2int(str, h+1) * 33) ^ str[h];
}

void get_settings(recidia_setings *settings) {

    // Set config settings
    Config cfg;
    try {
        cfg.readFile("../settings.cfg");
    }
    catch(const FileIOException &fioex) {
        // Take 2
        try {
            cfg.readFile("settings.cfg");
        }
        catch(const FileIOException &fioex) {
            printf("\nCould not find settings.cfg file\n"
            "Please have the file in the same dir as the executable\n\n");
        }
    }
    const Setting &root = cfg.getRoot();

    const Setting &controlledSettings = root["controlled_settings"];

    for(uint i=0; i < (uint) controlledSettings.getLength(); i++) {
        const Setting &setting = controlledSettings[i];

        string name;
        setting.lookupValue("name", name);

        int settingInt;
        switch (str2int(name.c_str())) {

            case str2int("Plot Height Cap"):
                setting.lookupValue("max", settingInt);
                settings->MAX_PLOT_HEIGHT_CAP = settingInt;
                setting.lookupValue("default", settingInt);
                settings->plot_height_cap = settingInt;
                break;

            case str2int("Plot Width"):
                setting.lookupValue("max", settingInt);
                settings->MAX_PLOT_WIDTH = settingInt;
                setting.lookupValue("default", settingInt);
                settings->plot_width = settingInt;
                break;

            case str2int("Gap Width"):
                setting.lookupValue("max", settingInt);
                settings->MAX_GAP_WIDTH = settingInt;
                setting.lookupValue("default", settingInt);
                settings->gap_width = settingInt;
                break;

            case str2int("SavGol Filter"):
                setting.lookupValue("window_size", settings->savgol_filter[0]);
                setting.lookupValue("poly_order", settings->savgol_filter[1]);
                break;

            case str2int("Interpolation"):
                setting.lookupValue("max", settingInt);
                settings->MAX_INTEROPLATION = settingInt;
                setting.lookupValue("default", settingInt);
                settings->interp = (uint) settingInt;
                break;

            case str2int("Audio Buffer Size"):
                setting.lookupValue("max", settingInt);
                settings->MAX_AUDIO_BUFFER_SIZE = settingInt;
                setting.lookupValue("default", settingInt);
                settings->audio_buffer_size = settingInt;
                break;
            case str2int("FPS"):
                setting.lookupValue("max", settingInt);
                settings->MAX_FPS = settingInt;
                setting.lookupValue("default", settingInt);
                settings->fps = settingInt;
                break;
            case str2int("Stats"):
                setting.lookupValue("enabled", settings->stats);
                break;
        }
    }

    const Setting &miscSettings = root["misc_settings"];

    for(uint i=0; i < (uint) miscSettings.getLength(); i++) {
        const Setting &setting = miscSettings[i];

        string name;
        setting.lookupValue("name", name);

        switch (str2int(name.c_str())) {
            case str2int("Plot Chart Guide"):
                setting.lookupValue("start_freq", settings->chart_guide[0]);
                setting.lookupValue("start_ctrl", settings->chart_guide[1]);
                setting.lookupValue("mid_freq", settings->chart_guide[2]);
                setting.lookupValue("mid_pos", settings->chart_guide[3]);
                setting.lookupValue("end_ctrl", settings->chart_guide[4]);
                setting.lookupValue("end_freq", settings->chart_guide[5]);
                break;
        }
    }
}
