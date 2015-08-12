/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>

#include "debug.h"

#define BUF_SIZE 10000

#define NCHANNELS 32
#define NSESSIONS 8
#define NEVENTS 6
#define EVLEN 150

typedef double dsignal[NCHANNELS];
typedef sts_word sax_signal[NCHANNELS];

typedef struct sax_signal_set {
    sax_signal *words;
    size_t n_words;
} sax_signal_set;

typedef struct session {
    dsignal *values;
    short *event_markers;
    size_t n_values;
} session;

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

/* Yep, these 4 could be macroses, 
 * but one does not simply use them to mimic generics in C */
void push_back_double(double* *series, size_t* size, double value) {
    *series = safe_realloc(*series, (++(*size)) * sizeof **series);
    (*series)[(*size) - 1] = value;
}

void push_back_short(short* *series, size_t* size, short value) {
    *series = safe_realloc(*series, (++(*size)) * sizeof **series);
    (*series)[(*size) - 1] = value;
}

void push_back_signal(dsignal* *series, size_t* size, dsignal value) {
    *series = safe_realloc(*series, (++(*size)) * sizeof **series);
    memcpy((*series)[(*size) - 1], value, NCHANNELS * sizeof(double));
}

void push_back_sax_signal(sax_signal* *series, size_t* size, sax_signal value) {
    *series = safe_realloc(*series, (++(*size)) * sizeof **series);
    memcpy((*series)[(*size) - 1], value, NCHANNELS * sizeof(sts_word));
}

session fetch_session_data(int pid, int sid, bool train) {
    char buf[BUF_SIZE];
    char *prefix = train ? "data/train" : "data/test";
    FILE *dataf, *eventsf = NULL;
    snprintf(buf, BUF_SIZE, 
            "%s/subj%d_series%d_data.csv", prefix, pid + 1, sid + 1);
    dataf = fopen(buf, "r");
    if (train) {
        snprintf(buf, BUF_SIZE, 
                "%s/subj%d_series%d_events.csv", prefix, pid + 1, sid + 1);
        eventsf = fopen(buf, "r");
    }
    if (!dataf || (train && !eventsf)) return (session) {NULL, NULL, 0};
    // Eat-up both csv headers
    if (fgets(buf, BUF_SIZE, dataf) == NULL) return (session) {NULL, NULL, 0};
    if (train && fgets(buf, BUF_SIZE, eventsf) == NULL) return (session) {NULL, NULL, 0};

    session data = (session) {NULL, NULL, 0};
    while (fgets(buf, BUF_SIZE, dataf)) {
        // Parse all channel data
        char *tmp = strdup(buf);
        dsignal sig;
        size_t i = 0;
        for (char *tok = strtok(tmp, ","); tok && *tok; tok = strtok(NULL, ",\n")) {
            if (i++ == 0) continue;
            double value = strtod(tok, NULL);
            sig[i-2] = value;
        }
        push_back_signal(&data.values, &data.n_values, sig);
        free(tmp);

        if (train) {
            // Parse event type
            if (fgets(buf, BUF_SIZE, eventsf) == NULL) return (session) {NULL, NULL, 0};
            tmp = strdup(buf);
            short event_type = 0;
            i = 0;
            for (char *tok = strtok(tmp, ","); tok && *tok; tok = strtok(NULL, ",\n")) {
                if (strcmp(tok, "1") == 0) {
                    event_type = i;
                    break;
                }
                ++i;
            }
            --data.n_values;
            push_back_short(&data.event_markers, &data.n_values, event_type);
            free(tmp);
        }
    }
    fclose(dataf);
    fclose(eventsf);
    return data;
}

session *fetch_train_data(int pid) { 
    session *train = malloc(NSESSIONS * sizeof *train);
    for (size_t sid = 0; sid < NSESSIONS; ++sid) {
        train[sid] = fetch_session_data(pid, sid, true);
        if (train[sid].values == NULL) {
            printf("I/O problems occured, exiting\n");
            exit(-1);
        }
    }
    return train;
}

/*
 * @param evid: 1-based id of the event. 
 * If evid is negative than each subrange not corresponding to event #evid 
 * of length EVLEN will be returned
 * If evid is zero, than all session subsequences of length EVLEN will be returned
 */
