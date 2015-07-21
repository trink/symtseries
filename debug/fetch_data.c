/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "symtseries.h"
#include "debug.h"

const char* getfield(char* line, int num)
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
void fetch_activity_data(int pid, int chid, int evid, 
        double **series, size_t *series_size) {
    char buf[BUF_SIZE];
    for (int sessionid = 0; sessionid < NSESSIONS; ++sessionid) {
        // Parsing next session
        snprintf(buf, BUF_SIZE, 
                "data/train/subj%d_series%d_data.csv", pid, sessionid + 1);
        FILE *dataf = fopen(buf, "r");
        snprintf(buf, BUF_SIZE, 
                "data/train/subj%d_series%d_events.csv", pid, sessionid + 1);
        FILE *eventsf = fopen(buf, "r");
        size_t n_values = 0;
        series[sessionid] = (double *) malloc(sizeof(double));
        // Eat-up both csv headers
        fgets(buf, BUF_SIZE, dataf);
        fgets(buf, BUF_SIZE, eventsf);
        while (fgets(buf, BUF_SIZE, eventsf)) {
            char *tmp = strdup(buf);
            if (strcmp(getfield(tmp, evid+1), "0") == 0) {
                // No activity on event of interest registered -> skip
                continue;
            }
            fgets(buf, BUF_SIZE, dataf);
            tmp = strdup(buf);
            series[sessionid][n_values++] = 
                strtod(getfield(tmp, chid+1), NULL);
            series[sessionid] = (double *)
                realloc(series[sessionid], (n_values + 1) * sizeof(double));
            free(tmp);
        }
        series_size[sessionid] = n_values;
    }}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Provide me with subject id, channel number and event number\n");
        exit(-1);
    }
    int pid = atoi(argv[1]);
    int chid = atoi(argv[2]);
    int evid = atoi(argv[3]);
    double *series[NSESSIONS];
    size_t series_size[NSESSIONS];
    char buf[BUF_SIZE];
    fetch_activity_data(pid, chid, evid, series, series_size);
    for (size_t sessionid = 0; sessionid < NSESSIONS; ++sessionid) {
        snprintf(buf, BUF_SIZE, "plot/session%zu", sessionid + 1);
        FILE *out = fopen(buf, "w");
        for (size_t frameid = 0; frameid < series_size[sessionid]; ++frameid) {
            fprintf(out, "%.2f,%lf\n", 
                    (float) frameid, series[sessionid][frameid]);
        }
        free(series[sessionid]);
    }
}
