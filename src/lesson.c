#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

void lesson_draw(const Lesson *lesson, int cursor_pos, const CharState *states)
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

    attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
    mvprintw(LINES - 1, TEXT_MARGIN, "Esc - back to menu");
    attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));

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

    int cursor_pos = 0;
    lesson_draw(lesson, cursor_pos, states);

    int ch;
    while ((ch = getch()) != 27) { /* Esc */
        if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
            if (cursor_pos > 0) {
                cursor_pos--;
                states[cursor_pos] = CHAR_UNTYPED;
            }
        } else if (ch == KEY_RESIZE) {
            /* перерисовать при resize */
        } else if (ch >= 32 && ch < 127) {
            if (cursor_pos < lesson->len) {
                states[cursor_pos] = (ch == lesson->text[cursor_pos])
                                     ? CHAR_CORRECT : CHAR_WRONG;
                cursor_pos++;
            }
            if (cursor_pos == lesson->len)
                break;
        }

        lesson_draw(lesson, cursor_pos, states);
    }

    free(states);
    lesson_free(lesson);
}
