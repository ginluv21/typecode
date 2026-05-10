#ifndef LESSON_H
#define LESSON_H

#include <time.h>

#define LESSON_NAME_MAX 128
#define MAX_LESSONS 64
#define LESSON_PATH_MAX 512

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

typedef struct {
    char name[LESSON_NAME_MAX];   /* название (из имени файла) */
    char path[LESSON_PATH_MAX];   /* полный путь к файлу */
} LessonEntry;

typedef struct {
    LessonEntry entries[MAX_LESSONS];
    int count;
} LessonList;

Lesson      *lesson_load(const char *path);
void         lesson_free(Lesson *lesson);
void         lesson_draw(const Lesson *lesson, int cursor_pos,
                         const CharState *states, const Metrics *metrics);
ResultAction lesson_show_results(const Metrics *metrics, const char *name);
ResultAction lesson_run(const char *path);
void         lesson_select_menu(const char *dir);

LessonList *lessons_scan(const char *dir);
void        lessons_free(LessonList *list);
int         lessons_run_menu(const char *dir);  /* показать меню и запустить выбранный */

#endif
