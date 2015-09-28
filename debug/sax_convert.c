/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "debug.h"
#include <stdio.h>

#ifdef _MSC_VER
#pragma warning( push )
// To silence unsafe warning
#pragma warning( disable : 4996 )
#endif

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Provide me with cardinality of the code\n");
        exit(-1);
    }
    int c = atoi(argv[1]);
    size_t w = (argc > 2) ? atoi(argv[2]) : 0;

    double *values = malloc(sizeof(double));
    size_t n_values = 0;
    while (scanf("%lf", &values[n_values]) != EOF) {
        values = realloc(values, (++n_values + 1) * sizeof(double));
    }
    if (w != 0) {
        show_conv_iSAX(values, n_values, w, c);
    } else {
        for (w = 1; w <= n_values; ++w) {
            if (n_values % w == 0)
                show_conv_iSAX(values, n_values, w, c);
        }
    }
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif
