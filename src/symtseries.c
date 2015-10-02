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

#ifdef _MSC_VER
// To silence the +INFINITY warning
#pragma warning( disable : 4056 )
#pragma warning( disable : 4756 )
#endif

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

static const double 
dist_table[STS_MAX_CARDINALITY - 1][STS_MAX_CARDINALITY + 1][STS_MAX_CARDINALITY + 1] = 
{
    {
        { 0.000,0.000,0.000, },
        { 0.000,0.000,0.000, },
        { 0.000,0.000,0.000, },
    },
    {
        { 0.000,0.000,0.861,0.861, },
        { 0.000,0.000,0.000,0.000, },
        { 0.861,0.000,0.000,0.861, },
        { 0.861,0.000,0.861,0.000, },
    },
    {
        { 0.000,0.000,0.674,1.349,1.349, },
        { 0.000,0.000,0.000,0.674,0.674, },
        { 0.674,0.000,0.000,0.000,0.674, },
        { 1.349,0.674,0.000,0.000,1.349, },
        { 1.349,0.674,0.674,1.349,0.000, },
    },
    {
        { 0.000,0.000,0.588,1.095,1.683,1.683, },
        { 0.000,0.000,0.000,0.507,1.095,1.095, },
        { 0.588,0.000,0.000,0.000,0.588,0.588, },
        { 1.095,0.507,0.000,0.000,0.000,1.095, },
        { 1.683,1.095,0.588,0.000,0.000,1.683, },
        { 1.683,1.095,0.588,1.095,1.683,0.000, },
    },
    {
        { 0.000,0.000,0.537,0.967,1.398,1.935,1.935, },
        { 0.000,0.000,0.000,0.431,0.861,1.398,1.398, },
        { 0.537,0.000,0.000,0.000,0.431,0.967,0.967, },
        { 0.967,0.431,0.000,0.000,0.000,0.537,0.967, },
        { 1.398,0.861,0.431,0.000,0.000,0.000,1.398, },
        { 1.935,1.398,0.967,0.537,0.000,0.000,1.935, },
        { 1.935,1.398,0.967,0.967,1.398,1.935,0.000, },
    },
    {
        { 0.000,0.000,0.502,0.888,1.248,1.634,2.135,2.135, },
        { 0.000,0.000,0.000,0.386,0.746,1.132,1.634,1.634, },
        { 0.502,0.000,0.000,0.000,0.360,0.746,1.248,1.248, },
        { 0.888,0.386,0.000,0.000,0.000,0.386,0.888,0.888, },
        { 1.248,0.746,0.360,0.000,0.000,0.000,0.502,1.248, },
        { 1.634,1.132,0.746,0.386,0.000,0.000,0.000,1.634, },
        { 2.135,1.634,1.248,0.888,0.502,0.000,0.000,2.135, },
        { 2.135,1.634,1.248,0.888,1.248,1.634,2.135,0.000, },
    },
    {
        { 0.000,0.000,0.476,0.832,1.150,1.469,1.825,2.301,2.301, },
        { 0.000,0.000,0.000,0.356,0.674,0.993,1.349,1.825,1.825, },
        { 0.476,0.000,0.000,0.000,0.319,0.637,0.993,1.469,1.469, },
        { 0.832,0.356,0.000,0.000,0.000,0.319,0.674,1.150,1.150, },
        { 1.150,0.674,0.319,0.000,0.000,0.000,0.356,0.832,1.150, },
        { 1.469,0.993,0.637,0.319,0.000,0.000,0.000,0.476,1.469, },
        { 1.825,1.349,0.993,0.674,0.356,0.000,0.000,0.000,1.825, },
        { 2.301,1.825,1.469,1.150,0.832,0.476,0.000,0.000,2.301, },
        { 2.301,1.825,1.469,1.150,1.150,1.469,1.825,2.301,0.000, },
    },
    {
        { 0.000,0.000,0.456,0.790,1.081,1.360,1.651,1.985,2.441,2.441, },
        { 0.000,0.000,0.000,0.334,0.625,0.904,1.195,1.529,1.985,1.985, },
        { 0.456,0.000,0.000,0.000,0.291,0.570,0.861,1.195,1.651,1.651, },
        { 0.790,0.334,0.000,0.000,0.000,0.279,0.570,0.904,1.360,1.360, },
        { 1.081,0.625,0.291,0.000,0.000,0.000,0.291,0.625,1.081,1.081, },
        { 1.360,0.904,0.570,0.279,0.000,0.000,0.000,0.334,0.790,1.360, },
        { 1.651,1.195,0.861,0.570,0.291,0.000,0.000,0.000,0.456,1.651, },
        { 1.985,1.529,1.195,0.904,0.625,0.334,0.000,0.000,0.000,1.985, },
        { 2.441,1.985,1.651,1.360,1.081,0.790,0.456,0.000,0.000,2.441, },
        { 2.441,1.985,1.651,1.360,1.081,1.360,1.651,1.985,2.441,0.000, },
    },
    {
        { 0.000,0.000,0.440,0.757,1.028,1.282,1.535,1.806,2.123,2.563,2.563, },
        { 0.000,0.000,0.000,0.317,0.588,0.842,1.095,1.366,1.683,2.123,2.123, },
        { 0.440,0.000,0.000,0.000,0.271,0.524,0.778,1.049,1.366,1.806,1.806, },
        { 0.757,0.317,0.000,0.000,0.000,0.253,0.507,0.778,1.095,1.535,1.535, },
        { 1.028,0.588,0.271,0.000,0.000,0.000,0.253,0.524,0.842,1.282,1.282, },
        { 1.282,0.842,0.524,0.253,0.000,0.000,0.000,0.271,0.588,1.028,1.282, },
        { 1.535,1.095,0.778,0.507,0.253,0.000,0.000,0.000,0.317,0.757,1.535, },
        { 1.806,1.366,1.049,0.778,0.524,0.271,0.000,0.000,0.000,0.440,1.806, },
        { 2.123,1.683,1.366,1.095,0.842,0.588,0.317,0.000,0.000,0.000,2.123, },
        { 2.563,2.123,1.806,1.535,1.282,1.028,0.757,0.440,0.000,0.000,2.563, },
        { 2.563,2.123,1.806,1.535,1.282,1.282,1.535,1.806,2.123,2.563,0.000, },
    },
    {
        { 0.000,0.000,0.427,0.731,0.986,1.221,1.449,1.684,1.940,2.244,2.670,2.670, },
        { 0.000,0.000,0.000,0.304,0.560,0.794,1.023,1.257,1.513,1.817,2.244,2.244, },
        { 0.427,0.000,0.000,0.000,0.256,0.490,0.719,0.953,1.209,1.513,1.940,1.940, },
        { 0.731,0.304,0.000,0.000,0.000,0.235,0.463,0.698,0.953,1.257,1.684,1.684, },
        { 0.986,0.560,0.256,0.000,0.000,0.000,0.228,0.463,0.719,1.023,1.449,1.449, },
        { 1.221,0.794,0.490,0.235,0.000,0.000,0.000,0.235,0.490,0.794,1.221,1.221, },
        { 1.449,1.023,0.719,0.463,0.228,0.000,0.000,0.000,0.256,0.560,0.986,1.449, },
        { 1.684,1.257,0.953,0.698,0.463,0.235,0.000,0.000,0.000,0.304,0.731,1.684, },
        { 1.940,1.513,1.209,0.953,0.719,0.490,0.256,0.000,0.000,0.000,0.427,1.940, },
        { 2.244,1.817,1.513,1.257,1.023,0.794,0.560,0.304,0.000,0.000,0.000,2.244, },
        { 2.670,2.244,1.940,1.684,1.449,1.221,0.986,0.731,0.427,0.000,0.000,2.670, },
        { 2.670,2.244,1.940,1.684,1.449,1.221,1.449,1.684,1.940,2.244,2.670,0.000, },
    },
    {
        { 0.000,0.000,0.416,0.709,0.952,1.173,1.383,1.593,1.814,2.057,2.350,2.766,2.766, },
        { 0.000,0.000,0.000,0.293,0.537,0.757,0.967,1.178,1.398,1.642,1.935,2.350,2.350, },
        { 0.416,0.000,0.000,0.000,0.244,0.464,0.674,0.885,1.105,1.349,1.642,2.057,2.057, },
        { 0.709,0.293,0.000,0.000,0.000,0.220,0.431,0.641,0.861,1.105,1.398,1.814,1.814, },
        { 0.952,0.537,0.244,0.000,0.000,0.000,0.210,0.421,0.641,0.885,1.178,1.593,1.593, },
        { 1.173,0.757,0.464,0.220,0.000,0.000,0.000,0.210,0.431,0.674,0.967,1.383,1.383, },
        { 1.383,0.967,0.674,0.431,0.210,0.000,0.000,0.000,0.220,0.464,0.757,1.173,1.383, },
        { 1.593,1.178,0.885,0.641,0.421,0.210,0.000,0.000,0.000,0.244,0.537,0.952,1.593, },
        { 1.814,1.398,1.105,0.861,0.641,0.431,0.220,0.000,0.000,0.000,0.293,0.709,1.814, },
        { 2.057,1.642,1.349,1.105,0.885,0.674,0.464,0.244,0.000,0.000,0.000,0.416,2.057, },
        { 2.350,1.935,1.642,1.398,1.178,0.967,0.757,0.537,0.293,0.000,0.000,0.000,2.350, },
        { 2.766,2.350,2.057,1.814,1.593,1.383,1.173,0.952,0.709,0.416,0.000,0.000,2.766, },
        { 2.766,2.350,2.057,1.814,1.593,1.383,1.383,1.593,1.814,2.057,2.350,2.766,0.000, },
    },
    {
        { 0.000,0.000,0.406,0.690,0.924,1.133,1.330,1.523,1.719,1.928,2.162,2.446,2.852,2.852, },
        { 0.000,0.000,0.000,0.284,0.518,0.727,0.924,1.117,1.313,1.522,1.756,2.040,2.446,2.446, },
        { 0.406,0.000,0.000,0.000,0.234,0.443,0.640,0.833,1.030,1.239,1.473,1.756,2.162,2.162, },
        { 0.690,0.284,0.000,0.000,0.000,0.209,0.406,0.599,0.796,1.005,1.239,1.522,1.928,1.928, },
        { 0.924,0.518,0.234,0.000,0.000,0.000,0.197,0.390,0.587,0.796,1.030,1.313,1.719,1.719, },
        { 1.133,0.727,0.443,0.209,0.000,0.000,0.000,0.193,0.390,0.599,0.833,1.117,1.523,1.523, },
        { 1.330,0.924,0.640,0.406,0.197,0.000,0.000,0.000,0.197,0.406,0.640,0.924,1.330,1.330, },
        { 1.523,1.117,0.833,0.599,0.390,0.193,0.000,0.000,0.000,0.209,0.443,0.727,1.133,1.523, },
        { 1.719,1.313,1.030,0.796,0.587,0.390,0.197,0.000,0.000,0.000,0.234,0.518,0.924,1.719, },
        { 1.928,1.522,1.239,1.005,0.796,0.599,0.406,0.209,0.000,0.000,0.000,0.284,0.690,1.928, },
        { 2.162,1.756,1.473,1.239,1.030,0.833,0.640,0.443,0.234,0.000,0.000,0.000,0.406,2.162, },
        { 2.446,2.040,1.756,1.522,1.313,1.117,0.924,0.727,0.518,0.284,0.000,0.000,0.000,2.446, },
        { 2.852,2.446,2.162,1.928,1.719,1.523,1.330,1.133,0.924,0.690,0.406,0.000,0.000,2.852, },
        { 2.852,2.446,2.162,1.928,1.719,1.523,1.330,1.523,1.719,1.928,2.162,2.446,2.852,0.000, },
    },
    {
        { 0.000,0.000,0.398,0.674,0.899,1.099,1.285,1.465,1.645,1.831,2.031,2.257,2.533,2.930,2.930, },
        { 0.000,0.000,0.000,0.276,0.502,0.701,0.888,1.068,1.248,1.434,1.634,1.859,2.135,2.533,2.533, },
        { 0.398,0.000,0.000,0.000,0.226,0.426,0.612,0.792,0.972,1.158,1.358,1.583,1.859,2.257,2.257, },
        { 0.674,0.276,0.000,0.000,0.000,0.200,0.386,0.566,0.746,0.932,1.132,1.358,1.634,2.031,2.031, },
        { 0.899,0.502,0.226,0.000,0.000,0.000,0.186,0.366,0.546,0.732,0.932,1.158,1.434,1.831,1.831, },
        { 1.099,0.701,0.426,0.200,0.000,0.000,0.000,0.180,0.360,0.546,0.746,0.972,1.248,1.645,1.645, },
        { 1.285,0.888,0.612,0.386,0.186,0.000,0.000,0.000,0.180,0.366,0.566,0.792,1.068,1.465,1.465, },
        { 1.465,1.068,0.792,0.566,0.366,0.180,0.000,0.000,0.000,0.186,0.386,0.612,0.888,1.285,1.465, },
        { 1.645,1.248,0.972,0.746,0.546,0.360,0.180,0.000,0.000,0.000,0.200,0.426,0.701,1.099,1.645, },
        { 1.831,1.434,1.158,0.932,0.732,0.546,0.366,0.186,0.000,0.000,0.000,0.226,0.502,0.899,1.831, },
        { 2.031,1.634,1.358,1.132,0.932,0.746,0.566,0.386,0.200,0.000,0.000,0.000,0.276,0.674,2.031, },
        { 2.257,1.859,1.583,1.358,1.158,0.972,0.792,0.612,0.426,0.226,0.000,0.000,0.000,0.398,2.257, },
        { 2.533,2.135,1.859,1.634,1.434,1.248,1.068,0.888,0.701,0.502,0.276,0.000,0.000,0.000,2.533, },
        { 2.930,2.533,2.257,2.031,1.831,1.645,1.465,1.285,1.099,0.899,0.674,0.398,0.000,0.000,2.930, },
        { 2.930,2.533,2.257,2.031,1.831,1.645,1.465,1.465,1.645,1.831,2.031,2.257,2.533,2.930,0.000, },
    },
    {
        { 0.000,0.000,0.390,0.659,0.878,1.070,1.248,1.417,1.585,1.754,1.932,2.124,2.343,2.612,3.002,3.002, },
        { 0.000,0.000,0.000,0.269,0.488,0.680,0.857,1.027,1.194,1.364,1.541,1.734,1.952,2.222,2.612,2.612, },
        { 0.390,0.000,0.000,0.000,0.219,0.411,0.588,0.758,0.925,1.095,1.272,1.465,1.683,1.952,2.343,2.343, },
        { 0.659,0.269,0.000,0.000,0.000,0.192,0.370,0.539,0.707,0.876,1.054,1.246,1.465,1.734,2.124,2.124, },
        { 0.878,0.488,0.219,0.000,0.000,0.000,0.177,0.347,0.514,0.684,0.861,1.054,1.272,1.541,1.932,1.932, },
        { 1.070,0.680,0.411,0.192,0.000,0.000,0.000,0.170,0.337,0.507,0.684,0.876,1.095,1.364,1.754,1.754, },
        { 1.248,0.857,0.588,0.370,0.177,0.000,0.000,0.000,0.167,0.337,0.514,0.707,0.925,1.194,1.585,1.585, },
        { 1.417,1.027,0.758,0.539,0.347,0.170,0.000,0.000,0.000,0.170,0.347,0.539,0.758,1.027,1.417,1.417, },
        { 1.585,1.194,0.925,0.707,0.514,0.337,0.167,0.000,0.000,0.000,0.177,0.370,0.588,0.857,1.248,1.585, },
        { 1.754,1.364,1.095,0.876,0.684,0.507,0.337,0.170,0.000,0.000,0.000,0.192,0.411,0.680,1.070,1.754, },
        { 1.932,1.541,1.272,1.054,0.861,0.684,0.514,0.347,0.177,0.000,0.000,0.000,0.219,0.488,0.878,1.932, },
        { 2.124,1.734,1.465,1.246,1.054,0.876,0.707,0.539,0.370,0.192,0.000,0.000,0.000,0.269,0.659,2.124, },
        { 2.343,1.952,1.683,1.465,1.272,1.095,0.925,0.758,0.588,0.411,0.219,0.000,0.000,0.000,0.390,2.343, },
        { 2.612,2.222,1.952,1.734,1.541,1.364,1.194,1.027,0.857,0.680,0.488,0.269,0.000,0.000,0.000,2.612, },
        { 3.002,2.612,2.343,2.124,1.932,1.754,1.585,1.417,1.248,1.070,0.878,0.659,0.390,0.000,0.000,3.002, },
        { 3.002,2.612,2.343,2.124,1.932,1.754,1.585,1.417,1.585,1.754,1.932,2.124,2.343,2.612,3.002,0.000, },
    },
    {
        { 0.000,0.000,0.384,0.647,0.860,1.045,1.215,1.377,1.534,1.691,1.853,2.023,2.209,2.421,2.684,3.068,3.068, },
        { 0.000,0.000,0.000,0.263,0.476,0.662,0.832,0.993,1.150,1.308,1.469,1.639,1.825,2.037,2.301,2.684,2.684, },
        { 0.384,0.000,0.000,0.000,0.213,0.398,0.569,0.730,0.887,1.044,1.206,1.376,1.562,1.774,2.037,2.421,2.421, },
        { 0.647,0.263,0.000,0.000,0.000,0.186,0.356,0.517,0.674,0.832,0.993,1.163,1.349,1.562,1.825,2.209,2.209, },
        { 0.860,0.476,0.213,0.000,0.000,0.000,0.170,0.331,0.489,0.646,0.807,0.978,1.163,1.376,1.639,2.023,2.023, },
        { 1.045,0.662,0.398,0.186,0.000,0.000,0.000,0.161,0.319,0.476,0.637,0.807,0.993,1.206,1.469,1.853,1.853, },
        { 1.215,0.832,0.569,0.356,0.170,0.000,0.000,0.000,0.157,0.315,0.476,0.646,0.832,1.044,1.308,1.691,1.691, },
        { 1.377,0.993,0.730,0.517,0.331,0.161,0.000,0.000,0.000,0.157,0.319,0.489,0.674,0.887,1.150,1.534,1.534, },
        { 1.534,1.150,0.887,0.674,0.489,0.319,0.157,0.000,0.000,0.000,0.161,0.331,0.517,0.730,0.993,1.377,1.534, },
        { 1.691,1.308,1.044,0.832,0.646,0.476,0.315,0.157,0.000,0.000,0.000,0.170,0.356,0.569,0.832,1.215,1.691, },
        { 1.853,1.469,1.206,0.993,0.807,0.637,0.476,0.319,0.161,0.000,0.000,0.000,0.186,0.398,0.662,1.045,1.853, },
        { 2.023,1.639,1.376,1.163,0.978,0.807,0.646,0.489,0.331,0.170,0.000,0.000,0.000,0.213,0.476,0.860,2.023, },
        { 2.209,1.825,1.562,1.349,1.163,0.993,0.832,0.674,0.517,0.356,0.186,0.000,0.000,0.000,0.263,0.647,2.209, },
        { 2.421,2.037,1.774,1.562,1.376,1.206,1.044,0.887,0.730,0.569,0.398,0.213,0.000,0.000,0.000,0.384,2.421, },
        { 2.684,2.301,2.037,1.825,1.639,1.469,1.308,1.150,0.993,0.832,0.662,0.476,0.263,0.000,0.000,0.000,2.684, },
        { 3.068,2.684,2.421,2.209,2.023,1.853,1.691,1.534,1.377,1.215,1.045,0.860,0.647,0.384,0.000,0.000,3.068, },
        { 3.068,2.684,2.421,2.209,2.023,1.853,1.691,1.534,1.534,1.691,1.853,2.023,2.209,2.421,2.684,3.068,0.000, },
    },
};

