#ifndef _SYMTSERIES_H_
#define _SYMTSERIES_H_
#include <float.h>
#include <stdlib.h>

/* TODO: get rid of it! (dynamic breakpoint estimation + big integers) */
#define MAX_CORDINALITY 3
/* Breakpoints used in iSAX symbol estimation */
extern const double breaks[MAX_CORDINALITY][(1 << MAX_CORDINALITY) + 1];

/*
 * Returns the coresponding iSAX binary symbol 
 * (as in the original paper: (b_c, INF] corresponding to 0 and so forth)
 */
unsigned int get_symbol(double value, int cardinality);

/*
 * Returns symbolic representation of series in binary notation
 * @param c: 2^c would be code's cardinality
 * @param w: length of returned code
 */
unsigned int *to_iSAX(double *series, size_t n_values, int w, int c);

double *normalize(double *series, size_t n_values);
#endif
