/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* This implementation is based on several SAX papers, 
 * the latest of which can be found here:
 * http://www.cs.ucr.edu/~eamonn/iSAX_2.0.pdf */

#include "symtseries.h"

#include <float.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/* Breakpoints used in iSAX symbol estimation */
const double breaks[STS_MAX_CORDINALITY][(1 << STS_MAX_CORDINALITY) + 1] = 
   {{-DBL_MAX, 0.0, DBL_MAX, 0, 0, 0, 0, 0, 0},
    {-DBL_MAX, -0.67, 0.0, 0.67, DBL_MAX, 0, 0, 0, 0},
    {-DBL_MAX, -1.15, -0.67, -0.32, 0.0, 0.32, 0.67, 1.15, DBL_MAX}};

sax_symbol get_symbol(double value, unsigned int c) {
    unsigned int pow = 1 << c;
    for (unsigned int i = 0; i < pow; ++i) {
        if (value >= breaks[c-1][i] 
            &&
            value < breaks[c-1][i+1]) {
            return pow - i - 1;
        }
    }
    return 0;
}

double *normalize(double *series, size_t n_values) {
    double mu = 0, std = 0;
    for (size_t i = 0; i < n_values; ++i) {
        mu += series[i];
    }
    mu /= n_values;
    for (size_t i = 0; i < n_values; ++i) {
        std += (mu - series[i]) * (mu - series[i]);
    }
    std /= n_values;
    std = sqrt(std);
    double *normalized = malloc(n_values * sizeof(double));
    if (std < STS_STAT_EPS) {
        // to prevent infinite-scaling for almost-stationary sequencies
        memset(normalized, 0, n_values * sizeof(double));
    } else {
        for (size_t i = 0; i < n_values; ++i) {
            normalized[i] = (series[i] - mu) / std;
        }
    }
    return normalized;
}

sax_word sts_to_iSAX(double *series, size_t n_values, int w, int c) {
    if (n_values % w != 0 || c > STS_MAX_CORDINALITY) {
        // TODO: Not supported yet, fix
        return NULL;
    }
    series = normalize(series, n_values);
    sax_word encoded_series = (sax_word) malloc(w * sizeof(sax_symbol));
    unsigned int frame_size = n_values / w;
    for (int i = 0; i < w; ++i) {
        double average = 0;
        for (size_t j = i * frame_size; j < (i+1) * frame_size; ++j) {
            average += series[j];
        } 
        average /= frame_size;
        encoded_series[i] = get_symbol(average, c);
    }
    free(series);
    return encoded_series;
}