struct sax_signal_set 
session_to_event_words(session *data, size_t n_sessions, short evid, int w, int c) {
    sax_signal *words = NULL;
    size_t n_words = 0;
    sts_window sax_window[NCHANNELS];
    for (size_t chid = 0; chid < NCHANNELS; ++chid)
       sax_window[chid] = sts_new_window(EVLEN, w, c);

    for (size_t sid = 0; sid < n_sessions; ++sid) {
        if (evid != 0 && data[sid].event_markers == NULL) {
            printf("No event_markers provided for marker-based fetch");
            exit(-1);
        }
        for (size_t chid = 0; chid < NCHANNELS; ++chid)
            sts_window_reset(sax_window[chid]);
        for (size_t trid = 0; trid < data[sid].n_values; ++trid) {
            sax_signal sig;
            bool do_push = false;
            for (size_t chid = 0; chid < NCHANNELS; ++chid) {
                double value = data[sid].values[trid][chid];
                sts_word word = NULL;
                if (evid == 0) {
                    word = sts_append_value(sax_window[chid], value);
                } else {
                    bool event_triggered = (data[sid].event_markers[trid] == abs(evid));
                    if ((event_triggered && evid > 0) ||
                        (!event_triggered && evid < 0)) {
                        word = sts_append_value(sax_window[chid], value);
                    } else {
                        sts_window_reset(sax_window[chid]);
                    }
                }
                if (word != NULL) {
                    do_push = true;
                    sig[chid] = word;
                }
            }
            if (do_push) {
                push_back_sax_signal(&words, &n_words, sig);
            }
        }
    }
    for (size_t chid = 0; chid < NCHANNELS; ++chid)
        sts_free_window(sax_window[chid]);
    return (sax_signal_set) {words, n_words};
}

double signal_dist(size_t n_dim, sts_word signal1[n_dim], sts_word signal2[n_dim]) {
    double dist = 0, idist;
    for (size_t i = 0; i < n_dim; ++i) {
        idist = sts_mindist(signal1[i], signal2[i]);
        dist += idist * idist;
    }
    return sqrt(dist);
}

double ndim_mindist(int n_dim, sts_word a[n_dim], sts_word (*events)[n_dim], 
        size_t n_events, size_t skipid) {
    double mindist = DBL_MAX;
    for (size_t trid = 0; trid < n_events; ++trid) {
        // since dist(x, x) == 0
        if (trid == skipid) continue;
        double dist = 
            signal_dist(n_dim, a, events[trid]);
        if (mindist > dist) mindist = dist;
    }
    return mindist;
}

double* evaluate_mindist(struct sax_signal_set A, struct sax_signal_set B) {
    int inner = (A.words == B.words) && (A.n_words == B.n_words);
    double *mindists = safe_malloc(A.n_words * sizeof *mindists);
    for (size_t trid = 0; trid < A.n_words; ++trid) {
        mindists[trid] = 
            ndim_mindist(NCHANNELS, A.words[trid], B.words, B.n_words, inner ? trid : INT_MAX);
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
        char buf[BUF_SIZE];

        session *data = fetch_train_data(pid);
        sax_signal_set ndim_events = session_to_event_words(data, NSESSIONS, evid, w, c);
        sts_word (*events)[1] = malloc(ndim_events.n_words * sizeof *events);
        size_t n_events = ndim_events.n_words;
        for (size_t i = 0; i < n_events; ++i) {
            events[i][0] = ndim_events.words[i][chid];
            for (int j = 0; j < NCHANNELS; ++j) {
                if (j != chid) sts_free_word(ndim_events.words[i][j]);
            }
        }

        for (size_t sid = 0; sid < NSESSIONS; ++sid) {
            snprintf(buf, BUF_SIZE, 
                    "plot/dist_plot_%d_%d_%d_%d_%d_%zu", pid, evid, chid, w, c, sid + 1);
            FILE *out = fopen(buf, "w");
            if (out == NULL) {
                printf("IO Error\n");
                exit(-1);
            }

            sax_signal_set all = session_to_event_words(data + sid, 1, 0, w, c);
            for (size_t frameid = 0; frameid < all.n_words; ++frameid) {
                sts_word frame[1] = {all.words[frameid][chid]};
                double dist = ndim_mindist(1, frame, events, n_events, INT_MAX);

                fprintf(out, "%.2f,%lf\n", (float) frameid, dist);
                for (size_t i = 0; i < NCHANNELS; ++i) {
                    sts_free_word(all.words[frameid][i]);
                }
            }
            fclose(out);
        }
    } else {
        session *train = fetch_train_data(pid);
        struct sax_signal_set match_trials = 
            session_to_event_words(train, NSESSIONS, evid, w, c);
        struct sax_signal_set nonmatch_trials = 
            session_to_event_words(train, NSESSIONS, -evid, w, c);
        double* match_mindists = 
            evaluate_mindist(match_trials, match_trials);
        double* nonmatch_mindists = 
            evaluate_mindist(nonmatch_trials, match_trials);
        double max_match_mindist = find_max(match_mindists, match_trials.n_words);
        double min_nonmatch_mindist = find_min(nonmatch_mindists, nonmatch_trials.n_words);
        printf("\n-- Overall: Maximum matching mindist == %lf VS \
                Minimum nonmatching mindist == %lf --\n", max_match_mindist, min_nonmatch_mindist);
    }
}
