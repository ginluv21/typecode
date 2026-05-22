#ifndef LESSON_H
#define LESSON_H

#include <time.h>
#include "settings.h"
#include "config.h"

typedef PracticeMode LessonMode;

static inline int   metrics_calc_wpm(int correct, double elapsed_sec)
{
    double mins = elapsed_sec / 60.0;
    return (mins > 0) ? (int)((correct / 5.0) / mins) : 0;
}

static inline float metrics_calc_accuracy(int correct, int errors)
{
    int total = correct + errors;
    return (total > 0) ? (correct * 100.0f / total) : 100.0f;
}

#define LESSON_NAME_MAX 128
#define MAX_LESSONS 64
#define LESSON_PATH_MAX 512

typedef enum {
    CHAR_UNTYPED = 0,
    CHAR_CORRECT,
    CHAR_WRONG
} CharState;

typedef struct {
    char name[LESSON_NAME_MAX];   /* название (из имени файла) */
    char path[LESSON_PATH_MAX];   /* полный путь к файлу */
} LessonEntry;

typedef struct {
    LessonEntry entries[MAX_LESSONS];
    int count;
} LessonList;

typedef struct {
    int             correct;
    int             errors;
    int             wpm;
    float           accuracy;
    int             duration_sec;
    struct timespec start;
    int             started;
} Metrics;

typedef enum {
    RESULT_LESSONS = 0,
    RESULT_REPEAT,
    RESULT_MAIN_MENU
} ResultAction;

typedef struct {
    char *text;
    int   len;
    char  name[LESSON_NAME_MAX];
} Lesson;

double       elapsed_sec(const Metrics *m);
void         metrics_update(Metrics *m);

Lesson      *lesson_load(const char *path);
void         lesson_free(Lesson *lesson);
void         lesson_draw(const Lesson *lesson, int cursor_pos,
                         const CharState *states, const Metrics *metrics);
ResultAction lesson_show_results(const Metrics *metrics, const char *name);
ResultAction lesson_run(const char *path, const Settings *s);
ResultAction lesson_run_with_name(const char *path, const char *display_name, const Settings *s);
void         lesson_run_text(const char *text, const char *name, int hardcore, LessonMode mode, int time_limit);
void         lesson_select_menu(const char *dir, const Settings *s, const AppConfig *cfg);

LessonList  *lessons_scan(const char *dir);
void         lessons_free(LessonList *list);
int          lessons_run_menu(const char *dir, const Settings *s, const AppConfig *cfg);
const char  *language_select_menu(const AppConfig *cfg);


#endif
