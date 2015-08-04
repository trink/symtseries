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
static const double breaks[STS_MAX_CARDINALITY - 1][STS_MAX_CARDINALITY + 1] = 
{
    {
        -INFINITY,
        0.0,
        INFINITY,
    },

    {
        -INFINITY,
        -0.430,
        0.430,
        INFINITY,
    },

    {
        -INFINITY,
        -0.674,
        0.0,
        0.674,
        INFINITY,
    },

    {
        -INFINITY,
        -0.841,
        -0.253,
        0.253,
        0.841,
        INFINITY,
    },

    {
        -INFINITY,
        -0.967,
        -0.430,
        0.0,
        0.430,
        0.967,
        INFINITY,
    },

    {
        -INFINITY,
        -1.067,
        -0.565,
        -0.180,
        0.180,
        0.565,
        1.067,
        INFINITY,
    },

    {
        -INFINITY,
        -1.150,
        -0.674,
        -0.318,
        0.0,
        0.318,
        0.674,
        1.150,
        INFINITY,
    },

    {
        -INFINITY,
        -1.220,
        -0.764,
        -0.430,
        -0.139,
        0.139,
        0.430,
        0.764,
        1.220,
        INFINITY,
    },

    {
        -INFINITY,
        -1.281,
        -0.841,
        -0.524,
        -0.253,
        0.0,
        0.253,
        0.524,
        0.841,
        1.281,
        INFINITY,
    },

    {
        -INFINITY,
        -1.335,
        -0.908,
        -0.604,
        -0.348,
        -0.114,
        0.114,
        0.348,
        0.604,
        0.908,
        1.335,
        INFINITY,
    },

    {
        -INFINITY,
        -1.382,
        -0.967,
        -0.674,
        -0.430,
        -0.210,
        0.0,
        0.210,
        0.430,
        0.674,
        0.967,
        1.382,
        INFINITY,
    },

    {
        -INFINITY,
        -1.426,
        -1.020,
        -0.736,
        -0.502,
        -0.293,
        -0.096,
        0.096,
        0.293,
        0.502,
        0.736,
        1.020,
        1.426,
        INFINITY,
    },

    {
        -INFINITY,
        -1.465,
        -1.067,
        -0.791,
        -0.565,
        -0.366,
        -0.180,
        0.0,
        0.180,
        0.366,
        0.565,
        0.791,
        1.067,
        1.465,
        INFINITY,
    },

    {
        -INFINITY,
        -1.501,
        -1.110,
        -0.841,
        -0.622,
        -0.430,
        -0.253,
        -0.083,
        0.083,
        0.253,
        0.430,
        0.622,
        0.841,
        1.110,
        1.501,
        INFINITY,
    },

    {
        -INFINITY,
        -1.534,
        -1.150,
        -0.887,
        -0.674,
        -0.488,
        -0.318,
        -0.157,
        0.0,
        0.157,
        0.318,
        0.488,
        0.674,
        0.887,
        1.150,
        1.534,
        INFINITY
    }
};

static sax_symbol get_symbol(double value, unsigned int c) {
    for (unsigned int i = 0; i < c; ++i) {
        if (value >= breaks[c-2][i]
            &&
            value < breaks[c-2][i+1]) {
            return c - i - 1;
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
    if (n_values % w != 0 || c > STS_MAX_CARDINALITY || c < 2) {
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
    if (c > STS_MAX_CARDINALITY || c < 2) {
        return INFINITY;
    }
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

static char *test_get_symbol_zero() {
    for (size_t c = 2; c <= STS_MAX_CARDINALITY; ++c) {
        sax_symbol zero_encoded = get_symbol(0.0, c);
        mu_assert(zero_encoded == (c / 2) - 1 + (c % 2),
                "zero encoded into %u for cardinality %zu", zero_encoded, c);
    }
    return NULL;
}

static char *test_get_symbol_breaks() {
    for (size_t c = 2; c <= STS_MAX_CARDINALITY; ++c) {
        for (unsigned int i = 0; i < c; ++i) {
            sax_symbol break_encoded = get_symbol(breaks[c-2][i], c);
            mu_assert(break_encoded == c - i - 1, "%lf encoded into %u instead of %zu. c == %zu", 
                    breaks[c-2][i], break_encoded, c - i - 1, c);
        }
    }
    return NULL;
}

char *test_to_iSAX_normalization() {
    double seq[16] = {-4, -3, -2, -1, 0, 1, 2, 3, -4, -3, -2, -1, 0, 1, 2, 3};
    double *normseq = normalize(seq, 16);
    for (size_t c = 2; c <= STS_MAX_CARDINALITY; ++c) {
        for (size_t w = 1; w <= 16; w *= 2) {
            sax_word sax = sts_to_iSAX(seq, 16, w, c), 
                     normsax = sts_to_iSAX(normseq, 16, w, c);
            mu_assert(sax != NULL, "sax conversion failed");
            mu_assert(normsax != NULL, "sax conversion failed");
            mu_assert(memcmp(sax, normsax, w) == 0, 
                    "normalized array got encoded differently for w=%zu, c=%zu", w, c);
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
    sax_word sax = sts_to_iSAX(nseq, 12, 4, 8);
    mu_assert(sax != NULL, "sax conversion failed");
    for (int i = 0; i < 4; ++i) {
        mu_assert(sax[i] == expected[i], 
                "Error converting sample series: \
                batch %d turned into %u instead of %u", i, sax[i], expected[i]);
    }
    free(sax);
    return NULL;
}

static char *test_to_iSAX_stationary() {
    double sseq[60] = {
        8 + STS_STAT_EPS, 8 - STS_STAT_EPS, 8, 8, 
        8, 8 + STS_STAT_EPS, 8, 8 + STS_STAT_EPS, 
        8 - STS_STAT_EPS, 8, 8 + STS_STAT_EPS, 8,
        8 + STS_STAT_EPS, 8 - STS_STAT_EPS, 8, 8, 
        8, 8 + STS_STAT_EPS, 8, 8 + STS_STAT_EPS, 
        8 - STS_STAT_EPS, 8, 8 + STS_STAT_EPS, 8,
        8 + STS_STAT_EPS, 8 - STS_STAT_EPS, 8, 8, 
        8, 8 + STS_STAT_EPS, 8, 8 + STS_STAT_EPS, 
        8 - STS_STAT_EPS, 8, 8 + STS_STAT_EPS, 8,
        8 + STS_STAT_EPS, 8 - STS_STAT_EPS, 8, 8, 
        8, 8 + STS_STAT_EPS, 8, 8 + STS_STAT_EPS, 
        8 - STS_STAT_EPS, 8, 8 + STS_STAT_EPS, 8,
        8 + STS_STAT_EPS, 8 - STS_STAT_EPS, 8, 8, 
        8, 8 + STS_STAT_EPS, 8, 8 + STS_STAT_EPS, 
        8 - STS_STAT_EPS, 8, 8 + STS_STAT_EPS, 8
    };
    for (size_t c = 2; c <= STS_MAX_CARDINALITY; ++c) {
        for (size_t w = 1; w <= 60; ++w) {
            sax_word sax = sts_to_iSAX(sseq, 60 - (60 % w), w, c);
            mu_assert(sax != NULL, "sax conversion failed");
            for (size_t i = 0; i < w; ++i) {
                mu_assert(sax[i] == (c / 2) - 1 + (c%2),
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
