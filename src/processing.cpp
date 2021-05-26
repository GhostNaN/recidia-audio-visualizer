#include <unistd.h>
#include <cmath>
#include <algorithm>
#include <vector>

#include <fftw3.h>
#include <gsl/gsl_linalg.h>

#include <recidia.h>

using namespace std;

static void create_chart_table(uint chart_size, uint *chart_table, recidia_audio_data *audio_data) {

    uint i, j;

    float beizerTable[chart_size];

    float plotFreq = (float) audio_data->sample_rate / (float) recidia_settings.data.audio_buffer_size;

    float startPoint = recidia_settings.data.chart_guide.start_freq / plotFreq;
    float startCtrl = startPoint * recidia_settings.data.chart_guide.start_ctrl;
    float midPoint = recidia_settings.data.chart_guide.mid_freq / plotFreq;
    uint midPointPos = round(chart_size * recidia_settings.data.chart_guide.mid_pos);
    float midCtrl = midPoint * recidia_settings.data.chart_guide.end_ctrl;
    float endPoint = recidia_settings.data.chart_guide.end_freq / plotFreq;

    uint samples;
    float p0, p2, c, n;
    uint k = 0; // beizerTable counter
    for (j=0; j < 2; j++) {

        if (j == 0) {
            n = midPointPos;
            samples = n;
            p0 = startPoint;
            p2 = midPoint;
            c = startCtrl;
        }
        else {
            n = chart_size - midPointPos;
            samples = n + 1;
            p0 = midPoint;
            p2 = endPoint;
            c = midCtrl;
        }

        for(i=0; i < samples; i++) {
            float t = (float) i / n;
            beizerTable[k] = (1 - t) * ((1-t) * p0 + t * c) + t * ((1-t) * c + t * p2);
            k++;
        }
    }

    float stepSize = 1;
    float limit = (float) recidia_settings.data.audio_buffer_size / 2;
    float prevStep;
    float nextStep = beizerTable[0];
    chart_table[0] = (uint) beizerTable[0];
    for (i=1; i < chart_size+1; i++) {
        prevStep = nextStep;
        nextStep = stepSize * round(beizerTable[i] / stepSize);

        if (beizerTable[i-1] <= beizerTable[i]) {
            if (nextStep - prevStep <= 0) {
                nextStep = prevStep + stepSize;
            }
        }
        else {
            if (prevStep - nextStep <= 0) {
                nextStep = prevStep - stepSize;
            }
        }

        float maxVal = *max_element(beizerTable, beizerTable + chart_size+1);
        if (nextStep > maxVal)
            nextStep = round(maxVal - stepSize);
        if (nextStep < 0)
            nextStep = stepSize;
        if (nextStep > limit)
            nextStep = limit - stepSize;
        if (prevStep == nextStep)
            nextStep += stepSize;

        chart_table[i] = (uint) nextStep;
    }
}

/**
 * Info: Compute the (Moore-Penrose) pseudo-inverse of a libgsl matrix in plain C.
 * Stolen and Modified From: Charl Linssen <charl@itfromb.it>
 * Created: Feb 2016
 * Copyright: PUBLIC DOMAIN
**/
static gsl_matrix* moore_penrose_pinv(gsl_matrix *A, const double rcond) {

    gsl_matrix *V, *Sigma_pinv, *U, *A_pinv;
    gsl_matrix *_tmp_mat = NULL;
    gsl_vector *_tmp_vec;
    gsl_vector *u;
    double x, cutoff;
    size_t i, j;
    unsigned int n = A->size1;
    unsigned int m = A->size2;

    /* do SVD */
    V = gsl_matrix_alloc(m, m);
    u = gsl_vector_alloc(m);
    _tmp_vec = gsl_vector_alloc(m);
    gsl_linalg_SV_decomp(A, V, u, _tmp_vec);
    gsl_vector_free(_tmp_vec);

    /* compute Σ⁻¹ */
    Sigma_pinv = gsl_matrix_alloc(m, n);
    gsl_matrix_set_zero(Sigma_pinv);
    cutoff = rcond * gsl_vector_max(u);

    for (i = 0; i < m; ++i) {
        if (gsl_vector_get(u, i) > cutoff) {
            x = 1. / gsl_vector_get(u, i);
        }
        else {
            x = 0.;
        }
        gsl_matrix_set(Sigma_pinv, i, i, x);
    }

    /* libgsl SVD yields "thin" SVD - pad to full matrix by adding zeros */
    U = gsl_matrix_alloc(n, n);
    gsl_matrix_set_zero(U);

    for (i = 0; i < n; ++i) {
        for (j = 0; j < m; ++j) {
            gsl_matrix_set(U, i, j, gsl_matrix_get(A, i, j));
        }
    }

    if (_tmp_mat != NULL) {
        gsl_matrix_free(_tmp_mat);
    }

    /* two dot products to obtain pseudoinverse */
    _tmp_mat = gsl_matrix_alloc(m, n);
    gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1., V, Sigma_pinv, 0., _tmp_mat);

    A_pinv = gsl_matrix_alloc(m, n);
    gsl_blas_dgemm(CblasNoTrans, CblasTrans, 1., _tmp_mat, U, 0., A_pinv);

    gsl_matrix_free(_tmp_mat);
    gsl_matrix_free(U);
    gsl_matrix_free(Sigma_pinv);
    gsl_vector_free(u);
    gsl_matrix_free(V);

    return A_pinv;
}

