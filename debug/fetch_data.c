/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "debug.h"

static const char* getfield(char* line, int num)
{
    const char* tok;
    for (tok = strtok(line, ",");
            tok && *tok;
            tok = strtok(NULL, ",\n"))
    {
        if (!--num)
            return tok;
    }
    return NULL;
}

#define BUF_SIZE 10000

#define NCHANNELS 32
#define NSESSIONS 8

#define FEXIT(series, size) {*(series) = NULL; *(size) = NULL; return 0;}

#define SFOR(i, j, limit_i, limit_j) \
    for (size_t (i) = 0; (i) < (limit_i); ++(i)) \
        for (size_t (j) = 0; (j) < (limit_j); ++(j))

/*
 * Parses subject-specific files in train and 
 * returns the subrange corresponding to specified activity
 */
static size_t fetch_activity_data(int pid, int chid, int evid, int sid,
        double* **series, size_t* *series_size) {
    char buf[BUF_SIZE];
    snprintf(buf, BUF_SIZE, 
            "data/train/subj%d_series%d_data.csv", pid + 1, sid + 1);
    FILE *dataf = fopen(buf, "r");
    snprintf(buf, BUF_SIZE, 
            "data/train/subj%d_series%d_events.csv", pid + 1, sid + 1);
    FILE *eventsf = fopen(buf, "r");
    size_t n_values = 0, n_trials = 0;
    int new = 1;
    *series = NULL;
    *series_size = NULL;
    // Eat-up both csv headers
    if (fgets(buf, BUF_SIZE, dataf) == NULL) FEXIT(series, series_size);
    if (fgets(buf, BUF_SIZE, eventsf) == NULL) FEXIT(series, series_size);
    while (fgets(buf, BUF_SIZE, eventsf)) {
        char *tmp = strdup(buf);
        if (strcmp(getfield(tmp, evid+2), "0") == 0) {
            // No activity on event of interest registered -> skip
            new = 1;
            free(tmp);
            continue;
        }
        if (new) {
            new = 0;
            n_values = 0;
            *series = (double **) realloc(*series, (++n_trials) * sizeof(double *));
            (*series)[n_trials - 1] = NULL;
            *series_size = (size_t *) realloc(*series_size, n_trials * sizeof(size_t));
        }
        free(tmp);
        if (fgets(buf, BUF_SIZE, dataf) == NULL) FEXIT(series, series_size);
        tmp = strdup(buf);
        (*series)[n_trials - 1] = (double *)
            realloc((*series)[n_trials - 1], (++n_values) * sizeof(double));
        (*series)[n_trials - 1][n_values - 1] = strtod(getfield(tmp, chid+2), NULL);
        free(tmp);
        (*series_size)[n_trials - 1] = n_values;
    }
    fclose(dataf);
    fclose(eventsf);
    return n_trials;
}

/* Better implement it this way than support data-cube rotation */
static double n_dim_dist(size_t dim, sax_word* word_matrix[][dim], 
        size_t row1, size_t column1, size_t row2, size_t column2, 
        size_t n, size_t w, unsigned int c) {
    double dist = 0, idist;
    for (size_t i = 0; i < dim; ++i) {
        idist = sts_mindist(word_matrix[row1][i][column1], 
                            word_matrix[row2][i][column2], n, w, c);
        dist += idist * idist;
    }
    return sqrt(dist);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("No option specified, exiting.\n(Should be one of: fetch, dist)\n");
        exit(-1);
    }
    if (argc < 4) {
        printf("Provide me with subject id and event number\n");
        exit(-1);
    }
    int pid = atoi(argv[2]), evid = atoi(argv[3]), chid, sid, w, c;
    int fetch = 0;
    if (strcmp("fetch", argv[1]) == 0) {
        fetch = 1;
        if (argc < 6) {
            printf("Provide me with channel number and session number\n");
            exit(-1);
        }
        chid = atoi(argv[4]);
        sid = atoi(argv[5]);
    } else {
        if (argc < 6) {
            printf("Provide me with w and c\n");
            exit(-1);
        }
        w = atoi(argv[4]);
        c = atoi(argv[5]);
    }
    double **series;
    size_t *series_size;
    // TODO: move both operations to separate files
    if (fetch) {
        char buf[BUF_SIZE];
        size_t n_trials = 
            fetch_activity_data(pid, chid, evid, sid, &series, &series_size);
        if (series == NULL) {
            printf("I/O problems occured, exiting\n");
            exit(-1);
        }
        for (size_t trid = 0; trid < n_trials; ++trid) {
            snprintf(buf, BUF_SIZE, "plot/trial%zu", trid + 1);
            FILE *out = fopen(buf, "w");
            for (size_t frameid = 0; frameid < series_size[trid]; ++frameid) {
                fprintf(out, "%.2f,%lf\n", 
                        (float) frameid, series[sid][frameid]);
            }
            fclose(out);
            free(series[sid]);
        }
    } else {
        sax_word *trials[NSESSIONS][NCHANNELS];
        size_t n_trials[NSESSIONS];
        size_t min_event_length = INT_MAX;
        for (size_t chid = 0; chid < NCHANNELS; ++chid) {
            for (size_t sid = 0; sid < NSESSIONS; ++sid) {
                n_trials[sid] = 
                    fetch_activity_data(pid, chid, evid, sid, &series, &series_size);
                if (series == NULL) {
                    printf("I/O problems occured, exiting\n");
                    exit(-1);
                }
                trials[sid][chid] = malloc(n_trials[sid] * sizeof(sax_word));
                for (size_t trid = 0; trid < n_trials[sid]; ++trid) {
                    // SAX conversion is defined for even partitioning only
                    series_size[trid] = series_size[trid] - series_size[trid] % w;
                    trials[sid][chid][trid] = 
                        sts_to_iSAX(series[trid], series_size[trid], w, c);
                    if (series_size[trid] < min_event_length)
                        min_event_length = series_size[trid];
                    free(series[trid]);
                }
                free(series);
                free(series_size);
            }
        }
        SFOR(sid, trid, NSESSIONS, n_trials[sid]) {
            double mindist = DBL_MAX;
            SFOR(osid, otrid, NSESSIONS, n_trials[osid]) {
                if (sid == osid && trid == otrid) continue;
                // We need both tseries to have equal size for this to work.
                // Since they are all different-sized originally, lets 
                // scale them down to the smallest one to start with.
                // And yes, we are SAX-converting them using their original
                // length, it is by design.
                double dist = 
                    n_dim_dist(NCHANNELS, trials, sid, trid, osid, otrid, 
                            min_event_length, w, c);
                if (mindist > dist) {
                    mindist = dist;
                }
            }
            printf("session #%zu, trial #%zu: mindist == %lf\n", sid, trid, mindist);
        }
    }
}
