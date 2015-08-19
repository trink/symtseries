/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* This implementation is based on several SAX papers, 
 * the latest of which can be found here:
 * http://www.cs.ucr.edu/~eamonn/iSAX_2.0.pdf */

#include "symtseries.h"

#include <float.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

/* Breakpoints used in iSAX symbol estimation */
static const double breaks[STS_MAX_CARDINALITY - 1][STS_MAX_CARDINALITY + 1] = 
{
    { -INFINITY, 0.0, INFINITY, },
    { -INFINITY, -0.430, 0.430, INFINITY, },
    { -INFINITY, -0.674, 0.0, 0.674, INFINITY, },
    { -INFINITY, -0.841, -0.253, 0.253, 0.841, INFINITY, },
    { -INFINITY, -0.967, -0.430, 0.0, 0.430, 0.967, INFINITY, },
    { -INFINITY, -1.067, -0.565, -0.180, 0.180, 0.565, 1.067, INFINITY, },
    { -INFINITY, -1.150, -0.674, -0.318, 0.0, 0.318, 0.674, 1.150, INFINITY, },
    { -INFINITY, -1.220, -0.764, -0.430, -0.139, 0.139, 0.430, 0.764, 1.220, INFINITY, },
    { -INFINITY, -1.281, -0.841, -0.524, -0.253, 0.0, 0.253, 0.524, 0.841, 1.281, INFINITY, },
    { -INFINITY, -1.335, -0.908, -0.604, -0.348, -0.114, 0.114, 0.348, 0.604, 0.908, 1.335, 
      INFINITY, },
    { -INFINITY, -1.382, -0.967, -0.674, -0.430, -0.210, 0.0, 0.210, 0.430, 0.674, 0.967, 
        1.382, INFINITY, },
    { -INFINITY, -1.426, -1.020, -0.736, -0.502, -0.293, -0.096, 0.096, 0.293, 0.502, 0.736, 
        1.020, 1.426, INFINITY, },
    { -INFINITY, -1.465, -1.067, -0.791, -0.565, -0.366, -0.180, 0.0, 0.180, 0.366, 0.565, 
        0.791, 1.067, 1.465, INFINITY, },
    { -INFINITY, -1.501, -1.110, -0.841, -0.622, -0.430, -0.253, -0.083, 0.083, 0.253, 0.430, 
        0.622, 0.841, 1.110, 1.501, INFINITY, },
    { -INFINITY, -1.534, -1.150, -0.887, -0.674, -0.488, -0.318, -0.157, 0.0, 0.157, 0.318, 
        0.488, 0.674, 0.887, 1.150, 1.534, INFINITY }
};