static sts_symbol get_symbol(double value, unsigned int c) {
    if (isnan(value)) return c;
    if (value == -INFINITY) return c - 1;
    for (unsigned int i = 0; i < c; ++i) {
        if (value > breaks[c-2][i] - STS_STAT_EPS
            &&
            value <= breaks[c-2][i+1] - STS_STAT_EPS) {
            return c - i - 1;
        }
    }
    return 0;
}

// On-line estimation for better precision
static void estimate_mu_and_std
(const double *series, size_t n_values, double *mu, double *std) {
    double mean = 0;
    double s2 = 0;
    size_t n = 0;
    for (size_t i = 0; i < n_values; ++i) {
        double value = series[i];
        if (isfinite(value)) {
            ++n;
            s2 += ((value - mean) * (value - mean) * (n - 1)) / n;
            mean += (value - mean) / n;
        }
    }
    if (n == 0) {
        *mu = 0;
        *std = 0;
    } else {
        *mu = mean;
        *std = sqrt(s2 / n);
    }
}

static sts_window new_window(size_t n, size_t w, short c, struct sts_ring_buffer *values) {
    sts_window window = malloc(sizeof *window);
    window->current_word.n_values = n;
    window->current_word.w = w;
    window->current_word.c = c;
    window->current_word.symbols = malloc(w * sizeof *window->current_word.symbols);
    if (window->current_word.symbols == NULL) return NULL;
    for (size_t i = 0; i < w; ++i) {
        window->current_word.symbols[i] = c;
    }
    window->values = values;
    return window;
}

