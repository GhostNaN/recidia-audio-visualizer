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
                    else {  // Scroll Down
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
    uint ceiling, plotsCount;
    uint finalPlots[settings->MAX_AUDIO_BUFFER_SIZE/2];
    uint slices = 8;


    while (1) {
        getmaxyx(stdscr, data->height, data->width);
        plotsCount = data->width / (settings->plot_width + settings->gap_width);
        ceiling = data->height * slices;

        // Finalize plots height
        for (i=0; i < plotsCount; i++ ) {

            // Scale plots
            finalPlots[i] = data->plots[i] * ((float) ceiling / settings->plot_height_cap);
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
                    for (j=0; j < settings->plot_width; j++) {
                        printBarLine += "\u2588";
                    }
                }
                else if ((limit - slices) < finalPlots[i]) {  // Part

                    unsigned int block = finalPlots[i] % slices;
                    string stringBlock = blockList[block - 1];
                    for (j=0; j < settings->plot_width; j++) {
                        printBarLine += stringBlock;
                    }
                }
                else {   // Empty
                    for (j=0; j < settings->plot_width; j++) {
                        printBarLine += " ";
                    }
                }
                for (j=0; j < settings->gap_width; j++) {
                    printBarLine += " ";
                }

            }
            mvprintw(y, 0, printBarLine.c_str());
        }

        auto latencyChrono = chrono::high_resolution_clock::now();
        double latency = chrono::duration<double>(latencyChrono.time_since_epoch()).count();
        latency -= data->time;
        latency *= 1000;

        if (settings->stats) {
            int spacing = 15;
            mvprintw(0, spacing * 0, "%s %i", "Height:" ,(int) settings->plot_height_cap);
            mvprintw(1, spacing * 0, "%s %i", "SavGol:" ,settings->savgol_filter[0]);
            mvprintw(2, spacing * 0, "%s %ix", "Interp:" ,settings->interp);
            mvprintw(3, spacing * 0, "%s %i", "Buffer:" ,settings->audio_buffer_size);

            mvprintw(0, spacing * 1, "%s %.1fms", "Latency:" ,latency);
            mvprintw(1, spacing * 1, "%s %.1f", "FPS:" ,1000 / data->frame_time);
            mvprintw(2, spacing * 1, "%s %i", "Plots:" ,plotsCount);
        }

        refresh(); 

        sync->status = 1; 
        while (sync->status) {
            usleep(1000);

            grab_input(settings, plotsCount);
        }
    }
}