static const double dist_table[STS_MAX_CARDINALITY - 1][STS_MAX_CARDINALITY][STS_MAX_CARDINALITY] = 
{
    {
        { 0,0, },
        { 0,0, },
    },
    {
        { 0,0,0.86, },
        { 0,0,0, },
        { 0.86,0,0, },
    },
    {
        { 0,0,0.67,1.35, },
        { 0,0,0,0.67, },
        { 0.67,0,0,0, },
        { 1.35,0.67,0,0, },
    },
    {
        { 0,0,0.59,1.09,1.68, },
        { 0,0,0,0.51,1.09, },
        { 0.59,0,0,0,0.59, },
        { 1.09,0.51,0,0,0, },
        { 1.68,1.09,0.59,0,0, },
    },
    {
        { 0,0,0.54,0.97,1.40,1.93, },
        { 0,0,0,0.43,0.86,1.40, },
        { 0.54,0,0,0,0.43,0.97, },
        { 0.97,0.43,0,0,0,0.54, },
        { 1.40,0.86,0.43,0,0,0, },
        { 1.93,1.40,0.97,0.54,0,0, },
    },
    {
        { 0,0,0.50,0.89,1.25,1.63,2.14, },
        { 0,0,0,0.39,0.75,1.13,1.63, },
        { 0.50,0,0,0,0.36,0.75,1.25, },
        { 0.89,0.39,0,0,0,0.39,0.89, },
        { 1.25,0.75,0.36,0,0,0,0.50, },
        { 1.63,1.13,0.75,0.39,0,0,0, },
        { 2.14,1.63,1.25,0.89,0.50,0,0, },
    },
    {
        { 0,0,0.48,0.83,1.15,1.47,1.82,2.30, },
        { 0,0,0,0.36,0.67,0.99,1.35,1.82, },
        { 0.48,0,0,0,0.32,0.64,0.99,1.47, },
        { 0.83,0.36,0,0,0,0.32,0.67,1.15, },
        { 1.15,0.67,0.32,0,0,0,0.36,0.83, },
        { 1.47,0.99,0.64,0.32,0,0,0,0.48, },
        { 1.82,1.35,0.99,0.67,0.36,0,0,0, },
        { 2.30,1.82,1.47,1.15,0.83,0.48,0,0, },
    },
    {
        { 0,0,0.46,0.79,1.08,1.36,1.65,1.99,2.44, },
        { 0,0,0,0.33,0.62,0.90,1.20,1.53,1.99, },
        { 0.46,0,0,0,0.29,0.57,0.86,1.20,1.65, },
        { 0.79,0.33,0,0,0,0.28,0.57,0.90,1.36, },
        { 1.08,0.62,0.29,0,0,0,0.29,0.62,1.08, },
        { 1.36,0.90,0.57,0.28,0,0,0,0.33,0.79, },
        { 1.65,1.20,0.86,0.57,0.29,0,0,0,0.46, },
        { 1.99,1.53,1.20,0.90,0.62,0.33,0,0,0, },
        { 2.44,1.99,1.65,1.36,1.08,0.79,0.46,0,0, },
    },
    {
        { 0,0,0.44,0.76,1.03,1.28,1.53,1.81,2.12,2.56, },
        { 0,0,0,0.32,0.59,0.84,1.09,1.37,1.68,2.12, },
        { 0.44,0,0,0,0.27,0.52,0.78,1.05,1.37,1.81, },
        { 0.76,0.32,0,0,0,0.25,0.51,0.78,1.09,1.53, },
        { 1.03,0.59,0.27,0,0,0,0.25,0.52,0.84,1.28, },
        { 1.28,0.84,0.52,0.25,0,0,0,0.27,0.59,1.03, },
        { 1.53,1.09,0.78,0.51,0.25,0,0,0,0.32,0.76, },
        { 1.81,1.37,1.05,0.78,0.52,0.27,0,0,0,0.44, },
        { 2.12,1.68,1.37,1.09,0.84,0.59,0.32,0,0,0, },
        { 2.56,2.12,1.81,1.53,1.28,1.03,0.76,0.44,0,0, },
    },
    {
        { 0,0,0.43,0.73,0.99,1.22,1.45,1.68,1.94,2.24,2.67, },
        { 0,0,0,0.30,0.56,0.79,1.02,1.26,1.51,1.82,2.24, },
        { 0.43,0,0,0,0.26,0.49,0.72,0.95,1.21,1.51,1.94, },
        { 0.73,0.30,0,0,0,0.23,0.46,0.70,0.95,1.26,1.68, },
        { 0.99,0.56,0.26,0,0,0,0.23,0.46,0.72,1.02,1.45, },
        { 1.22,0.79,0.49,0.23,0,0,0,0.23,0.49,0.79,1.22, },
        { 1.45,1.02,0.72,0.46,0.23,0,0,0,0.26,0.56,0.99, },
        { 1.68,1.26,0.95,0.70,0.46,0.23,0,0,0,0.30,0.73, },
        { 1.94,1.51,1.21,0.95,0.72,0.49,0.26,0,0,0,0.43, },
        { 2.24,1.82,1.51,1.26,1.02,0.79,0.56,0.30,0,0,0, },
        { 2.67,2.24,1.94,1.68,1.45,1.22,0.99,0.73,0.43,0,0, },
    },
    {
        { 0,0,0.42,0.71,0.95,1.17,1.38,1.59,1.81,2.06,2.35,2.77, },
        { 0,0,0,0.29,0.54,0.76,0.97,1.18,1.40,1.64,1.93,2.35, },
        { 0.42,0,0,0,0.24,0.46,0.67,0.88,1.11,1.35,1.64,2.06, },
        { 0.71,0.29,0,0,0,0.22,0.43,0.64,0.86,1.11,1.40,1.81, },
        { 0.95,0.54,0.24,0,0,0,0.21,0.42,0.64,0.88,1.18,1.59, },
        { 1.17,0.76,0.46,0.22,0,0,0,0.21,0.43,0.67,0.97,1.38, },
        { 1.38,0.97,0.67,0.43,0.21,0,0,0,0.22,0.46,0.76,1.17, },
        { 1.59,1.18,0.88,0.64,0.42,0.21,0,0,0,0.24,0.54,0.95, },
        { 1.81,1.40,1.11,0.86,0.64,0.43,0.22,0,0,0,0.29,0.71, },
        { 2.06,1.64,1.35,1.11,0.88,0.67,0.46,0.24,0,0,0,0.42, },
        { 2.35,1.93,1.64,1.40,1.18,0.97,0.76,0.54,0.29,0,0,0, },
        { 2.77,2.35,2.06,1.81,1.59,1.38,1.17,0.95,0.71,0.42,0,0, },
    },
    {
        { 0,0,0.41,0.69,0.92,1.13,1.33,1.52,1.72,1.93,2.16,2.45,2.85, },
        { 0,0,0,0.28,0.52,0.73,0.92,1.12,1.31,1.52,1.76,2.04,2.45, },
        { 0.41,0,0,0,0.23,0.44,0.64,0.83,1.03,1.24,1.47,1.76,2.16, },
        { 0.69,0.28,0,0,0,0.21,0.41,0.60,0.80,1.00,1.24,1.52,1.93, },
        { 0.92,0.52,0.23,0,0,0,0.20,0.39,0.59,0.80,1.03,1.31,1.72, },
        { 1.13,0.73,0.44,0.21,0,0,0,0.19,0.39,0.60,0.83,1.12,1.52, },
        { 1.33,0.92,0.64,0.41,0.20,0,0,0,0.20,0.41,0.64,0.92,1.33, },
        { 1.52,1.12,0.83,0.60,0.39,0.19,0,0,0,0.21,0.44,0.73,1.13, },
        { 1.72,1.31,1.03,0.80,0.59,0.39,0.20,0,0,0,0.23,0.52,0.92, },
        { 1.93,1.52,1.24,1.00,0.80,0.60,0.41,0.21,0,0,0,0.28,0.69, },
        { 2.16,1.76,1.47,1.24,1.03,0.83,0.64,0.44,0.23,0,0,0,0.41, },
        { 2.45,2.04,1.76,1.52,1.31,1.12,0.92,0.73,0.52,0.28,0,0,0, },
        { 2.85,2.45,2.16,1.93,1.72,1.52,1.33,1.13,0.92,0.69,0.41,0,0, },
    },
    {
        { 0,0,0.40,0.67,0.90,1.10,1.29,1.47,1.65,1.83,2.03,2.26,2.53,2.93, },
        { 0,0,0,0.28,0.50,0.70,0.89,1.07,1.25,1.43,1.63,1.86,2.14,2.53, },
        { 0.40,0,0,0,0.23,0.43,0.61,0.79,0.97,1.16,1.36,1.58,1.86,2.26, },
        { 0.67,0.28,0,0,0,0.20,0.39,0.57,0.75,0.93,1.13,1.36,1.63,2.03, },
        { 0.90,0.50,0.23,0,0,0,0.19,0.37,0.55,0.73,0.93,1.16,1.43,1.83, },
        { 1.10,0.70,0.43,0.20,0,0,0,0.18,0.36,0.55,0.75,0.97,1.25,1.65, },
        { 1.29,0.89,0.61,0.39,0.19,0,0,0,0.18,0.37,0.57,0.79,1.07,1.47, },
        { 1.47,1.07,0.79,0.57,0.37,0.18,0,0,0,0.19,0.39,0.61,0.89,1.29, },
        { 1.65,1.25,0.97,0.75,0.55,0.36,0.18,0,0,0,0.20,0.43,0.70,1.10, },
        { 1.83,1.43,1.16,0.93,0.73,0.55,0.37,0.19,0,0,0,0.23,0.50,0.90, },
        { 2.03,1.63,1.36,1.13,0.93,0.75,0.57,0.39,0.20,0,0,0,0.28,0.67, },
        { 2.26,1.86,1.58,1.36,1.16,0.97,0.79,0.61,0.43,0.23,0,0,0,0.40, },
        { 2.53,2.14,1.86,1.63,1.43,1.25,1.07,0.89,0.70,0.50,0.28,0,0,0, },
        { 2.93,2.53,2.26,2.03,1.83,1.65,1.47,1.29,1.10,0.90,0.67,0.40,0,0, },
    },
    {
        { 0,0,0.39,0.66,0.88,1.07,1.25,1.42,1.58,1.75,1.93,2.12,2.34,2.61,3.00, },
        { 0,0,0,0.27,0.49,0.68,0.86,1.03,1.19,1.36,1.54,1.73,1.95,2.22,2.61, },
        { 0.39,0,0,0,0.22,0.41,0.59,0.76,0.93,1.09,1.27,1.46,1.68,1.95,2.34, },
        { 0.66,0.27,0,0,0,0.19,0.37,0.54,0.71,0.88,1.05,1.25,1.46,1.73,2.12, },
        { 0.88,0.49,0.22,0,0,0,0.18,0.35,0.51,0.68,0.86,1.05,1.27,1.54,1.93, },
        { 1.07,0.68,0.41,0.19,0,0,0,0.17,0.34,0.51,0.68,0.88,1.09,1.36,1.75, },
        { 1.25,0.86,0.59,0.37,0.18,0,0,0,0.17,0.34,0.51,0.71,0.93,1.19,1.58, },
        { 1.42,1.03,0.76,0.54,0.35,0.17,0,0,0,0.17,0.35,0.54,0.76,1.03,1.42, },
        { 1.58,1.19,0.93,0.71,0.51,0.34,0.17,0,0,0,0.18,0.37,0.59,0.86,1.25, },
        { 1.75,1.36,1.09,0.88,0.68,0.51,0.34,0.17,0,0,0,0.19,0.41,0.68,1.07, },
        { 1.93,1.54,1.27,1.05,0.86,0.68,0.51,0.35,0.18,0,0,0,0.22,0.49,0.88, },
        { 2.12,1.73,1.46,1.25,1.05,0.88,0.71,0.54,0.37,0.19,0,0,0,0.27,0.66, },
        { 2.34,1.95,1.68,1.46,1.27,1.09,0.93,0.76,0.59,0.41,0.22,0,0,0,0.39, },
        { 2.61,2.22,1.95,1.73,1.54,1.36,1.19,1.03,0.86,0.68,0.49,0.27,0,0,0, },
        { 3.00,2.61,2.34,2.12,1.93,1.75,1.58,1.42,1.25,1.07,0.88,0.66,0.39,0,0, },
    },
    {
        { 0,0,0.38,0.65,0.86,1.05,1.22,1.38,1.53,1.69,1.85,2.02,2.21,2.42,2.68,3.07 },
        { 0,0,0,0.26,0.48,0.66,0.83,0.99,1.15,1.31,1.47,1.64,1.82,2.04,2.30,2.68 },
        { 0.38,0,0,0,0.21,0.40,0.57,0.73,0.89,1.04,1.21,1.38,1.56,1.77,2.04,2.42 },
        { 0.65,0.26,0,0,0,0.19,0.36,0.52,0.67,0.83,0.99,1.16,1.35,1.56,1.82,2.21 },
        { 0.86,0.48,0.21,0,0,0,0.17,0.33,0.49,0.65,0.81,0.98,1.16,1.38,1.64,2.02 },
        { 1.05,0.66,0.40,0.19,0,0,0,0.16,0.32,0.48,0.64,0.81,0.99,1.21,1.47,1.85 },
        { 1.22,0.83,0.57,0.36,0.17,0,0,0,0.16,0.31,0.48,0.65,0.83,1.04,1.31,1.69 },
        { 1.38,0.99,0.73,0.52,0.33,0.16,0,0,0,0.16,0.32,0.49,0.67,0.89,1.15,1.53 },
        { 1.53,1.15,0.89,0.67,0.49,0.32,0.16,0,0,0,0.16,0.33,0.52,0.73,0.99,1.38 },
        { 1.69,1.31,1.04,0.83,0.65,0.48,0.31,0.16,0,0,0,0.17,0.36,0.57,0.83,1.22 },
        { 1.85,1.47,1.21,0.99,0.81,0.64,0.48,0.32,0.16,0,0,0,0.19,0.40,0.66,1.05 },
        { 2.02,1.64,1.38,1.16,0.98,0.81,0.65,0.49,0.33,0.17,0,0,0,0.21,0.48,0.86 },
        { 2.21,1.82,1.56,1.35,1.16,0.99,0.83,0.67,0.52,0.36,0.19,0,0,0,0.26,0.65 },
        { 2.42,2.04,1.77,1.56,1.38,1.21,1.04,0.89,0.73,0.57,0.40,0.21,0,0,0,0.38 },
        { 2.68,2.30,2.04,1.82,1.64,1.47,1.31,1.15,0.99,0.83,0.66,0.48,0.26,0,0,0 },
        { 3.07,2.68,2.42,2.21,2.02,1.85,1.69,1.53,1.38,1.22,1.05,0.86,0.65,0.38,0,0 }
    }
};

