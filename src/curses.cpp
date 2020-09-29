#include <string>
#include <locale.h>
#include <chrono>
#include <unistd.h>

#include <ncurses.h>

#include <recidia.h>

using namespace std;

const char PLOT_HEIGHT_CAP_DECREASE = 't';
const char PLOT_HEIGHT_CAP_INCREASE = 'g';

const char PLOT_WIDTH_DECREASE = 'e';
const char PLOT_WIDTH_INCREASE = 'r';

const char GAP_WIDTH_DECREASE = 'd';
const char GAP_WIDTH_INCREASE = 'f';

const char SAVGOL_WINDOW_SIZE_DECREASE = 'q';
const char SAVGOL_WINDOW_SIZE_INCREASE = 'w';

const char INTERPOLATION_DECREASE = 'a';
const char INTERPOLATION_INCREASE = 's';

const char AUDIO_BUFFER_SIZE_DECREASE = 'z';
const char AUDIO_BUFFER_SIZE_INCREASE = 'x';

const char FPS_DECREASE = 'h';
const char FPS_INCREASE = 'y';

const char STATS_TOGGLE = 'i';


static void grab_input(recidia_setings *settings, uint plots_count) {
    // Get input
    int ch = getch();

    if (ch != -1) {
        switch (ch) {
            case KEY_MOUSE:
                MEVENT mouseEvent;

                if(getmouse(&mouseEvent) == OK) {
                    if (mouseEvent.bstate & BUTTON4_PRESSED) { // Scroll Up
                        if (settings->plot_height_cap > 1)
                            settings->plot_height_cap /= 1.25;
                    }
                    else if (mouseEvent.bstate & BUTTON5_PRESSED) {  // Scroll Down
                        if (settings->plot_height_cap < settings->MAX_PLOT_HEIGHT_CAP)
                            settings->plot_height_cap *= 1.25;
                    }
                }
                break;

            case PLOT_HEIGHT_CAP_DECREASE:
                if (settings->plot_height_cap < settings->MAX_PLOT_HEIGHT_CAP)
                    settings->plot_height_cap /= 1.5;
                break;
            case PLOT_HEIGHT_CAP_INCREASE:
                if (settings->plot_height_cap > 1)
                    settings->plot_height_cap *= 1.5;
                break;

            case PLOT_WIDTH_DECREASE:
                if (settings->plot_width > 1)
                    settings->plot_width -= 1;
                break;
            case PLOT_WIDTH_INCREASE:
                if (settings->plot_width < settings->MAX_PLOT_WIDTH)
                    settings->plot_width += 1;
                break;

            case GAP_WIDTH_DECREASE:
                if (settings->gap_width > 0)
                    settings->gap_width -= 1;
                break;
            case GAP_WIDTH_INCREASE:
                if (settings->gap_width < settings->MAX_GAP_WIDTH)
                    settings->gap_width += 1;
                break;

            case SAVGOL_WINDOW_SIZE_DECREASE:
                if (settings->savgol_filter[0] > 0)
                    settings->savgol_filter[0] -= 1;
                break;
            case SAVGOL_WINDOW_SIZE_INCREASE:
                if (settings->savgol_filter[0] < plots_count)
                    settings->savgol_filter[0] += 1;
                break;

            case INTERPOLATION_DECREASE:
                if (settings->interp > 0)
                    settings->interp -= 1;
                break;
            case INTERPOLATION_INCREASE:
                if (settings->interp < settings->MAX_INTEROPLATION)
                    settings->interp += 1;
                break;

            case AUDIO_BUFFER_SIZE_DECREASE:
                if (settings->audio_buffer_size > 1024)
                    settings->audio_buffer_size /= 2;
                break;
            case AUDIO_BUFFER_SIZE_INCREASE:
                if (settings->audio_buffer_size < settings->MAX_AUDIO_BUFFER_SIZE)
                    settings->audio_buffer_size *= 2;
                break;

            case FPS_DECREASE:
                if (settings->fps > 1)
                    settings->fps -= 1;
                break;
            case FPS_INCREASE:
                if (settings->fps < settings->MAX_FPS)
                    settings->fps += 1;
                break;

            case STATS_TOGGLE:
                if (settings->stats)
                    settings->stats = 0;
                else
                   settings->stats = 1;
                break;
        }
    }

}

