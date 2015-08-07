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
#define EVLEN 150

typedef sax_word sax_signal[NCHANNELS];

struct sax_signal_set {
    sax_signal *series;
    size_t n_series;
};

struct session {
    double **series;
    size_t n_series;
};

void *safe_realloc(void *mem, size_t size) {
    void *new_mem = realloc(mem, size);
    if (!new_mem) {
        printf("Unable to allocate more mem, exiting\n");
        exit(-1);
    }
    return new_mem;
}

void *safe_malloc(size_t size) {
    void *new_mem = malloc(size);
    if (!new_mem) {
        printf("Unable to allocate more mem, exiting\n");
        exit(-1);
    }
    return new_mem;
}

void push_back(double* *series, size_t* size, double value) {
    *series = safe_realloc(*series, (++(*size)) * sizeof(double)); \
    (*series)[(*size) - 1] = value;
}

void push_back_pointer(double* **series, size_t* size, double* value) {
    *series = safe_realloc(*series, (++(*size)) * sizeof(double*)); \
    (*series)[(*size) - 1] = value;
}

void push_back_buffer(double* *val_buffer, size_t* n_values, double value, 
        double* **series, size_t* n_trials) {
    push_back(val_buffer, n_values, value);
    if (*n_values >= EVLEN) {
        double *new_subseq = safe_malloc(EVLEN * sizeof(double));
        memcpy(new_subseq, *val_buffer + (*n_values - EVLEN), EVLEN);
        push_back_pointer(series, n_trials, new_subseq);
    }
}

/*
 * Parses subject-specific files in train and 
 * returns the subrange corresponding to specified activity
 * @param evid: 1-based id of the event. 
 * If evid is negative than each subrange not corresponding to event #evid 
 * of length EVLEN will be returned
 * If evid is zero, than all session subsequences of length EVLEN will be returned
 */
struct session fetch_session_data(int pid, int chid, int sid, int evid) {
    char buf[BUF_SIZE];
    snprintf(buf, BUF_SIZE, 
            "data/train/subj%d_series%d_data.csv", pid + 1, sid + 1);
    FILE *dataf = fopen(buf, "r");
    snprintf(buf, BUF_SIZE, 
            "data/train/subj%d_series%d_events.csv", pid + 1, sid + 1);
    FILE *eventsf = fopen(buf, "r");
    if (!dataf || !eventsf) return (struct session) {NULL, 0};
    // Eat-up both csv headers
    if (fgets(buf, BUF_SIZE, dataf) == NULL) return (struct session) {NULL, 0};
    if (fgets(buf, BUF_SIZE, eventsf) == NULL) return (struct session) {NULL, 0};

    size_t n_values = 0, n_trials = 0;
    int new = 1;
    double *val_buffer = NULL;
    double **series = NULL;
    printf("%d, %d, %d, %d\n", pid, chid, sid, evid);
    fflush(stdout);
    while (fgets(buf, BUF_SIZE, eventsf)) {
        char *tmp = strdup(buf);
        int event_triggered = (strcmp(getfield(tmp, abs(evid)+1), "0") == 1);
        free(tmp);
        if (fgets(buf, BUF_SIZE, dataf) == NULL) return (struct session) {NULL, 0};
        tmp = strdup(buf);
        double value = strtod(getfield(tmp, chid+2), NULL);
        free(tmp);
        if (evid == 0) {
            push_back_buffer(&val_buffer, &n_values, value, &series, &n_trials);
        } else if (evid > 0) {
                // Normal fetch
                if (!event_triggered) {
                    // No activity on event of interest registered -> skip
                    new = 1;
                    continue;
                }
                if (new) {
                    new = 0;
                    n_values = 0;
                    push_back_pointer(&series, &n_trials, NULL);
                }
                push_back(&series[n_trials-1], &n_values, value);
        } else {
            // Inverse fetch
            if (event_triggered && val_buffer != NULL) {
                // Overlapped with event subsequence -- drop data.
                // We'd be better off with circular buffer, 
                // but this will quite suffice for current usecase
                n_values = 0;
                free(val_buffer);
                val_buffer = NULL;
                continue;
            }
            push_back_buffer(&val_buffer, &n_values, value, &series, &n_trials);
        }
    }
    if (val_buffer != NULL) free(val_buffer);
    fclose(dataf);
    fclose(eventsf);
    return (struct session) {series, n_trials};
}

struct sax_signal_set fetch_sax_signals(int pid, int evid, int w, int c) {
    sax_signal *series = NULL;
    size_t n_series = 0;
    for (size_t sid = 0; sid < NSESSIONS; ++sid) {
        for (size_t chid = 0; chid < NCHANNELS; ++chid) {
            struct session trials = 
                fetch_session_data(pid, chid, sid, evid);
            if (trials.series == NULL) {
                printf("I/O problems occured, exiting\n");
                exit(-1);
            }
            if (chid == 0) {
                n_series += trials.n_series; 
                series = safe_realloc(series, n_series * sizeof *series);
            }
            for (size_t trid = 0; trid < trials.n_series; ++trid) {
                // SAX conversion is defined for even partitioning only
                series[n_series - trid - 1][chid] = 
                    sts_to_iSAX(trials.series[trid], EVLEN - (EVLEN % w), w, c);
                free(trials.series[trid]);
            }
            free(trials.series);
        }
    }
    return (struct sax_signal_set) {series, n_series};
}

