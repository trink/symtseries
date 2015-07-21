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

/* TODO: get rid of it! (dynamic breakpoint estimation + big integers) */
#define MAX_CORDINALITY 3
#define STAT_EPS 1e-2

#ifdef STS_COMPILE_UNIT_TESTS
/*
 * Returns the coresponding iSAX binary symbol 
 * (as in the original paper: (b_c, INF] corresponding to 0 and so forth)
 */
unsigned int get_symbol(double value, int cardinality);

double *normalize(double *series, size_t n_values);
#endif // STS_COMPILE_UNIT_TESTS

/*
 * Returns symbolic representation of series in binary notation
 * @param c: 2^c would be code's cardinality
 * @param w: length of returned code
 */
unsigned int *to_iSAX(double *series, size_t n_values, int w, int c);

#endif
