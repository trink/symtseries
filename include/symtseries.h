/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* This implementation is based on several SAX papers, 
 * the latest of which can be found here:
 * http://www.cs.ucr.edu/~eamonn/iSAX_2.0.pdf */

#ifndef _SYMTSERIES_H_
#define _SYMTSERIES_H_
#include <float.h>
#include <stdlib.h>

#define STS_MAX_CARDINALITY 16
#define STS_STAT_EPS 1e-2

typedef unsigned char sax_symbol;

struct ring_buffer;

typedef struct ring_buffer ring_buffer;

typedef struct sax_word {
    size_t n_values;
    size_t w;
    size_t c; // TODO: migrate to multi-cardinal words (for indexing)
    sax_symbol *symbols;
    struct ring_buffer* values;
} sax_word;

/*
 * Initializes empty sliding-window-like-container
 * Any operations on the word will fail until
 * at least n values are provided by append_value(s)
 * @param n: size of the window
 * @param c: code's cardinality
 * @param w: length of the produced code, should be divisor of n
 * @returns {0, 0, 0, NULL, NULL} on failure
 */
sax_word sts_new_sliding_word(size_t n, size_t w, unsigned int c);

/*
 * Appends new value to the end of given sax-word
 * If word.n_values == word.values->cnt drops the head value
 * Re-computes symbols in accordance with word.c and word.w
 * @returns 0 on failure, 1 otherwise
 */
int sts_append_value(sax_word *word, double value);

/*
 * Returns symbolic representation of series which doesn't store initial values
 * @param n_values: number of elements in series
 * @param c: code's cardinality
 * @param w: length of returned code, should be divisor of n
 * @returns {0, 0, 0, NULL, NULL} on failure
 */
sax_word sts_to_sax(double *series, size_t n_values, size_t w, unsigned int c);

/*
 * Returns the lowerbounding approximation on distance 
 * between sax-represented series a and b.
 * @param a, b: sax representations of sequences
 * @returns NaN on failure, otherwise minimum possible distance between original series
 */
double sts_mindist(sax_word a, sax_word b, size_t n, size_t w, unsigned int c);

/*
 * Frees allocated memory for sax representation
 */
void sts_free_word(sax_word a);

#endif