static sts_symbol get_symbol(double value, unsigned int c) {
    if (isnan(value)) return c;
    for (unsigned int i = 0; i < c; ++i) {
        if (value >= breaks[c-2][i]
            &&
            value < breaks[c-2][i+1]) {
            return c - i - 1;
        }
    }
    return 0;
}

static double *normalize(double *series_begin, size_t n_values, 
        double *series_end, double *buffer_start, double *buffer_break) {
    if (series_end == NULL) series_end = series_begin + n_values;
    size_t actual_n_values = n_values;
    size_t i = 0;
    double *value = series_begin;
    double *series = malloc(n_values * sizeof(double));
    if (!series) return NULL;

    // Copy elements from buffer into *series
    while (value != series_end) {
        series[i++] = *value;
        if (++value == buffer_break) value = buffer_start;
    }

    double mu = 0, std = 0;
    // Estimate mean
    for (i = 0; i < n_values; ++i) {
        if (!isfinite(series[i])) {
            --actual_n_values;
        } else {
            mu += series[i];
        }
    }
    mu /= actual_n_values > 0 ? actual_n_values : 1;

    // Estimate variance
    for (i = 0; i < n_values; ++i) {
        if (!isfinite(series[i])) continue;
        std += (mu - series[i]) * (mu - series[i]);
    }
    std /= actual_n_values > 0 ? actual_n_values : 1;
    std = sqrt(std);

    // Scale *series
    if (std < STS_STAT_EPS && actual_n_values != 0) {
        // to prevent infinite-scaling for almost-stationary sequencies
        memset(series, 0, n_values * sizeof(double));
    } else {
        for (i = 0; i < n_values; ++i) {
            if (isfinite(series[i])) {
                series[i] = (series[i] - mu) / std;
            }
        }
    }

    return series;
}

