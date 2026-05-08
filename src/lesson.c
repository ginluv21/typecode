#define _POSIX_C_SOURCE 200809L
THIS_IS_A_TEST_ERROR
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "lesson.h"
#include "typecode.h"

#define TEXT_MARGIN 3

Lesson *lesson_load(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    if (size <= 0) { fclose(f); return NULL; }

    Lesson *lesson = malloc(sizeof(Lesson));
    if (!lesson) { fclose(f); return NULL; }

    lesson->text = malloc(size + 1);
    if (!lesson->text) { free(lesson); fclose(f); return NULL; }

    size_t n = fread(lesson->text, 1, size, f);
    lesson->text[n] = '\0';
    lesson->len = (int)n;
    fclose(f);

    /* name from filename, without extension */
    const char *slash = strrchr(path, '/');
    const char *base  = slash ? slash + 1 : path;
    strncpy(lesson->name, base, LESSON_NAME_MAX - 1);
    lesson->name[LESSON_NAME_MAX - 1] = '\0';
    char *dot = strrchr(lesson->name, '.');
    if (dot) *dot = '\0';
    for (char *p = lesson->name; *p; p++)
        if (*p == '-' || *p == '_') *p = ' ';

    return lesson;
}

void lesson_free(Lesson *lesson)
{
    if (!lesson) return;
    free(lesson->text);
    free(lesson);
}

static double elapsed_sec(const Metrics *m)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec - m->start.tv_sec)
         + (now.tv_nsec - m->start.tv_nsec) / 1e9;
}

static void metrics_update(Metrics *m)
{
    if (!m->started) return;
    double mins = elapsed_sec(m) / 60.0;
    m->wpm      = (mins > 0) ? (int)((m->correct / 5.0) / mins) : 0;
    int total   = m->correct + m->errors;
    m->accuracy = (total > 0) ? (m->correct * 100.0f / total) : 100.0f;
}

void lesson_draw(const Lesson *lesson, int cursor_pos,
                 const CharState *states, const Metrics *metrics)
{
    int max_col  = COLS  - TEXT_MARGIN;
    int max_row  = LINES - 3;
    int text_row = 3;
    int text_col = TEXT_MARGIN;

    clear();

    attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
    mvhline(0, 0, ACS_HLINE, COLS);
    mvprintw(1, TEXT_MARGIN, "%s", lesson->name);
    mvhline(2, 0, ACS_HLINE, COLS);
    attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

    /* строка метрик внизу */
    if (metrics && metrics->started) {
        attron(COLOR_PAIR(COLOR_GREEN_ON_BLACK) | A_BOLD);
        mvprintw(LINES - 1, TEXT_MARGIN, "WPM: %d", metrics->wpm);
        attroff(COLOR_PAIR(COLOR_GREEN_ON_BLACK) | A_BOLD);

        attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        mvprintw(LINES - 1, TEXT_MARGIN + 10, "Accuracy: %.0f%%", metrics->accuracy);
        attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));

        attron(COLOR_PAIR(COLOR_RED_ON_BLACK) | A_BOLD);
        mvprintw(LINES - 1, TEXT_MARGIN + 24, "Errors: %d", metrics->errors);
        attroff(COLOR_PAIR(COLOR_RED_ON_BLACK) | A_BOLD);

        attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        mvprintw(LINES - 1, COLS - 20, "Esc - back to menu");
        attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
    } else {
        attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        mvprintw(LINES - 1, TEXT_MARGIN, "Esc - back to menu");
        attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
    }

    for (int i = 0; i < lesson->len && text_row < max_row; i++) {
        char ch = lesson->text[i];

        if (ch == '\n') {
            text_row++;
            text_col = TEXT_MARGIN;
            continue;
        }

        if (text_col >= max_col) {
            text_row++;
            text_col = TEXT_MARGIN;
            if (text_row >= max_row) break;
        }

        if (i == cursor_pos) {
            attron(COLOR_PAIR(COLOR_YELLOW_ON_BLACK) | A_BOLD | A_REVERSE);
            mvaddch(text_row, text_col, (unsigned char)ch);
            attroff(COLOR_PAIR(COLOR_YELLOW_ON_BLACK) | A_BOLD | A_REVERSE);
        } else if (states && states[i] == CHAR_CORRECT) {
            attron(COLOR_PAIR(COLOR_GREEN_ON_BLACK));
            mvaddch(text_row, text_col, (unsigned char)ch);
            attroff(COLOR_PAIR(COLOR_GREEN_ON_BLACK));
        } else if (states && states[i] == CHAR_WRONG) {
            attron(COLOR_PAIR(COLOR_RED_ON_BLACK) | A_BOLD);
            mvaddch(text_row, text_col, (unsigned char)ch);
            attroff(COLOR_PAIR(COLOR_RED_ON_BLACK) | A_BOLD);
        } else {
            attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
            mvaddch(text_row, text_col, (unsigned char)ch);
            attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        }

        text_col++;
    }

    refresh();
}

void lesson_run(const char *path)
{
    Lesson *lesson = lesson_load(path);
    if (!lesson) return;

    CharState *states = calloc(lesson->len, sizeof(CharState));
    if (!states) { lesson_free(lesson); return; }

    Metrics metrics = {0};
    int cursor_pos  = 0;
    lesson_draw(lesson, cursor_pos, states, &metrics);

    int ch;
    while ((ch = getch()) != 27) { /* Esc */
        if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
            if (cursor_pos > 0) {
                cursor_pos--;
                if (states[cursor_pos] == CHAR_CORRECT) metrics.correct--;
                else if (states[cursor_pos] == CHAR_WRONG) metrics.errors--;
                states[cursor_pos] = CHAR_UNTYPED;
            }
        } else if (ch == KEY_RESIZE) {
            /* перерисовать при resize */
        } else if (ch >= 32 && ch < 127) {
            /* запустить таймер при первом нажатии */
            if (!metrics.started) {
                clock_gettime(CLOCK_MONOTONIC, &metrics.start);
                metrics.started = 1;
            }
            if (cursor_pos < lesson->len) {
                if (ch == lesson->text[cursor_pos]) {
                    states[cursor_pos] = CHAR_CORRECT;
                    metrics.correct++;
                } else {
                    states[cursor_pos] = CHAR_WRONG;
                    metrics.errors++;
                }
                cursor_pos++;
            }
            if (cursor_pos == lesson->len)
                break;
        }

        metrics_update(&metrics);
        lesson_draw(lesson, cursor_pos, states, &metrics);
    }

    free(states);
    lesson_free(lesson);
}