void init_curses(recidia_setings *settings, recidia_data *data, recidia_sync *sync) {
    setlocale(LC_ALL, "");
    initscr();
    noecho();
    nodelay(stdscr, TRUE);
    curs_set(FALSE);
    keypad(stdscr, TRUE);
    mousemask(ALL_MOUSE_EVENTS, NULL);
    mouseinterval(0); // No double click

    uint i, j;
    uint ceiling;
    uint finalPlots[settings->MAX_AUDIO_BUFFER_SIZE/2];
    uint slices = 8;

    // Used to show settings changes
    double plotHeightCap = settings->plot_height_cap;
    uint plotWidth = settings->plot_width;
    uint gapWidth = settings->gap_width;
    uint savgolWindowSize = settings->savgol_filter[0];
    uint interp = settings->interp;
    uint audioBufferSize = settings->audio_buffer_size;
    uint plotsCount = data->width / (plotWidth + gapWidth);

    uint fps = settings->fps;

    string settingToDisplay;
    uint timeOfDisplayed = 0;
    const uint SECONDS_TO_DISPLAY = 2;

    uint frameCount = 0;
    float realfps = 0;
    double latency = 0;


    while (1) {
        getmaxyx(stdscr, data->height, data->width);

        // Track setting changes
        if (plotHeightCap != settings->plot_height_cap) {
            plotHeightCap = settings->plot_height_cap;

            timeOfDisplayed = 0;
            settingToDisplay = "Height Cap " + to_string((int) plotHeightCap);
        }
        if (plotWidth != settings->plot_width) {
            plotWidth = settings->plot_width;

            timeOfDisplayed = 0;
            settingToDisplay = "Plot Width " + to_string(plotWidth);
        }
        if (gapWidth != settings->gap_width) {
            gapWidth = settings->gap_width;

            timeOfDisplayed = 0;
            settingToDisplay = "Gap Width " + to_string(gapWidth);
        }
        if (savgolWindowSize != settings->savgol_filter[0]) {
            savgolWindowSize = settings->savgol_filter[0];

            timeOfDisplayed = 0;
            settingToDisplay = "Savgol Window Size " + to_string(savgolWindowSize);
        }
        if (interp != settings->interp) {
            interp = settings->interp;

            timeOfDisplayed = 0;
            settingToDisplay = "Interpolation " + to_string(interp) + "x";
        }
        if (audioBufferSize != settings->audio_buffer_size) {
            audioBufferSize = settings->audio_buffer_size;

            timeOfDisplayed = 0;
            settingToDisplay = "Audio Buffer Size " + to_string(audioBufferSize);
        }
        if (fps != settings->fps) {
            fps = settings->fps;

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
            finalPlots[i] = data->plots[i] * ((float) ceiling / plotHeightCap);
            if (finalPlots[i] > ceiling) {
                finalPlots[i] = ceiling;
            }
        }

        // Print bars
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

        if (settingToDisplay.compare("") != 0) {
            if (frameCount % (settings->fps / 1) == 0) {
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
        if (settings->stats) {
            if (frameCount % (settings->fps / 10) == 0) { // Slow down stats
                auto latencyChrono = chrono::high_resolution_clock::now();
                latency = chrono::duration<double>(latencyChrono.time_since_epoch()).count();
                latency = (latency - data->time) * 1000;

                realfps = 1000 / data->frame_time;
            }

            mvprintw(0, 0, "%s %.1fms", "Latency:" ,latency);
            mvprintw(1, 0, "%s %.1f", "FPS:" ,realfps);
            mvprintw(2, 0, "%s %i", "Plots:" ,plotsCount);
        }

        refresh(); 

        frameCount += 1;
        if (frameCount > 1000000)
            frameCount = 0;

        sync->status = 1; 
        while (sync->status) {
            usleep(1000);

            grab_input(settings, plotsCount);
        }
    }
}