static sts_window new_window(size_t n, size_t w, short c, struct sts_ring_buffer *values) {
    sts_window window = malloc(sizeof *window);
    window->n_values = n;
    window->w = w;
    window->c = c;
    window->values = values;
    return window;
}

sts_window sts_new_window(size_t n, size_t w, unsigned int c) {
    if (n % w != 0 || c > STS_MAX_CARDINALITY || c < 2) {
        return NULL;
    }
    struct sts_ring_buffer *values = malloc(sizeof(*values));
    if (!values) return NULL;
    values->cnt=0;
    values->buffer = malloc((n + 1) * sizeof values->buffer);
    if (!values->buffer) {
        free(values);
        return NULL;
    }
    values->buffer_end = values->buffer + n + 1;
    values->head = values->tail = values->buffer;
    return new_window(n, w, c, values);
}

void rb_push(struct sts_ring_buffer* rb, double value)
{
    *rb->head = value;
    ++rb->head;
    if (rb->head == rb->buffer_end)
        rb->head = rb->buffer;

    if (rb->head == rb->tail) {
        if ((rb->tail + 1) == rb->buffer_end)
            rb->tail = rb->buffer;
        else
            ++rb->tail;
    } else {
        ++rb->cnt;
    }
}

static void apply_sax_transform(size_t n, size_t w, unsigned int c, sts_symbol *out, 
        double *series) {
    unsigned int frame_size = n / w;
    for (unsigned int i = 0; i < w; ++i) {
        double average = 0;
        unsigned int current_frame_size = frame_size;
        for (size_t j = i * frame_size; j < (i+1) * frame_size; ++j) {
            if (isnan(series[j])) {
                --current_frame_size;
                continue;
            }
            average += series[j];
        } 
        if (current_frame_size == 0 || isnan(average)) {
            // All NaNs or (-INF + INF)
            average = NAN;
        } else {
            average /= current_frame_size;
        }
        out[i] = get_symbol(average, c);
    }
}

