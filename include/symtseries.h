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

typedef int bool;
#define false 0
#define true 1

#define STS_MAX_CARDINALITY 16
#define STS_STAT_EPS 1e-2

typedef unsigned char sts_symbol;

typedef struct sts_word {
    size_t n_values;
    size_t w;
    size_t c; // TODO: migrate to multi-cardinal words (for indexing)
    sts_symbol *symbols;
} *sts_word;

struct sts_ring_buffer {
    size_t cnt;
    double *buffer, *buffer_end;
    double *head, *tail;
};

typedef struct sts_window {
    struct sts_ring_buffer* values;
    struct sts_word current_word;
    double *norm_buffer;
} *sts_window;

/*
 * Initializes empty window-like-container
 * @param n: size of the window
 * @param c: code's cardinality
 * @param w: length of the produced code, should be divisor of n
 * @returns NULL on failure or allocated window
 */
sts_window sts_new_window(size_t n, size_t w, unsigned int c);

/*
 * Appends new value to the end of the window
 * If window->n_values == window->values->cnt drops the head value
 * Re-computes symbols in accordance with window->c and window->w
 * @param word: window to be updated
 * @param value: value to be appended
 * @returns pointer to updated window->current_word if there are enough values 
 * to construct a word, NULL otherwise. sts_dup_word to store it
 */
const struct sts_word* sts_append_value(sts_window window, double value);

/*
 * Appends provided array. Only the last word is stored in window->current_word.
 * @returns pointer to updated window->current_word if there are enough values
 * to construct a word, NULL otherwise. sts_dup_word to store it
 */
const struct sts_word* sts_append_array(sts_window window, const double *values, size_t n_values);

/*
 * Returns symbolic representation of series which doesn't store initial values
 * @param n_values: number of elements in series
 * @param c: code's cardinality
 * @param w: length of returned code, should be divisor of n
 * @returns NULL on failure or freshly-alocated sts_word
 */
sts_word sts_from_double_array(const double *series, size_t n_values, size_t w, unsigned int c);

/*
 * Constructs word from symbolic representation, e.g. "AABBC"
 * @param symbols: symbolic representation in SAX notation
 * @param c: cardinality of the word
 * @returns NULL on failure (illegal symbols for cardinality or unprocessable cardinality itself)
 * or freshly-allocated sts_word with sts_word.w == strlen(symbols)
 */
sts_word sts_from_sax_string(const char *symbols, size_t c);

/*
 * @param a: word
 * @returns NULL on failure (illegal symbols for cardinality) or SAX string corresponding to a
 */
char *sts_word_to_sax_string(const struct sts_word *a);

/*
 * Returns the lowerbounding approximation on distance 
 * between sax-represented series a and b.
 * One of the words can have sts_word->n_values == 0 and 
 * method will use other's word n_values for mindist estimation.
 * @param a, b: sax representations of sequences
 * @returns NaN on failure, otherwise minimum possible distance between original series
 */
double sts_mindist(const struct sts_word* a, const struct sts_word* b);

/*
 * Frees allocated memory for sax representation
 * @param a: pre-allocated word which contents should be freed
 */
void sts_free_word(sts_word a);

/*
 * Frees allocated window
 * @param w: pre-allocated window
 */
void sts_free_window(sts_window w);

/*
 * Resets a->values->cnt to zero and adjusts ring buffer accordingly
 * @param w: window to be reset to zero size
 * @returns true in case of successfull reset and false if the window was malformed
 */
bool sts_reset_window(sts_window w);

/*
 * @param a: word to be copied
 * @returns freshly-allocated copy of the provided word or NULL on failure
 */
sts_word sts_dup_word(const struct sts_word* a);

/*
 * @param a: window
 * @returns whether there is enough values stored in window to construct current word
 */
bool sts_window_is_ready(const struct sts_window *a);

#endif
