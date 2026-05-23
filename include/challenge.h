#ifndef CHALLENGE_H
#define CHALLENGE_H

#include <time.h>
#include "lesson.h"
#include "config.h"

typedef struct {
    int round;
    int min_wpm;
    int time_sec;
    const char *lesson_path;
} ChallengeRound;

typedef struct {
    int wpm;
    time_t timestamp;
} ChallengeRecord;

void challenge_run(const AppConfig *cfg);
int  challenge_load_records(ChallengeRecord out[], int max);

#endif