static sts_word new_word(size_t n, size_t w, short c, sts_symbol *symbols) {
    sts_word new = malloc(sizeof *new);
    new->n_values = n;
    new->w = w;
    new->c = c;
    new->symbols = symbols;
    return new;
}

sts_word sts_append_value(sts_window window, double value) {
    if (window == NULL || window->values == NULL || window->values->buffer == NULL ||
            window->c < 2 || window->c > STS_MAX_CARDINALITY)
        return NULL;
    rb_push(window->values, value);
    if (window->values->cnt < window->n_values) return NULL;

    sts_symbol *symbols = malloc(window->w * sizeof(sts_symbol));
    if (!symbols) return NULL;
    double *norm_series = 
        normalize(window->values->tail, window->n_values, window->values->head, 
                window->values->buffer, window->values->buffer_end);
    if (!norm_series) return NULL;
    apply_sax_transform(window->n_values, window->w, window->c, symbols, norm_series);
    free(norm_series);
    return new_word(window->n_values, window->w, window->c, symbols);
}

sts_word sts_to_sax(double *series, size_t n_values, size_t w, unsigned int c) {
    if (n_values % w != 0 || c > STS_MAX_CARDINALITY || c < 2 || series == NULL) {
        return NULL;
    }
    double *norm_series = normalize(series, n_values, NULL, NULL, NULL);
    if (!norm_series) return NULL;
    sts_symbol *symbols =  malloc(w * sizeof(sts_symbol));
    if (!symbols) return NULL;
    apply_sax_transform(n_values, w, c, symbols, norm_series);
    free(norm_series);
    return new_word(n_values, w, c, symbols);
}

