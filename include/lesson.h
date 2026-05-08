#ifndef LESSON_H
#define LESSON_H

#include <time.h>

#define LESSON_NAME_MAX 128

typedef enum {
    CHAR_UNTYPED = 0,
    CHAR_CORRECT,
    CHAR_WRONG
} CharState;

typedef struct {
    int             correct;
    int             errors;
    int             wpm;
    float           accuracy;
    struct timespec start;
    int             started;
} Metrics;

typedef struct {
    char *text;
    int   len;
    char  name[LESSON_NAME_MAX];
} Lesson;

Lesson *lesson_load(const char *path);
void    lesson_free(Lesson *lesson);
void    lesson_draw(const Lesson *lesson, int cursor_pos,
                    const CharState *states, const Metrics *metrics);
void    lesson_run(const char *path);

#endif