static double signal_dist(size_t n_dim, sax_word signal1[n_dim], sax_word signal2[n_dim], 
        size_t n, size_t w, unsigned int c) {
    double dist = 0, idist;
    for (size_t i = 0; i < n_dim; ++i) {
        idist = sts_mindist(signal1[i], signal2[i], n, w, c);
        dist += idist * idist;
    }
    return sqrt(dist);
}

double ndim_mindist(int n_dim, sax_word a[n_dim], sax_word (*events)[n_dim], size_t n_events, int n, int w, int c, size_t skipid) {
    double mindist = DBL_MAX;
    for (size_t trid = 0; trid < n_events; ++trid) {
        // since dist(x, x) == 0
        if (trid == skipid) continue;
        double dist = 
            signal_dist(n_dim, a, events[trid], n, w, c);
        if (mindist > dist) mindist = dist;
    }
    return mindist;
}

double* evaluate_mindist(struct sax_signal_set A, struct sax_signal_set B, size_t n, int w, int c) {
    int inner = (A.series == B.series) && (A.n_series == B.n_series);
    double *mindists = safe_malloc(A.n_series * sizeof (double));
    for (size_t trid = 0; trid < A.n_series; ++trid) {
        mindists[trid] = 
            ndim_mindist(NCHANNELS, A.series[trid], B.series, B.n_series, 
                    n, w, c, inner ? trid : INT_MAX);
    }
    return mindists;
}

double find_max(double *arr, size_t size) {
    double max = 0;
    for (size_t i = 0; i < size; ++i) {
        if (arr[i] > max) max = arr[i];
    }
    return max;
}

double find_min(double *arr, size_t size) {
    double min = DBL_MAX;
    for (size_t i = 0; i < size; ++i) {
        if (arr[i] < min) min = arr[i];
    }
    return min;
}

void dump_session_data(struct session data, size_t *series_sizes, char *prefix) {
    char buf[BUF_SIZE];
    for (size_t trid = 0; trid < data.n_series; ++trid) {
        snprintf(buf, BUF_SIZE, "plot/%s%zu", prefix, trid + 1);
        FILE *out = fopen(buf, "w");
        for (size_t frameid = 0; frameid < series_sizes[trid]; ++frameid) {
            fprintf(out, "%.2f,%lf\n", 
                    (float) frameid, data.series[trid][frameid]);
        }
        fclose(out);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("No option specified, exiting.\n(Should be one of: plot, dist)\n");
        exit(-1);
    }
    if (argc < 4) {
        printf("Provide me with subject id and event number\n");
        exit(-1);
    }
    int pid = atoi(argv[2]), evid = atoi(argv[3]);
    // The rest of the program assumes 1-based event ids
    evid++;
    if (argc < 6) {
        printf("Provide me with w and c\n");
        exit(-1);
    }
    int w = atoi(argv[4]);
    int c = atoi(argv[5]);

    if (strcmp("plot", argv[1]) == 0) {
        if (argc < 7) {
            printf("Provide me with channel number\n");
            exit(-1);
        }
        int chid = atoi(argv[6]);

        sax_word (*events)[1] = NULL;
        size_t n_events = 0;
        for (size_t sid = 0; sid < NSESSIONS; ++sid) {
            struct session event = fetch_session_data(pid, chid, sid, evid);
            n_events += event.n_series;
            events = safe_realloc(events, n_events * sizeof(sax_word[1]));
            for (size_t frameid = 0; frameid < event.n_series; ++frameid) {
                events[n_events - frameid - 1][0] = 
                    sts_to_iSAX(event.series[frameid], EVLEN - (EVLEN % w), w, c);
                free(event.series[frameid]);
            }
            free(event.series);
        }
        struct session dist_plot;
        dist_plot.n_series = NSESSIONS;
        dist_plot.series = malloc(NSESSIONS * sizeof(double *));
        size_t series_sizes[NSESSIONS];
        for (size_t sid = 0; sid < NSESSIONS; ++sid) {
            struct session channel = fetch_session_data(pid, chid, sid, 0);
            series_sizes[sid] = channel.n_series;
            dist_plot.series[sid] = malloc(channel.n_series * sizeof(double));
            for (size_t frameid = 0; frameid < channel.n_series; ++frameid) {
                sax_word frame[1] = 
                    {sts_to_iSAX(channel.series[frameid], EVLEN - (EVLEN % w), w, c)};
                dist_plot.series[sid][frameid] = 
                    ndim_mindist(1, frame, events, n_events, EVLEN, w, c, INT_MAX);
                free(channel.series[frameid]);
                free(frame[0]);
            }
            free(channel.series);
        }
        char name_buf[BUF_SIZE];
        snprintf(name_buf, BUF_SIZE, "dist_plot_%d_%d_%d_%d_", pid, evid, chid, w);
        dump_session_data(dist_plot, series_sizes, name_buf);
    } else {
        struct sax_signal_set match_trials = fetch_sax_signals(pid, evid, w, c);
        struct sax_signal_set nonmatch_trials = fetch_sax_signals(pid, -evid, w, c);
        double* match_mindists = 
            evaluate_mindist(match_trials, match_trials, EVLEN, w, c);
        double* nonmatch_mindists = 
            evaluate_mindist(nonmatch_trials, match_trials, EVLEN, w, c);
        double max_match_mindist = find_max(match_mindists, match_trials.n_series);
        double min_nonmatch_mindist = find_min(nonmatch_mindists, nonmatch_trials.n_series);
        printf("\n-- Overall: Maximum matching mindist == %lf VS \
                Minimum nonmatching mindist == %lf --\n", max_match_mindist, min_nonmatch_mindist);
    }
}
