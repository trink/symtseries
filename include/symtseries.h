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

/* TODO: get rid of it? (dynamic breakpoint estimation + big integers) */
#define STS_MAX_CARDINALITY 16
#define STS_STAT_EPS 1e-2

typedef unsigned char sax_symbol;
typedef sax_symbol *sax_word;

/*
 * Returns symbolic representation of series in binary notation
 * @param c: 2^c would be code's cardinality
 * @param w: length of returned code
 * TODO: support cardinalities which are not powers of 2
 */
sax_word sts_to_iSAX(double *series, size_t n_values, size_t w, unsigned int c);

/*
 * Returns the lowerbounding approximation on distance 
 * between sax-represented series a and b.
 * @param a, b: sax representations of sequences
 */
double sts_mindist(sax_word a, sax_word b, size_t n, size_t w, unsigned int c);

#endif
