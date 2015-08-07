/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "symtseries.h"
#include <stdio.h>
#include <string.h>

char *strdup (const char *s) {
    char *d = malloc (strlen (s) + 1);
    if (d != NULL) strcpy (d,s);
    return d;
}

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

void show_iSAX(sax_word word, size_t w, unsigned int c) {
    printf("{");
    for (size_t i = 0; i < w; ++i) {
        fflush(stdout);
        char buffer[19];
        to_binary(word.symbols[i], buffer, c);
        printf("%s", buffer);
        if (i < w - 1) printf(", ");
    }
    printf("}\n");
}

void show_conv_iSAX(double *series, size_t n_values, size_t w, unsigned int c) {
    sax_word word = sts_to_sax(series, n_values, w, c);
    if (word.symbols == NULL) {
        printf("OOps, yet unable to manage given params\n");
        return;
    }
    show_iSAX(word, w, c);
    sts_free_word(word);
}
