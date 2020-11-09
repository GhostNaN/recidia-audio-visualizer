#include <unistd.h>
#include <string>
#include <locale.h>
#include <chrono>
#include <math.h>

#include <ncurses.h>

#include <recidia.h>

using namespace std;



void init_curses(recidia_settings *settings, recidia_data *data, recidia_sync *sync) {
    setlocale(LC_ALL, "");
    initscr();
    noecho();
    nodelay(stdscr, TRUE);
    curs_set(FALSE);
    keypad(stdscr, TRUE);
    mousemask(ALL_MOUSE_EVENTS, NULL);
    mouseinterval(0); // No double click

    // Initialize vars
    uint i, j;
    uint ceiling;
    uint finalPlots[settings->data.AUDIO_BUFFER_SIZE.MAX / 2];
    uint frameCount = 0;
    float realfps = 0;
    double latency = 0;
    uint slices = 8;    // How many pieces/slices of a char block is shown

    // Used to show settings changes
    double plotHeightCap = settings->data.height_cap;
    uint plotWidth = settings->design.plot_width;
    uint gapWidth = settings->design.gap_width;
    uint savgolWindowSize = settings->data.savgol_filter.window_size;
    uint interp = settings->data.interp;
    uint audioBufferSize = settings->data.audio_buffer_size;
    uint plotsCount = data->width / (plotWidth + gapWidth);
    uint fps = settings->misc.fps;

    string settingToDisplay;
    uint timeOfDisplayed = 0;
    const uint SECONDS_TO_DISPLAY = 2;

    while (1) {
        getmaxyx(stdscr, data->height, data->width);

        // Track setting changes
        if (plotHeightCap != settings->data.height_cap) {
            plotHeightCap = settings->data.height_cap;

            float db  = 20 * log10(plotHeightCap / 32768);
            char dbCharArrayt[10];
            sprintf(dbCharArrayt, "%.1f", db);
            string dbString(dbCharArrayt);

            timeOfDisplayed = 0;
            settingToDisplay = "Volume Cap " + dbString + "db";
        }
        if (plotWidth != settings->design.plot_width) {
            plotWidth = settings->design.plot_width;

            timeOfDisplayed = 0;
            settingToDisplay = "Plot Width " + to_string(plotWidth);

            clear();
        }
        if (gapWidth != settings->design.gap_width) {
            gapWidth = settings->design.gap_width;

            timeOfDisplayed = 0;
            settingToDisplay = "Gap Width " + to_string(gapWidth);

            clear();
        }
        if (savgolWindowSize != settings->data.savgol_filter.window_size) {
            savgolWindowSize = settings->data.savgol_filter.window_size;

            timeOfDisplayed = 0;
            settingToDisplay = "Savgol Window Size " + to_string(savgolWindowSize);
        }
        if (interp != settings->data.interp) {
            interp = settings->data.interp;

            timeOfDisplayed = 0;
            settingToDisplay = "Interpolation " + to_string(interp) + "x";
        }
        if (audioBufferSize != settings->data.audio_buffer_size) {
            audioBufferSize = settings->data.audio_buffer_size;

            timeOfDisplayed = 0;
            settingToDisplay = "Audio Buffer Size " + to_string(audioBufferSize);
        }
        if (fps != settings->misc.fps) {
            fps = settings->misc.fps;

            timeOfDisplayed = 0;
            settingToDisplay = "FPS " + to_string(fps);
        }
        if (plotsCount != (data->width / (plotWidth + gapWidth))) {
            plotsCount = data->width / (plotWidth + gapWidth);

            clear();
        }

        ceiling = data->height * slices;

        // Finalize plots height
        for (i=0; i < plotsCount; i++ ) {

            // Scale plots
            finalPlots[i] = (data->plots[i] / plotHeightCap) * (float) ceiling;
            if (finalPlots[i] > ceiling) {
                finalPlots[i] = ceiling;
            }
        }

        // Print plots/bars
        string blockList[] = {"\u2581", "\u2582", "\u2583", "\u2584", "\u2585", "\u2586", "\u2587"};

        for (unsigned int y = 0; y < data->height; y++) {

            unsigned int limit = ceiling - (y * slices);
            string printBarLine;

            for (i=0; i < plotsCount; i++) {

                if (limit <= finalPlots[i]) {   // Full
                    for (j=0; j < plotWidth; j++) {
                        printBarLine += "\u2588";
                    }
                }
                else if ((limit - slices) < finalPlots[i]) {  // Part

                    unsigned int block = finalPlots[i] % slices;
                    string stringBlock = blockList[block - 1];
                    for (j=0; j < plotWidth; j++) {
                        printBarLine += stringBlock;
                    }
                }
                else {   // Empty
                    for (j=0; j < plotWidth; j++) {
                        printBarLine += " ";
                    }
                }
                for (j=0; j < gapWidth; j++) {
                    printBarLine += " ";
                }

            }
            mvprintw(y, 0, printBarLine.c_str());
        }

        // Show changes in settings on scrren
        if (settingToDisplay.compare("") != 0) {
            if (frameCount % (settings->misc.fps / 1) == 0) {
                timeOfDisplayed += 1;
            }
            if (timeOfDisplayed < SECONDS_TO_DISPLAY) {
                uint printPos  = (data->width - settingToDisplay.length()) / 2;
                mvprintw(data->height / 2, printPos, "%s" ,settingToDisplay.c_str());
            }
            else {
                settingToDisplay = "";
            }
        }

        // Draw stats
        if (settings->misc.stats) {
            if (frameCount % (settings->misc.fps / 10) == 0) { // Slow down stats
                auto latencyChrono = chrono::high_resolution_clock::now();
                latency = chrono::duration<double>(latencyChrono.time_since_epoch()).count();
                latency = (latency - data->time) * 1000;

                realfps = 1000 / data->frame_time;
            }

            mvprintw(0, 0, "%s %.1fms", "Latency:" ,latency);
            mvprintw(1, 0, "%s %.1f", "FPS:" ,realfps);
            mvprintw(2, 0, "%s %i", "Plots:" ,plotsCount);
        }

        // Draw frame
        refresh(); 

        frameCount += 1;
        if (frameCount > 1000000)
            frameCount = 0;

        // Wait and process user input
        sync->status = 1; 
        while (sync->status) {
            usleep(1000);

            // Get input
            int ch = getch();
            // Convert Mouse events to key
            if (ch == KEY_MOUSE) {
                MEVENT mouseEvent;

                if(getmouse(&mouseEvent) == OK) {
                    if (mouseEvent.bstate & BUTTON4_PRESSED) { // Scroll Up
                        if (settings->data.height_cap > 1) {
                            settings->data.height_cap /= 1.25;

                            if (settings->data.height_cap < 1)
                                settings->data.height_cap = 1;
                        }
                    }
                    else if (mouseEvent.bstate & BUTTON5_PRESSED) {  // Scroll Down
                        if (settings->data.height_cap < settings->data.HEIGHT_CAP.MAX) {
                            settings->data.height_cap *= 1.25;

                            if (settings->data.height_cap > settings->data.HEIGHT_CAP.MAX)
                                settings->data.height_cap = settings->data.HEIGHT_CAP.MAX;
                        }
                    }
                }
            }
            else {
                change_setting_by_key(settings, ch);
            }
        }
    }
}