double sts_mindist(sts_word a, sts_word b) {
    // TODO: mindist estimation for words of different n, w and c
    if (a->c != b->c || a->w != b->w || a->n_values != b->n_values) return NAN;
    size_t w = a->w, n = a->n_values;
    unsigned int c = a->c;
    if (c > STS_MAX_CARDINALITY || c < 2 || a->symbols == NULL || b->symbols == NULL) {
        return NAN;
    }

    double distance = 0, sym_distance;
    for (size_t i = 0; i < w; ++i) {
        // TODO: other variants of NAN handling, that is:
        // Ignoring, assuming 0 dist to any other symbol, substitution to median, throwing NaN
        sts_symbol x = a->symbols[i] == c ? get_symbol(0, c) : a->symbols[i];
        sts_symbol y = b->symbols[i] == c ? get_symbol(0, c) : b->symbols[i];
        // Current way of handling: substitute with average value
        sym_distance = dist_table[c-2][x][y];
        distance += sym_distance * sym_distance;
    }
    distance = sqrt((double) n / (double) w) * sqrt(distance);
    return distance;
}

bool sts_window_reset(sts_window w) {
    if (w->values == NULL || w->values->buffer == NULL) return false;
    w->values->tail = w->values->head = w->values->buffer;
    w->values->cnt = 0;
    return true;
}

void sts_free_window(sts_window w) {
    if (w->values != NULL) {
        free(w->values->buffer);
        free(w->values);
    }
    free(w);
}