sts_window sts_new_window(size_t n, size_t w, unsigned int c) {
    if (n % w != 0 || c > STS_MAX_CARDINALITY || c < 2) {
        return NULL;
    }
    struct sts_ring_buffer *values = malloc(sizeof *values);
    if (!values) return NULL;
    values->buffer = malloc((n + 1) * sizeof *values->buffer);
    if (!values->buffer) {
        free(values);
        return NULL;
    }
    for (size_t i = 0; i <= n; ++i) {
        values->buffer[i] = NAN;
    }
    values->buffer_end = values->buffer + n + 1;
    values->head = values->tail = values->buffer;
    values->mu = 0;
    values->s2 = 0;
    values->finite_cnt = 0;
    return new_window(n, w, c, values);
}

/*
 * Apend to circular buffer, updates finite_cnt
 */
static double rb_push(struct sts_ring_buffer* rb, double value)
{
    double prev_tail = 0;
    if (isfinite(value)) {
        ++rb->finite_cnt;
    }
    *rb->head = value;
    ++rb->head;
    if (rb->head == rb->buffer_end)
        rb->head = rb->buffer;

    if (rb->head == rb->tail) {
        prev_tail = *rb->tail;
        if (isfinite(*rb->tail)) {
            --rb->finite_cnt;
        }
        if ((rb->tail + 1) == rb->buffer_end)
            rb->tail = rb->buffer;
        else
            ++rb->tail;
    } 

    return prev_tail;
}

