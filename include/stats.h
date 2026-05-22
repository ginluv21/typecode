#ifndef STATS_H
#define STATS_H

#define STATS_DIR  "/.typecode"
#define STATS_FILE "/.typecode/stats.txt"
#define LESSON_MAX 128

typedef struct {
    long  timestamp;
    int   wpm;
    float accuracy;
    int   errors;
    int   duration_sec;
    char  lesson[LESSON_MAX];
} SessionResult;

#include "config.h"

void  stats_ensure_dir(void);
void  stats_save(const SessionResult *r);
int   stats_load(SessionResult *out, int max);
void  stats_reset(void);
int   stats_best_wpm(void);
float stats_avg_wpm(int last_n);
void  stats_draw_screen(const AppConfig *cfg);

#endif