static vector<float> get_savgol_coeffs(int window_size, int poly_order ) {
    const int halfWindow = (window_size - 1) / 2;

    gsl_matrix *A = gsl_matrix_alloc(window_size, poly_order+1);
    gsl_matrix *A_pinv;

    uint i = 0;
    for (int x=-halfWindow; x < halfWindow+1; x++ ) {
        for (int k=0; k < poly_order + 1; k++ ) {
            gsl_matrix_set(A, i, k, pow(x, k));
        }
        i++;
    }
    const double rcond = 1E-15;
    A_pinv = moore_penrose_pinv(A, rcond);

    vector<float> out;
    for(i=0; i < A_pinv->size2; i++) {
        out.push_back((float) gsl_matrix_get(A_pinv, 0, i));
    }
    reverse(out.begin(), out.end());

    gsl_matrix_free(A);
    gsl_matrix_free(A_pinv);

    return out;
}
//[[ 1 -2  4 -8] [ 1 -1  1 -1] [ 1  0  0  0] [ 1  1  1  1] [ 1  2  4  8]]
// [-0.08571429  0.34285714  0.48571429  0.34285714 -0.08571429]

void init_processing(recidia_audio_data *audio_data) {
    // Allocate Default Vars
    uint i, j;
    uint interpIndex = 0;

    // Copy of some settings
    uint audioBufferSize = recidia_settings.data.audio_buffer_size;
    uint interp = recidia_settings.data.interp;
    uint plotsCount = recidia_data.plots_count;
    float savgolRelativeWindowSize = recidia_settings.data.savgol_filter.window_size;
    uint savgolWindowSize = savgolRelativeWindowSize * plotsCount;

    double *fftIn = (double*) fftw_malloc(sizeof(double) * recidia_settings.data.AUDIO_BUFFER_SIZE.MAX);
    double *fftOut = (double*) fftw_malloc(sizeof(double) * recidia_settings.data.AUDIO_BUFFER_SIZE.MAX);
    fftw_plan fftPlan = fftw_plan_r2r_1d(audioBufferSize, fftIn, fftOut, FFTW_R2HC, FFTW_MEASURE);

    float interpArray[recidia_settings.data.INTERP.MAX][recidia_settings.data.AUDIO_BUFFER_SIZE.MAX/2];
    float proArray[recidia_settings.data.AUDIO_BUFFER_SIZE.MAX/2];
    uint chartTable[recidia_settings.data.AUDIO_BUFFER_SIZE.MAX/2];
    vector<float> pinvVector;

    create_chart_table(plotsCount, chartTable, audio_data);
    if (recidia_settings.data.savgol_filter.window_size > recidia_settings.data.savgol_filter.poly_order + 1)
        pinvVector = get_savgol_coeffs(savgolWindowSize, recidia_settings.data.savgol_filter.poly_order);

    while (1) {
        u_int64_t timerStart = utime_now();

        // Handling volatile vars
        if (audioBufferSize != recidia_settings.data.audio_buffer_size) {
            audioBufferSize = recidia_settings.data.audio_buffer_size;

            fftw_destroy_plan(fftPlan);
            fftPlan = fftw_plan_r2r_1d(audioBufferSize, fftIn, fftOut, FFTW_R2HC, FFTW_MEASURE);
            create_chart_table(plotsCount, chartTable, audio_data);
        }
        if (interp != recidia_settings.data.interp) {
            interp = recidia_settings.data.interp;

            interpIndex = 0;
        }
        if (plotsCount != recidia_data.plots_count) {
            plotsCount = recidia_data.plots_count;

            if (savgolRelativeWindowSize) {
                savgolWindowSize = savgolRelativeWindowSize * plotsCount;
                if (savgolWindowSize % 2 == 0) savgolWindowSize += 1;
                if (savgolWindowSize < recidia_settings.data.savgol_filter.poly_order+2)
                    savgolWindowSize = recidia_settings.data.savgol_filter.poly_order+2;
                pinvVector = get_savgol_coeffs(savgolWindowSize, recidia_settings.data.savgol_filter.poly_order);
            }

            create_chart_table(plotsCount, chartTable, audio_data);
        }
        if (savgolRelativeWindowSize != recidia_settings.data.savgol_filter.window_size) {
            savgolRelativeWindowSize = recidia_settings.data.savgol_filter.window_size;

            if (savgolRelativeWindowSize) {
                savgolWindowSize = savgolRelativeWindowSize * plotsCount;
                if (savgolWindowSize % 2 == 0) savgolWindowSize += 1;
                if (savgolWindowSize < recidia_settings.data.savgol_filter.poly_order+2)
                    savgolWindowSize = recidia_settings.data.savgol_filter.poly_order+2;
                pinvVector = get_savgol_coeffs(savgolWindowSize, recidia_settings.data.savgol_filter.poly_order);
            }
            else
                savgolWindowSize = 0;
        }

        // Convert samples to double for FFTW
        for (i=0; i < audioBufferSize; i++ ) {
            fftIn[i] = (double) audio_data->samples[i];
            // Set Freq test
//            fftIn[i] =  32768 * sin(2 * 3.1415 * 5000 * (i / (double) 44100));
        }

        // For latency display
        recidia_data.start_time = utime_now();

        fftw_execute(fftPlan);

        // Absolute and normalized of FFT output
        for (i=1; i < audioBufferSize / 2; i++ ) {
            // i is +1 to throw away fftOut[0] and only half the data is usable
            fftOut[i-1] = abs(fftOut[i]) / (double) audioBufferSize;
        }

        // Apply plot table
        for (i=0; i < plotsCount; i++) {
            uint minNum = chartTable[i];
            uint maxNum = chartTable[i+1];

            if (maxNum > minNum)
                proArray[i] = *max_element(fftOut + minNum, fftOut + maxNum);
            else
                proArray[i] = *max_element(fftOut + maxNum, fftOut + minNum);
        }

        // Savitzky Golay Filter
        if (savgolWindowSize >= recidia_settings.data.savgol_filter.poly_order + 2) {
            int halfWindow = (savgolWindowSize - 1) / 2;

            vector<float> filterIn(proArray, proArray+plotsCount);
            // Pad signal at extremes
            vector<float>::const_iterator front, back;
            front = filterIn.begin() + 1;
            back = filterIn.begin() + halfWindow + 1;
            vector<float> frontPad(front, back);
            front = filterIn.end() -  halfWindow - 1;
            back = filterIn.end() - 1;
            vector<float> backPad(front, back);
            filterIn.insert(filterIn.begin(), frontPad.begin(), frontPad.end());
            filterIn.insert(filterIn.end(), backPad.begin(), backPad.end());

            // Convolve
            int const nf = pinvVector.size();
            int const ng = filterIn.size();
            vector<float> min_v = (nf < ng)? pinvVector : filterIn;
            vector<float> max_v = (nf < ng)? filterIn : pinvVector;
            int const n  = max(nf, ng) - min(nf, ng) + 1;
            vector<float> filterOut(n);
            for(auto i(0); i < n; ++i) {
                for(int j(min_v.size() - 1), k(i); j >= 0; --j) {
                    if (min_v[j] > 0 && max_v[k] > 0)
                        filterOut[i] += min_v[j] * max_v[k]; // No negative nums
                    ++k;
                }
            }
            copy(filterOut.begin(), filterOut.end(), proArray);
        }

        // Interpolation
        if (interp) {
            copy(proArray, proArray+plotsCount, interpArray[interpIndex]);

            for (j=0; j < interp; j++ ) {
                if (j != interpIndex) {
                    for (i=0; i < plotsCount; i++ ) {
                        proArray[i] += interpArray[j][i];
                    }
                }
            }
            for (i=0; i < plotsCount; i++ ) {
                proArray[i] /= interp;
            }

            interpIndex++;
            if (interpIndex >= interp) {
                interpIndex = 0;
            }
        }

        // Send out plots
        for (i=0; i < plotsCount; i++ ) {
            recidia_data.plots[i] = proArray[i];
        }

        // Sleep for poll time
        uint latency = utime_now() - timerStart;
        int sleepTime = ((recidia_settings.data.poll_rate * 1000) - latency);
        if (sleepTime > 0)
            usleep(sleepTime);
    }
}