/*
 * Given code params, mu and std of series + buffer where that series lies
 * writes SAX-representation of the series into *out
 */

static void apply_sax_transform
(size_t n, size_t w, unsigned int c, double mu, double std, 
 sts_symbol *out, 
 const double *series_begin, const double *buffer_start, const double *buffer_break) 
{
    unsigned int frame_size = n / w;
    const double *val = series_begin;
    for (unsigned int i = 0; i < w; ++i) {
        double average = 0;
        unsigned int current_frame_size = frame_size;
        for (size_t j = 0; j < frame_size; ++j) {
            if (isnan(*val)) {
                --current_frame_size;
            } else {
                average += *val;
            }
            if (++val == buffer_break) val = buffer_start;
        } 
        if (current_frame_size == 0 || isnan(average)) {
            // All NaNs or (-INF + INF)
            average = NAN;
        } else {
            if (isfinite(average)) {
                if (std < STS_STAT_EPS) {
                    average = 0;
                } else {
                    average = (average - (current_frame_size * mu)) / (current_frame_size * std);
                }
            }
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

static double get_window_std(sts_window window) {
    return window->values->finite_cnt == 0  
        ? 0  
        : sqrt(window->values->s2 / window->values->finite_cnt);
}

static sts_word update_current_word(sts_window window) {
    apply_sax_transform(window->current_word.n_values, window->current_word.w, 
            window->current_word.c, window->values->mu, get_window_std(window), 
            window->current_word.symbols, 
            window->values->tail, window->values->buffer, window->values->buffer_end);
    return &window->current_word;
}

/*
 * Appends value, updates mu and s2 in on-line fashion, but doesn't update word itself
 */
static void append_value(sts_window window, double value) {
    size_t prev_finite = window->values->finite_cnt;
    double tail = rb_push(window->values, value);
    size_t new_finite = window->values->finite_cnt;
    // Update mu and s2
    if (prev_finite == new_finite) {
        // either 
        // 1) added finite and removed finite from tail or 
        // 2) added non-finite and removed non-finite or 
        // 3) added non-finite on an empty place
        // update only in case 1 (size remained the same)
        if (isfinite(value)) {
            double diff = value - tail;
            window->values->mu += diff / prev_finite;
            double a = value - window->values->mu;
            double b = tail - window->values->mu;
            window->values->s2 += diff * diff / new_finite + a * a - b * b;
        } 
    } else if (new_finite < prev_finite) {
        // added non-finite in place of finite (size decreased)
        if (new_finite == 0) {
            window->values->mu = 0;
            window->values->s2 = 0;
        } else {
            double prev_mu = window->values->mu;
            window->values->mu = (prev_mu * prev_finite - tail) / new_finite; 
            double old_diff = prev_mu - tail;
            double new_diff = window->values->mu - tail;
            window->values->s2 += ((old_diff * old_diff * prev_finite) 
                    / (new_finite * new_finite)) - new_diff * new_diff;
        }
    } else {
        // added new finite either on the empty place
        // or in place of non-finite tail
        // size increased in any case -> update
        window->values->s2 += ((value - window->values->mu) * (value - window->values->mu) 
                * prev_finite) / new_finite;
        window->values->mu += (value - window->values->mu) / new_finite;
    }
    if (window->values->s2 < 0 && window->values->s2 > - STS_STAT_EPS) {
        // to fight sqrt(-0)
        window->values->s2 = 0;
    }
}

const struct sts_word* sts_append_value(sts_window window, double value) {
    if (window == NULL || window->values == NULL || window->values->buffer == NULL ||
            window->current_word.c < 2 || window->current_word.c > STS_MAX_CARDINALITY)
        return NULL;
    append_value(window, value);
    return update_current_word(window);
}

const struct sts_word* sts_append_array(sts_window window, const double *values, size_t n_values) {
    if (window == NULL || window->values == NULL || window->values->buffer == NULL ||
            window->current_word.c < 2 || window->current_word.c > STS_MAX_CARDINALITY || !values)
        return NULL;
    size_t start = 
        n_values > window->current_word.n_values ? n_values - window->current_word.n_values : 0;
    for (size_t i = start; i < n_values; ++i) {
        append_value(window, values[i]);
    }
    return update_current_word(window);
}

sts_word sts_from_double_array(const double *series, size_t n_values, size_t w, unsigned int c) {
    if (n_values % w != 0 || c > STS_MAX_CARDINALITY || c < 2 || series == NULL) {
        return NULL;
    }
    double mu, sigma;
    estimate_mu_and_std(series, n_values, &mu, &sigma);
    sts_symbol *symbols =  malloc(w * sizeof *symbols);
    if (!symbols) return NULL;
    apply_sax_transform(n_values, w, c, mu, sigma, symbols, series, NULL, NULL);
    return new_word(n_values, w, c, symbols);
}

sts_word sts_from_sax_string(const char *symbols, size_t c) {
    if (!symbols || c < 2 || c > STS_MAX_CARDINALITY) return NULL;
    size_t w = strlen(symbols);
    if (w == 0) return NULL;
    sts_symbol *sts_symbols = malloc(w * sizeof *sts_symbols);
    if (!sts_symbols) return NULL;
    for (size_t i = 0; i < w; ++i) {
        if (symbols[i] == '#') {
            sts_symbols[i] = c;
        } else {
            if (symbols[i] < 'A' || symbols[i] >= (char) ('A' + c)) return NULL;
            sts_symbols[i] = c - (symbols[i] - 'A') - 1;
        }
    }
    return new_word(0, w, c, sts_symbols);
}

char *sts_word_to_sax_string(const struct sts_word *a) {
    if (!a || !a->symbols) return NULL;
    char *str = malloc((a->w + 1) * sizeof *str);
    if (!str) return NULL;
    str[a->w] = '\0';
    for (size_t i = 0; i < a->w; ++i) {
        unsigned char dig = a->symbols[i];
        if (dig > a->c) {
            free(str);
            return NULL;
        }
        if (dig == a->c) {
            // All-NaN frame
            str[i] = '#'; // Not to mix with valid SAX symbols
        } else {
            str[i] = a->c - a->symbols[i] - 1 + 'A';
        }
    }
    return str;
}

double sts_mindist(const struct sts_word* a, const struct sts_word* b) {
    // TODO: mindist estimation for words of different n, w and c
    if (!a || !b || a->c != b->c || a->w != b->w) return NAN;
    if (a->n_values != b->n_values && (a->n_values != 0 && b->n_values != 0)) return NAN;
    size_t w = a->w;
    // sts_word->n_values == 0 means "Default to other word's n" logic
    size_t n = a->n_values > 0 ? a->n_values : b->n_values;
    if (n == 0) return NAN;
    unsigned int c = a->c;
    if (c > STS_MAX_CARDINALITY || c < 2 || a->symbols == NULL || b->symbols == NULL) {
        return NAN;
    }

    double distance = 0, sym_distance;
    for (size_t i = 0; i < w; ++i) {
        // TODO: other variants of NAN handling, i.e.:
        // Ignoring, assuming 0 dist to any other symbol, substitution to mean, throwing NaN
        sym_distance = dist_table[c-2][a->symbols[i]][b->symbols[i]];
        distance += sym_distance * sym_distance;
    }
    distance = sqrt((double) n / (double) w) * sqrt(distance);
    return distance;
}

bool sts_words_equal(const struct sts_word* a, const struct sts_word* b) {
    if (!a || !b) return false;
    if (a->w != b->w || a->c != b->c) {
        return false;
    }
    return memcmp(a->symbols, b->symbols, a->w * sizeof *a->symbols) == 0;
}

bool sts_reset_window(sts_window w) {
    if (!w || w->values == NULL || w->values->buffer == NULL) return false;
    w->values->tail = w->values->head = w->values->buffer;
    w->values->mu = 0;
    w->values->s2 = 0;
    w->values->finite_cnt = 0;
    for (size_t i = 0; i < w->current_word.w; ++i) {
        w->current_word.symbols[i] = w->current_word.c;
    }
    return true;
}

void sts_free_window(sts_window w) {
    if (!w) return;
    if (w->values != NULL) {
        free(w->values->buffer);
        free(w->values);
    }
    if (w->current_word.symbols != NULL)
        free(w->current_word.symbols);
    free(w);
}

void sts_free_word(sts_word a) {
    if (!a) return;
    if (a->symbols != NULL) free(a->symbols);
    free(a);
}

sts_word sts_dup_word(const struct sts_word* a) {
    if (a == NULL || a->c < 2 || a->c > STS_MAX_CARDINALITY || a->symbols == NULL)
        return NULL;
    sts_symbol *sts_symbols = malloc(a->w * sizeof *sts_symbols);
    memcpy(sts_symbols, a->symbols, a->w * sizeof *sts_symbols);
    return new_word(a->n_values, a->w, a->c, sts_symbols);
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
                "zero encoded into %u for cardinality %" PRIuSIZE, zero_encoded, (usize) c);
    }
    return NULL;
}

static char *test_get_symbol_breaks() {
    for (size_t c = 2; c <= STS_MAX_CARDINALITY; ++c) {
        for (unsigned int i = 0; i < c; ++i) {
            sts_symbol break_encoded = get_symbol(breaks[c-2][i], c);
            mu_assert(break_encoded == c - i - 1, "%lf encoded into %u instead of %" 
                    PRIuSIZE ". c == %" PRIuSIZE, 
                    breaks[c-2][i], break_encoded, (usize) c - i - 1, (usize) c);
        }
    }
    return NULL;
}

static char *test_to_sax_sample() {
    // After averaging and normalization this series looks like:
    // {highest sector, lowest sector, sector right above 0, sector right under 0}
    double nseq[12] = {5, 6, 7, -5, -6, -7, 0.25, 0.17, 0.04, -0.04, -0.17, -0.25};
    unsigned int expected[4] = {0, 7, 3, 4};
    sts_word sax = sts_from_double_array(nseq, 12, 4, 8);
    char sym[] = "HAED";
    sts_word symsax = sts_from_sax_string(sym, 8);
    mu_assert(sax->symbols != NULL, "sax conversion failed");
    for (int i = 0; i < 4; ++i) {
        mu_assert(sax->symbols[i] == expected[i], 
                "Error converting sample series: batch %d turned into %u instead of %u", 
                i, sax->symbols[i], expected[i]);
        mu_assert(symsax->symbols[i] == expected[i], 
                "Error converting sample series: batch %d (%c) turned into %u instead of %u", 
                i, sym[i], symsax->symbols[i], expected[i]);
    }
    sts_free_word(sax);
    sts_free_word(symsax);
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
            sts_word sax = sts_from_double_array(sseq, 60 - (60 % w), w, c);
            mu_assert(sax->symbols != NULL, "sax conversion failed");
            for (size_t i = 0; i < w; ++i) {
                mu_assert(sax->symbols[i] == (c / 2) - 1 + (c%2),
                        "#%" PRIuSIZE "element of stationary sequence encoded into %u", 
                        (usize) i, sax->symbols[i]);
            }
            sts_free_word(sax);
        }
    }
    return NULL;
}

