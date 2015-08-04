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
static const double breaks[STS_MAX_CORDINALITY][(1 << STS_MAX_CORDINALITY) + 1] = 
   {{-DBL_MAX, 0.0, DBL_MAX, 0, 0, 0, 0, 0, 0},
    {-DBL_MAX, -0.67, 0.0, 0.67, DBL_MAX, 0, 0, 0, 0},
    {-DBL_MAX, -1.15, -0.67, -0.32, 0.0, 0.32, 0.67, 1.15, DBL_MAX}};

static sax_symbol get_symbol(double value, unsigned int c) {
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

static double *normalize(double *series, size_t n_values) {
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

sax_word sts_to_iSAX(double *series, size_t n_values, size_t w, unsigned int c) {
    if (n_values % w != 0 || c > STS_MAX_CORDINALITY || c == 0) {
        return NULL;
    }
    series = normalize(series, n_values);
    sax_word encoded_series = (sax_word) malloc(w * sizeof(sax_symbol));
    unsigned int frame_size = n_values / w;
    for (unsigned int i = 0; i < w; ++i) {
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

/* TODO: precompute dist table */
static double sym_dist(sax_symbol a, sax_symbol b, unsigned int c) {
    if (abs(a - b) <= 1) {
        return 0;
    }
    if (a > b) {
        sax_symbol c_ = a;
        a = b;
        b = c_;
    }
    return breaks[c-1][a+1] - breaks[c-1][b];
}

double sts_mindist(sax_word a, sax_word b, size_t n, size_t w, unsigned int c) {
    double distance = 0, sym_distance;
    for (size_t i = 0; i < w; ++i) {
        sym_distance = sym_dist(a[i], b[i], c);
        distance += sym_distance * sym_distance;
    }
    distance = sqrt((double) n / (double) w) * sqrt(distance);
    return distance;
}

/* No namespaces in C, so it goes here */
#ifdef STS_COMPILE_UNIT_TESTS

#include "test/sts_test.h"
#include <errno.h>
#include <stdio.h>

const double tbreaks[8] = {1.15, 0.67, 0.32, 0, -0.32, -0.67, -1.15, -1.16};

static char *test_get_symbol_zero() {
    for (size_t pow = 1; pow <= STS_MAX_CORDINALITY; ++pow) {
        sax_symbol zero_encoded = get_symbol(0.0, pow);
        mu_assert(zero_encoded == (1 << (pow-1)) - 1, 
                "zero encoded into %u for cardinality %zu", zero_encoded, pow);
    }
    return NULL;
}

static char *test_get_symbol_breaks() {
    for (unsigned int i = 0; i < 8; ++i) {
        sax_symbol break_encoded = get_symbol(tbreaks[i], 3);
        mu_assert(break_encoded == i, "%lf encoded into %u instead of %u", 
                tbreaks[i], break_encoded, i);
    }
    return NULL;
}

char *test_to_iSAX_normalization() {
    double seq[16] = {-4, -3, -2, -1, 0, 1, 2, 3, -4, -3, -2, -1, 0, 1, 2, 3};
    double *normseq = normalize(seq, 16);
    for (size_t pow = 1; pow <= STS_MAX_CORDINALITY; ++pow) {
        for (size_t w = 1; w <= 16; w *= 2) {
            sax_word sax = sts_to_iSAX(seq, 16, w, pow), 
                     normsax = sts_to_iSAX(normseq, 16, w, pow);
            mu_assert(memcmp(sax, normsax, w) == 0, 
                    "normalized array got encoded differently for w=%zu, c=%zu", w, pow);
            free(sax);
            free(normsax);
        }
    }
    free(normseq);
    return NULL;
}

static char *test_to_iSAX_sample() {
    // After averaging and normalization this series looks like:
    // {highest sector, lowest sector, sector right above 0, sector right under 0}
    double nseq[12] = {5, 6, 7, -5, -6, -7, 0.25, 0.17, 0.04, -0.04, -0.17, -0.25};
    unsigned int expected[4] = {0, 7, 3, 4};
    sax_word sax = sts_to_iSAX(nseq, 12, 4, 3);
    for (int i = 0; i < 4; ++i) {
        mu_assert(sax[i] == expected[i], 
                "Error converting sample series: \
                batch %d turned into %u instead of %u", i, sax[i], expected[i]);
    }
    free(sax);
    return NULL;
}

static char *test_to_iSAX_stationary() {
    double sseq[8] = {8 + STS_STAT_EPS, 8 - STS_STAT_EPS, 8, 8, 8, 8 + STS_STAT_EPS, 8, 8};
    for (size_t pow = 1; pow <= STS_MAX_CORDINALITY; ++pow) {
        for (size_t w = 1; w <= 8; w *= 2) {
            sax_word sax = sts_to_iSAX(sseq, 8, w, pow);
            for (size_t i = 0; i < w; ++i) {
                mu_assert(sax[i] == (1 << (pow-1)) - 1, 
                        "#%zu element of stationary sequence encoded into %u", i, sax[i]);
            }
            free(sax);
        }
    }
    return NULL;
}

static char* all_tests() {
    mu_run_test(test_get_symbol_zero);
    mu_run_test(test_get_symbol_breaks);
    mu_run_test(test_to_iSAX_normalization);
    mu_run_test(test_to_iSAX_sample);
    mu_run_test(test_to_iSAX_stationary);
    return NULL;
}

int main() {
    char* result = all_tests();
    if (result) {
        printf("%s\n", result);
    } else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", mu_tests_run);

    return result != 0;
}

#endif // STS_COMPILE_UNIT_TESTS
