/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifdef STS_COMPILE_UNIT_TESTS

#include "test.h"
#include "symtseries.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *test_get_symbol_zero() {
    for (size_t pow = 1; pow <= MAX_CORDINALITY; ++pow) {
        unsigned int zero_encoded = get_symbol(0.0, pow);
        mu_assert(zero_encoded == (1 << (pow-1)), 
                "zero encoded into %u for cardinality %zu", zero_encoded, pow);
    }
    return NULL;
}

static char *test_get_symbol_breaks() {
    double breaks[8] = {1.16, 1.15, 0.67, 0.32, 0, -0.32, -0.67, -1.15};
    for (unsigned int i = 0; i < 8; ++i) {
        unsigned int break_encoded = get_symbol(breaks[i], 3);
        mu_assert(break_encoded == i, "%lf encoded into %u instead of %u", 
                breaks[i], break_encoded, i);
    }
    return NULL;
}

static char *test_to_iSAX_normalization() {
    double seq[16] = {-4, -3, -2, -1, 0, 1, 2, 3, -4, -3, -2, -1, 0, 1, 2, 3};
    double *normseq = normalize(seq, 16);
    for (size_t pow = 1; pow <= MAX_CORDINALITY; ++pow) {
        for (size_t w = 1; w <= 16; w *= 2) {
            unsigned int *sax = to_iSAX(seq, 16, w, pow), 
                         *normsax = to_iSAX(normseq, 16, w, pow);
            mu_assert(memcmp(sax, normsax, w) == 0, 
                    "normalized array got encoded differently for w=%zu, c=%zu", w, pow);
        }
    }
    return NULL;
}

static char *test_to_iSAX_sample() {
    // After averaging and normalization this series looks like:
    // {highest sector, lowest sector, sector right above 0, sector right under 0}
    double nseq[12] = {5, 6, 7, -5, -6, -7, 0.25, 0.17, 0.04, -0.04, -0.17, -0.25};
    unsigned int expected[4] = {0, 7, 3, 4};
    unsigned int *sax = to_iSAX(nseq, 12, 4, 3);
    for (int i = 0; i < 4; ++i) {
        mu_assert(sax[i] == expected[i], 
                "Error converting sample series: \
                batch %d turned into %u instead of %u", i, sax[i], expected[i]);
    }
    return NULL;
}

static char *test_to_iSAX_stationary() {
    double seq[8] = {8 + STAT_EPS, 8 - STAT_EPS, 8, 8, 8, 8 + STAT_EPS, 8, 8};
    for (size_t pow = 1; pow <= MAX_CORDINALITY; ++pow) {
        for (size_t w = 1; w <= 8; w *= 2) {
            unsigned int *sax = to_iSAX(seq, 8, w, pow);
            for (size_t i = 0; i < w; ++i) {
                mu_assert(sax[i] == (1 << (pow-1)), 
                        "#%zu element of stationary sequence encoded into %u", i, sax[i]);
            }
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

#endif