#define TEST_FILL(window, word, test) \
    for (size_t i = 0; i < 16; ++i) { \
        (word) = sts_append_value((window), seq[i]); \
        mu_assert((word) != NULL, \
            "sts_append_value failed %" PRIuSIZE, (usize) i); \
    } \
    mu_assert((window)->values->finite_cnt == 16, "ring buffer failed"); \
    mu_assert((word)->symbols != NULL, "ring buffer failed"); \
    mu_assert(memcmp((test)->symbols, (word)->symbols, w) == 0, "ring buffer failed"); \
    mu_assert(((word) = sts_append_value((window), 0)) != NULL, "ring buffer failed"); \
    mu_assert((window)->values->finite_cnt == 16, "ring buffer failed"); \

static bool words_equal(const struct sts_word* a, const struct sts_word* b) {
    return a->n_values == b->n_values && a->w == b->w && a->c == b->c &&
        memcmp(a->symbols, b->symbols, a->w * sizeof *a->symbols) == 0;
}

static char *test_sliding_word() {
    double seq[16] = 
    {5, 4.2, -3.7, 1.0, 0.1, -2.1, 2.2, -3.3, 4, 0.8, 0.7, -0.2, 4, -3.5, 1.8, -0.4};
    double nseq[17] = 
    {5, 4.2, -3.7, 1.0, 0.1, -2.1, 2.2, -3.3, 4, 0.8, 0.7, -0.2, 4, -3.5, 1.8, -0.4, 0.0};
    for (unsigned int c = 2; c < STS_MAX_CARDINALITY; ++c) {
        for (size_t w = 1; w <= 16; w*=2) {
            sts_word word = sts_from_double_array(seq, 16, w, c);
            sts_window window = sts_new_window(16, w, c);
            mu_assert(window != NULL && window->values != NULL, "sts_new_sliding_word failed");
            mu_assert(word != NULL && word->symbols != NULL, "sts_from_double_array failed");
            const struct sts_word* dword;
            TEST_FILL(window, dword, word);
            sts_word cword = sts_dup_word(dword);
            mu_assert(cword->symbols != NULL, "sts_dup_word failed");
            mu_assert(cword->symbols != dword->symbols, "sts_dup_word should allocate new word");

            mu_assert(sts_reset_window(window), "sts_winodw_reset failed");
            mu_assert(window->values->finite_cnt == 0, "sts_reset_window failed");
            TEST_FILL(window, dword, word);
            mu_assert(words_equal(cword, dword), "sts_dup_word failed");
            mu_assert(words_equal(cword, sts_append_array(window, nseq, 17)), 
                    "sts_append_array failed");

            sts_free_word(word);
            sts_free_word(cword);
            sts_free_window(window);
        }
    }
    return NULL;
}

