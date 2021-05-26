#include <unistd.h>
#include <string>
#include <cstring>
#include <locale.h>

#include <ncurses.h>

#include <recidia.h>

using namespace std;

static void set_colors() {
    if (recidia_settings.design.main_color.alpha || recidia_settings.design.back_color.alpha) {
        start_color();
        use_default_colors();
        short main_color = -1;
        short back_color = -1;

        if (recidia_settings.design.main_color.alpha) {
            main_color = 1;
            short main_red = recidia_settings.design.main_color.red / 0.255;
            short main_green = recidia_settings.design.main_color.green / 0.255;
            short main_blue = recidia_settings.design.main_color.blue / 0.255;
            init_color(main_color, main_red, main_green, main_blue);
        }
        if (recidia_settings.design.back_color.alpha) {
            back_color = 2;
            short back_red = recidia_settings.design.back_color.red / 0.255;
            short back_green = recidia_settings.design.back_color.green / 0.255;
            short back_blue = recidia_settings.design.back_color.blue / 0.255;
            init_color(back_color, back_red, back_green, back_blue);
        }

        init_pair(1, main_color, back_color);
        attron(COLOR_PAIR(1));
    }
}

static string *get_draw_chars(uint *draw_slices) {
    // If none or less than 2 chars in draw_chars, force defaults
    if (recidia_settings.design.draw_chars == NULL) {
        const char *backupDrawChars[] = {" ","▁","▂","▃","▄","▅","▆","▇","█"};
        recidia_settings.design.draw_chars = new char*[10];
        for (uint i=0; i < 9; i++) {
            recidia_settings.design.draw_chars[i] = new char[strlen(backupDrawChars[i])];
            strcpy(recidia_settings.design.draw_chars[i], backupDrawChars[i]);
        }
        recidia_settings.design.draw_chars[9] = new char[1];
        strcpy(recidia_settings.design.draw_chars[9], "\0");
    }
    
    uint charsCount = 0;
    for(uint i=0; strcmp(recidia_settings.design.draw_chars[i], "\0"); i++) {
        charsCount++;
    }
    string *charList = new string[charsCount];
    for(uint i=0; i < charsCount; i++) {
        charList[i] = recidia_settings.design.draw_chars[i];
    }
    *draw_slices = charsCount - 1;

    return charList;
}