void sts_free_word(sts_word a) {
    if (a->symbols != NULL) free(a->symbols);
    free(a);
}

/* No namespaces in C, so it goes here */
#ifdef STS_COMPILE_UNIT_TESTS

#include "test/sts_test.h"
#include <errno.h>
#include <stdio.h>

static char *test_get_symbol_zero() {
    for (size_t c = 2; c <= STS_MAX_CARDINALITY; ++c) {
        sts_symbol zero_encoded = get_symbol(0.0, c);
        mu_assert(zero_encoded == (c / 2) - 1 + (c % 2),
                "zero encoded into %u for cardinality %zu", zero_encoded, c);
    }
    return NULL;
}

static char *test_get_symbol_breaks() {
    for (size_t c = 2; c <= STS_MAX_CARDINALITY; ++c) {
        for (unsigned int i = 0; i < c; ++i) {
            sts_symbol break_encoded = get_symbol(breaks[c-2][i], c);
            mu_assert(break_encoded == c - i - 1, "%lf encoded into %u instead of %zu. c == %zu", 
                    breaks[c-2][i], break_encoded, c - i - 1, c);
        }
    }
    return NULL;
}

static char *test_to_sax_normalization() {
    double seq[16] = {-4, -3, -2, -1, 0, 1, 2, 3, -4, -3, -2, -1, 0, 1, 2, 3};
    double *normseq = normalize(seq, 16, NULL, NULL, NULL);
    mu_assert(normseq != NULL, "normalize failed");
    for (size_t c = 2; c <= STS_MAX_CARDINALITY; ++c) {
        for (size_t w = 1; w <= 16; w *= 2) {
            sts_word sax = sts_to_sax(seq, 16, w, c), 
                     normsax = sts_to_sax(normseq, 16, w, c);
            mu_assert(sax->symbols != NULL, "sax conversion failed");
            mu_assert(normsax->symbols != NULL, "sax conversion failed");
            mu_assert(memcmp(sax->symbols, normsax->symbols, w) == 0, 
                    "normalized array got encoded differently for w=%zu, c=%zu", w, c);
            sts_free_word(sax);
            sts_free_word(normsax);
        }
    }
    free(normseq);
    return NULL;
}

static char *test_to_sax_sample() {
    // After averaging and normalization this series looks like:
    // {highest sector, lowest sector, sector right above 0, sector right under 0}
    double nseq[12] = {5, 6, 7, -5, -6, -7, 0.25, 0.17, 0.04, -0.04, -0.17, -0.25};
    unsigned int expected[4] = {0, 7, 3, 4};
    sts_word sax = sts_to_sax(nseq, 12, 4, 8);
    mu_assert(sax->symbols != NULL, "sax conversion failed");
    for (int i = 0; i < 4; ++i) {
        mu_assert(sax->symbols[i] == expected[i], 
                "Error converting sample series: \
                batch %d turned into %u instead of %u", i, sax->symbols[i], expected[i]);
    }
    sts_free_word(sax);
    return NULL;
}

static char *test_to_sax_stationary() {
    double sseq[60] = {
        8 + STS_STAT_EPS, 8 - STS_STAT_EPS, 8, 8, 
        8, 8 + STS_STAT_EPS, 8, 8 + STS_STAT_EPS, 
        8 - STS_STAT_EPS, 8, 8 + STS_STAT_EPS, 8,
        8 + STS_STAT_EPS, 8 - STS_STAT_EPS, 8, 8, 
        8, 8 + STS_STAT_EPS, 8, 8 + STS_STAT_EPS, 
        8 - STS_STAT_EPS, 8, 8 + STS_STAT_EPS, 8,
        8 + STS_STAT_EPS, 8 - STS_STAT_EPS, 8, 8, 
        8, 8 + STS_STAT_EPS, 8, 8 + STS_STAT_EPS, 
        8 - STS_STAT_EPS, 8, 8 + STS_STAT_EPS, 8,
        8 + STS_STAT_EPS, 8 - STS_STAT_EPS, 8, 8, 
        8, 8 + STS_STAT_EPS, 8, 8 + STS_STAT_EPS, 
        8 - STS_STAT_EPS, 8, 8 + STS_STAT_EPS, 8,
        8 + STS_STAT_EPS, 8 - STS_STAT_EPS, 8, 8, 
        8, 8 + STS_STAT_EPS, 8, 8 + STS_STAT_EPS, 
        8 - STS_STAT_EPS, 8, 8 + STS_STAT_EPS, 8
    };
    for (size_t c = 2; c <= STS_MAX_CARDINALITY; ++c) {
        for (size_t w = 1; w <= 60; ++w) {
            sts_word sax = sts_to_sax(sseq, 60 - (60 % w), w, c);
            mu_assert(sax->symbols != NULL, "sax conversion failed");
            for (size_t i = 0; i < w; ++i) {
                mu_assert(sax->symbols[i] == (c / 2) - 1 + (c%2),
                        "#%zu element of stationary sequence encoded into %u", i, sax->symbols[i]);
            }
            sts_free_word(sax);
        }
    }
    return NULL;
}

