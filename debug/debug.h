/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "symtseries.h"
#include <stdio.h>

void to_binary(unsigned int n, char *buffer, int c) {
    int i = 0;
    while (n) {
        buffer[i++] = '0' + (n & 1);
        n /= 2;
    }
    while (i < c) {
        buffer[i++] = '0';
    }
    for (int j = 0; j < i / 2; ++j) {
        int tmp = buffer[i-j-1];
        buffer[i-j-1] = buffer[j];
        buffer[j] = tmp;
    }
    buffer[i] = '\0';
}

void show_iSAX(double *series, size_t n_values, int w, int c) {
    sax_symbol *iSAX_word = sts_to_iSAX(series, n_values, w, c);
    if (iSAX_word == NULL) {
        printf("OOps, yet unable to manage given params\n");
        return;
    }
    printf("{");
    for (int i = 0; i < w; ++i) {
        char buffer[19];
        to_binary(iSAX_word[i], buffer, c);
        printf("%s", buffer);
        if (i < w - 1) printf(", ");
    }
    printf("}\n");
    free(iSAX_word);
}
