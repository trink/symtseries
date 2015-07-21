/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "symtseries.h"

#include <float.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define STAT_EPS 1e-2

const double breaks[MAX_CORDINALITY][(1 << MAX_CORDINALITY) + 1] = 
   {{-DBL_MAX, 0.0, DBL_MAX, 0, 0, 0, 0, 0, 0},
    {-DBL_MAX, -0.67, 0.0, 0.67, DBL_MAX, 0, 0, 0, 0},
    {-DBL_MAX, -1.15, -0.67, -0.32, 0.0, 0.32, 0.67, 1.15, DBL_MAX}};

unsigned int get_symbol(double value, int cardinality) {
    unsigned int pow = 1 << cardinality;
    for (unsigned int i = 0; i < pow; ++i) {
        if (value >= breaks[cardinality-1][i] 
            &&
            value <= breaks[cardinality-1][i+1]) {
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
    if (std < STAT_EPS) {
        // to prevent infinite-scaling for almost-stationary sequencies
        memset(normalized, 0, n_values * sizeof(double));
    } else {
        for (size_t i = 0; i < n_values; ++i) {
            normalized[i] = (series[i] - mu) / std;
        }
    }
    return normalized;
}

unsigned int *to_iSAX(double *series, size_t n_values, int w, int c) {
    if (n_values % w != 0 || c > MAX_CORDINALITY) {
        // TODO: Not supported yet, fix
        return NULL;
    }
    series = normalize(series, n_values);
    unsigned int *encoded_series = (unsigned int *) malloc(w * sizeof(unsigned int));
    unsigned int chunk_size = n_values / w;
    for (int i = 0; i < w; ++i) {
        double average = 0;
        for (size_t j = i * chunk_size; j < (i+1) * chunk_size; ++j) {
            average += series[j];
        } 
        average /= chunk_size;
        encoded_series[i] = get_symbol(average, c);
    }
    free(series);
    return encoded_series;
}