static bool isclose(double a, double b) {
    return fabs(a - b) < STS_STAT_EPS;
}

void swap(size_t *a, size_t *b) {
    size_t c = *a;
    *a = *b;
    *b = c;
}

#include <time.h>
#include <stdlib.h>
#define STS_TEST_BUF_SIZE 1000
static char *online_mu_sigma_random_test() {
    size_t n_runs = 250;
    double buf[STS_TEST_BUF_SIZE];
    srand(time(NULL));
    sts_window win;
    size_t n_values = 32;
    size_t prev_fin, new_fin;
    size_t w = 8;
    size_t c = 6;
    for (size_t i = 0; i < n_runs; ++i) {
        for (size_t j = 0; j < STS_TEST_BUF_SIZE; ++j) {
            buf[j] = (float)rand()/(float)(RAND_MAX/10.0);
            int r = rand() % 15;
            if (r == 0) buf[j] = NAN;
            else if (r == 1) buf[j] = INFINITY;
            else if (r == 2) buf[j] = -INFINITY;
        }
        for (size_t offset = 0; offset < STS_TEST_BUF_SIZE - n_values - 1; ++offset) {
            if (offset == 0) {
                win = sts_new_window(n_values, w, c);
                sts_append_array(win, buf, n_values);
                prev_fin = win->values->finite_cnt;
                new_fin = win->values->finite_cnt;
            } else {
                sts_append_value(win, buf[n_values + offset - 1]);
                new_fin = win->values->finite_cnt;
            }
            double mu, std;
            estimate_mu_and_std(buf + offset, n_values, &mu, &std);
            double winmu = win->values->mu; 
            double winstd = get_window_std(win);
            if (!isclose(mu, winmu) || 
                !isclose(std, winstd)) {
                printf("%zu, %zu, %zu\n", w, c, offset);
                printf("window.mu = %lf, window.std = %lf, window.s2 = %lf\n", 
                        winmu, winstd, win->values->s2);
                printf("actual.mu = %lf, actual.std = %lf\n", mu, std);
                printf("prev_fin = %zu, new_fin = %zu\n", prev_fin, new_fin);
                for (size_t printid = 0; printid < offset + n_values; ++printid) {
                    printf("%f, ", buf[printid]);
                }
                printf("\n");
                mu_assert(0, "sigma and mu are sufficiently different between " 
                        "word and window estimations");
            }
            swap(&prev_fin, &new_fin);
        }
        sts_free_window(win);
    }
    return NULL;
}

static char *test_nan_and_infinity_in_series() {
    // NaN frames are converted into special symbol and treated accordingly afterwards
    // OTOH, if the frame isn't all-NaN, they are ignored not to mess up the whole frame
    double nseq[12] = {NAN, NAN, INFINITY, -INFINITY, INFINITY, 1, -INFINITY, -1, NAN, -5, 5, NAN};
    unsigned int expected[6] = {8, 8, 0, 7, 7, 0};
    sts_word sax = sts_from_double_array(nseq, 12, 6, 8);
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
    mu_run_test(test_to_sax_sample);
    mu_run_test(test_to_sax_stationary);
    mu_run_test(test_nan_and_infinity_in_series);
    mu_run_test(test_sliding_word);
    mu_run_test(online_mu_sigma_random_test);
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