void init_curses() {
    setlocale(LC_ALL, "");
    initscr();
    noecho();
    nodelay(stdscr, TRUE);
    curs_set(FALSE);
    keypad(stdscr, TRUE);
    mousemask(ALL_MOUSE_EVENTS, NULL);
    mouseinterval(0); // No double click

    set_colors();

    // Initialize vars
    uint i, j;
    uint ceiling;
    uint finalPlots[recidia_settings.data.AUDIO_BUFFER_SIZE.MAX / 2];
    uint frameCount = 0;
    float realfps = 0;

    // Used to show settings changes
    double plotHeightCap = recidia_settings.data.height_cap;
    uint plotWidth = recidia_settings.design.plot_width;
    uint gapWidth = recidia_settings.design.gap_width;
    float savgolWindowSize = recidia_settings.data.savgol_filter.window_size;
    uint interp = recidia_settings.data.interp;
    uint audioBufferSize = recidia_settings.data.audio_buffer_size;
    uint plotsCount = recidia_data.plots_count;
    uint fps = recidia_settings.design.fps_cap;
    uint poll_rate = recidia_settings.data.poll_rate;

    string settingToDisplay;
    uint timeOfDisplayed = 0;
    const uint SECONDS_TO_DISPLAY = 2;

    // Setup chars for drawing
    uint drawSlices = 0;
    string *charList = get_draw_chars(&drawSlices);

    while (1) {
        u_int64_t timerStart = utime_now();
        getmaxyx(stdscr, recidia_data.height, recidia_data.width);
        recidia_data.plots_count = (recidia_data.width / (recidia_settings.design.plot_width + recidia_settings.design.gap_width)) + 1;

        // Track setting changes
        if (plotHeightCap != recidia_settings.data.height_cap) {
            plotHeightCap = recidia_settings.data.height_cap;

            timeOfDisplayed = 0;
            settingToDisplay = "Height Cap " + to_string((int) (plotHeightCap+0.5));
        }
        if (plotWidth != recidia_settings.design.plot_width) {
            plotWidth = recidia_settings.design.plot_width;

            timeOfDisplayed = 0;
            settingToDisplay = "Plot Width " + to_string(plotWidth);

            clear();
        }
        if (gapWidth != recidia_settings.design.gap_width) {
            gapWidth = recidia_settings.design.gap_width;

            timeOfDisplayed = 0;
            settingToDisplay = "Gap Width " + to_string(gapWidth);

            clear();
        }
        if (savgolWindowSize != recidia_settings.data.savgol_filter.window_size) {
            savgolWindowSize = recidia_settings.data.savgol_filter.window_size;

            timeOfDisplayed = 0;
            settingToDisplay = "Savgol Window " + to_string((int) ((savgolWindowSize * 100) + 0.5)) + "%";
        }
        if (interp != recidia_settings.data.interp) {
            interp = recidia_settings.data.interp;

            timeOfDisplayed = 0;
            settingToDisplay = "Interpolation " + to_string(interp) + "x";
        }
        if (audioBufferSize != recidia_settings.data.audio_buffer_size) {
            audioBufferSize = recidia_settings.data.audio_buffer_size;

            timeOfDisplayed = 0;
            settingToDisplay = "Audio Buffer Size " + to_string(audioBufferSize);
        }
        if (poll_rate != recidia_settings.data.poll_rate) {
            poll_rate = recidia_settings.data.poll_rate;

            timeOfDisplayed = 0;
            settingToDisplay = "Poll Rate " + to_string(poll_rate) + "ms";
        }
        if (fps != recidia_settings.design.fps_cap) {
            fps = recidia_settings.design.fps_cap;

            timeOfDisplayed = 0;
            settingToDisplay = "FPS Cap " + to_string(fps);
        }
        if (plotsCount != recidia_data.plots_count) {
            plotsCount = recidia_data.plots_count;

            clear();
        }

        ceiling = recidia_data.height * drawSlices;

        // Finalize plots height
        for (i=0; i < plotsCount; i++ ) {

            // Scale plots
            finalPlots[i] = (recidia_data.plots[i] / plotHeightCap) * (float) ceiling;
            if (finalPlots[i] > ceiling) {
                finalPlots[i] = ceiling;
            }
        }

        // Print plots/bars
        for (uint y = 0; y < recidia_data.height; y++) {

            uint limit = ceiling - (y * drawSlices);
            string printBarLine;

            for (i=0; i < plotsCount; i++) {

                if (limit <= finalPlots[i]) {   // Full
                    for (j=0; j < plotWidth; j++) {
                        printBarLine += charList[drawSlices];
                    }
                }
                else if ((limit - drawSlices) < finalPlots[i]) {  // Part
                    for (j=0; j < plotWidth; j++) {
                        printBarLine += charList[finalPlots[i] % drawSlices];
                    }
                }
                else {   // Empty
                    for (j=0; j < plotWidth; j++) {
                        printBarLine += charList[0];
                    }
                }
                for (j=0; j < gapWidth; j++) {
                    printBarLine += charList[0];
                }

            }
            mvprintw(y, 0, printBarLine.c_str());
        }

        // Show changes in settings on scrren
        if (settingToDisplay.compare("") != 0) {
            if (frameCount % (recidia_settings.design.fps_cap / 1) == 0) {
                timeOfDisplayed += 1;
            }
            if (timeOfDisplayed < SECONDS_TO_DISPLAY) {
                uint printPos  = (recidia_data.width - settingToDisplay.length()) / 2;
                mvprintw(recidia_data.height / 2, printPos, "%s" ,settingToDisplay.c_str());
            }
            else {
                settingToDisplay = "";
            }
        }

        // Draw stats
        if (recidia_settings.data.stats) {
            if (frameCount % ((recidia_settings.design.fps_cap / 10) + 1) == 0) { // Slow down stats
                recidia_data.latency = (float) (utime_now() - recidia_data.start_time) / 1000;

                realfps = 1000 / recidia_data.frame_time;
            }

            mvprintw(0, 0, "%s %.1fms", "Latency:" ,recidia_data.latency);
            mvprintw(1, 0, "%s %.1f", "FPS:" ,realfps);
            mvprintw(2, 0, "%s %i", "Plots:" ,plotsCount);
        }

        // Draw frame
        refresh(); 

        frameCount += 1;
        if (frameCount > 1000000)
            frameCount = 0;

        int sleepTime = 1;
        while (sleepTime > 0) {
            u_int64_t delayTime = utime_now() - timerStart;

            sleepTime = (((1000 / (double) recidia_settings.design.fps_cap) * 1000) - delayTime);
            usleep(1000);

            // Get input
            int ch = getch();
            // Convert Mouse events to key
            if (ch == KEY_MOUSE) {
                MEVENT mouseEvent;

                if(getmouse(&mouseEvent) == OK) {
                    if (mouseEvent.bstate & BUTTON4_PRESSED) { // Scroll Up
                        if (recidia_settings.data.height_cap > 1) {
                            recidia_settings.data.height_cap /= 1.25;

                            if (recidia_settings.data.height_cap < 1)
                                recidia_settings.data.height_cap = 1;
                        }
                    }
                    else if (mouseEvent.bstate & BUTTON5_PRESSED) {  // Scroll Down
                        if (recidia_settings.data.height_cap < recidia_settings.data.HEIGHT_CAP.MAX) {
                            recidia_settings.data.height_cap *= 1.25;

                            if (recidia_settings.data.height_cap > recidia_settings.data.HEIGHT_CAP.MAX)
                                recidia_settings.data.height_cap = recidia_settings.data.HEIGHT_CAP.MAX;
                        }
                    }
                }
            }
            else {
                change_setting_by_key(ch);
            }
        }
        recidia_data.frame_time = (float) (utime_now() - timerStart) / 1000;
    }
}
