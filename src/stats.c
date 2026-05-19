#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "stats.h"

#define LOAD_MAX 1024

static void build_path(char *buf, int size, const char *suffix)
{
    const char *home = getenv("HOME");
    snprintf(buf, size, "%s%s", home ? home : "", suffix);
}

void stats_ensure_dir(void)
{
    char path[512];
    build_path(path, sizeof(path), STATS_DIR);
    mkdir(path, 0755);
}

void stats_save(const SessionResult *r)
{
    char path[512];
    build_path(path, sizeof(path), STATS_FILE);
    FILE *f = fopen(path, "a");
    if (!f) return;
    fprintf(f, "%ld|%d|%.1f|%d|%d|%s\n",
            r->timestamp, r->wpm, r->accuracy,
            r->errors, r->duration_sec, r->lesson);
    fclose(f);
}

int stats_load(SessionResult *out, int max)
{
    char path[512];
    build_path(path, sizeof(path), STATS_FILE);
    FILE *f = fopen(path, "r");
    if (!f) return 0;

    char line[512];
    int count = 0;
    while (count < max && fgets(line, sizeof(line), f)) {
        SessionResult r = {0};
        if (sscanf(line, "%ld|%d|%f|%d|%d|%127[^\n]",
                   &r.timestamp, &r.wpm, &r.accuracy,
                   &r.errors, &r.duration_sec, r.lesson) == 6) {
            out[count++] = r;
        }
    }
    fclose(f);
    return count;
}

int stats_best_wpm(void)
{
    SessionResult buf[LOAD_MAX];
    int n = stats_load(buf, LOAD_MAX);
    int best = 0;
    for (int i = 0; i < n; i++)
        if (buf[i].wpm > best) best = buf[i].wpm;
    return best;
}

float stats_avg_wpm(int last_n)
{
    SessionResult buf[LOAD_MAX];
    int n = stats_load(buf, LOAD_MAX);
    if (n == 0) return 0.0f;
    int from = (n > last_n) ? n - last_n : 0;
    float sum = 0;
    for (int i = from; i < n; i++) sum += buf[i].wpm;
    return sum / (n - from);
}
