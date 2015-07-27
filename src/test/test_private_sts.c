/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifdef STS_COMPILE_UNIT_TESTS

#include "sts_test.h"
#include "symtseries.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
