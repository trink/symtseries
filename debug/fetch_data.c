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

/*
 * Parses subject-specific files in train and 
 * returns the subrange corresponding to specified activity
 */
static void fetch_activity_data(int pid, int chid, int evid, 
        double **series, size_t *series_size) {
    char buf[BUF_SIZE];
    for (int sid = 0; sid < NSESSIONS; ++sid) {
        // Parsing next session
        snprintf(buf, BUF_SIZE, 
                "data/train/subj%d_series%d_data.csv", pid + 1, sid + 1);
        FILE *dataf = fopen(buf, "r");
        snprintf(buf, BUF_SIZE, 
                "data/train/subj%d_series%d_events.csv", pid + 1, sid + 1);
        FILE *eventsf = fopen(buf, "r");
        size_t n_values = 0;
        series[sid] = (double *) malloc(sizeof(double));
        // Eat-up both csv headers
        fgets(buf, BUF_SIZE, dataf);
        fgets(buf, BUF_SIZE, eventsf);
        while (fgets(buf, BUF_SIZE, eventsf)) {
            char *tmp = strdup(buf);
            if (strcmp(getfield(tmp, evid+2), "0") == 0) {
                // No activity on event of interest registered -> skip
                free(tmp);
                continue;
            }
            free(tmp);
            fgets(buf, BUF_SIZE, dataf);
            tmp = strdup(buf);
            series[sid][n_values++] = 
                strtod(getfield(tmp, chid+2), NULL);
            series[sid] = (double *)
                realloc(series[sid], (n_values + 1) * sizeof(double));
            free(tmp);
        }
        fclose(dataf);
        fclose(eventsf);
        series_size[sid] = n_values;
    }
}

static double n_dim_dist(sax_word *word1, sax_word *word2, 
        size_t n, size_t w, unsigned int c, size_t dim) {
    double dist = 0, idist;
    for (size_t i = 0; i < dim; ++i) {
        idist = sts_mindist(word1[i], word2[i], n, w, c);
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
    int pid = atoi(argv[2]), evid = atoi(argv[3]), chid, w, c;
    int fetch = 0;
    if (strcmp("fetch", argv[1]) == 0) {
        fetch = 1;
        if (argc < 5) {
            printf("Provide me with channel number\n");
            exit(-1);
        }
        chid = atoi(argv[4]);
    } else {
        if (argc < 6) {
            printf("Provide me with w and c\n");
            exit(-1);
        }
        w = atoi(argv[4]);
        c = atoi(argv[5]);
    }
    double *series[NSESSIONS];
    size_t series_size[NSESSIONS];
    if (fetch) {
        char buf[BUF_SIZE];
        fetch_activity_data(pid, chid, evid, series, series_size);
        for (size_t sid = 0; sid < NSESSIONS; ++sid) {
            snprintf(buf, BUF_SIZE, "plot/session%zu", sid + 1);
            FILE *out = fopen(buf, "w");
            for (size_t frameid = 0; frameid < series_size[sid]; ++frameid) {
                fprintf(out, "%.2f,%lf\n", 
                        (float) frameid, series[sid][frameid]);
            }
            free(series[sid]);
        }
    } else {
        sax_word sessions[NSESSIONS][NCHANNELS];
        size_t n_values[NSESSIONS][NCHANNELS];
        size_t min_event_length = INT_MAX;
        for (size_t chid = 0; chid < NCHANNELS; ++chid) {
            fetch_activity_data(pid, chid, evid, series, series_size);
            for (size_t sid = 0; sid < NSESSIONS; ++sid) {
                // SAX conversion is defined for even partitioning only
                n_values[sid][chid] = series_size[sid] - series_size[sid] % w;
                sessions[sid][chid] = 
                    sts_to_iSAX(series[sid], n_values[sid][chid], w, c);
                if (n_values[sid][chid] < min_event_length)
                    min_event_length = n_values[sid][chid];
                free(series[sid]);
            }
        }
        for (size_t sid = 0; sid < NSESSIONS; ++sid) {
            double mindist = DBL_MAX;
            for (size_t oid = 0; oid < NSESSIONS; ++oid) {
                if (sid == oid) continue;
                // We need both tseries to have equal size for this to work.
                // Since they are all different-sized originally, lets 
                // cut them down to the smallest one to start with.
                // And yes, we are SAX-converting them using their original
                // length, it is by design.
                double dist = 
                    n_dim_dist(sessions[sid], sessions[oid], 
                            min_event_length, w, c, NCHANNELS);
                if (mindist > dist) mindist = dist;
            }
            printf("series #%zu: mindist == %lf\n", sid, mindist);
        }
    }
}
