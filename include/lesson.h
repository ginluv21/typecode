#ifndef LESSON_H
#define LESSON_H

#define LESSON_NAME_MAX 128

typedef struct {
    char *text;
    int   len;
    char  name[LESSON_NAME_MAX];
} Lesson;

Lesson *lesson_load(const char *path);
void    lesson_free(Lesson *lesson);
void    lesson_draw(const Lesson *lesson, int cursor_pos);
void    lesson_run(const char *path);

#endif
