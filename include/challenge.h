#ifndef CHALLENGE_H
#define CHALLENGE_H

#include "lesson.h"
#include "config.h"

typedef struct {
    int round;
    int min_wpm;
    int time_sec;
    const char *lesson_path;
} ChallengeRound;

void challenge_run(const AppConfig *cfg);

#endif