#define TEST_FILL(window, word, test) \
    for (size_t i = 0; i < 16; ++i) { \
        (word) = sts_append_value((window), seq[i]); \
        mu_assert(((word) == NULL) == (i < 15 ? true : false), \
            "sts_append_value failed %zu", i); \
    } \
    mu_assert((window)->values->cnt == 16, "ring buffer failed"); \
    mu_assert((word)->symbols != NULL, "ring buffer failed"); \
    mu_assert(memcmp((test)->symbols, (word)->symbols, w) == 0, "ring buffer failed"); \
    sts_free_word((word)); \
    mu_assert(((word) = sts_append_value((window), 0)) != NULL, "ring buffer failed"); \
    mu_assert((window)->values->cnt == 16, "ring buffer failed"); \
    sts_free_word((word));

static char *test_sliding_word() {
    double seq[16] = 
    {5, 4.2, -3.7, 1.0, 0.1, -2.1, 2.2, -3.3, 4, 0.8, 0.7, -0.2, 4, -3.5, 1.8, -0.4};
    for (unsigned int c = 2; c < STS_MAX_CARDINALITY; ++c) {
        for (size_t w = 1; w <= 16; w*=2) {
            sts_word word = sts_to_sax(seq, 16, w, c);
            sts_window window = sts_new_window(16, w, c);
            mu_assert(window != NULL && window->values != NULL, "sts_new_sliding_word failed");
            mu_assert(word != NULL && word->symbols != NULL, "sts_to_sax failed");
            sts_word dword;
            TEST_FILL(window, dword, word);

            mu_assert(sts_window_reset(window), "sts_winodw_reset failed");
            mu_assert(window->values->cnt == 0, "sts_window_reset failed");
            TEST_FILL(window, dword, word);

            sts_free_word(word);
            sts_free_window(window);
        }
    }
    return NULL;
}

static char *test_nan_and_infinity_in_series() {
    // NaN frames are converted into special symbol and treated accordingly afterwards
    // OTOH, if the frame isn't all-NaN, they are ignored not to mess up the whole frame
    double nseq[12] = {NAN, NAN, INFINITY, -INFINITY, INFINITY, 1, -INFINITY, -1, NAN, -5, 5, NAN};
    unsigned int expected[6] = {8, 8, 0, 7, 7, 0};
    sts_word sax = sts_to_sax(nseq, 12, 6, 8);
    mu_assert(sax->symbols != NULL, "sax conversion failed");
    for (int i = 0; i < 6; ++i) {
        mu_assert(sax->symbols[i] == expected[i], 
                "Error converting sample series: \
                batch %d turned into %u instead of %u", i, sax->symbols[i], expected[i]);
    }
    sts_free_word(sax);
    return NULL;
}

static char* all_tests() {
    mu_run_test(test_get_symbol_zero);
    mu_run_test(test_get_symbol_breaks);
    mu_run_test(test_to_sax_normalization);
    mu_run_test(test_to_sax_sample);
    mu_run_test(test_to_sax_stationary);
    mu_run_test(test_nan_and_infinity_in_series);
    mu_run_test(test_sliding_word);
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

#endif // STS_COMPILE_UNIT_TESTS
