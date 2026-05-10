#define _POSIX_C_SOURCE 200809L
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include "lesson.h"
#include "typecode.h"

#define TEXT_MARGIN 3

Lesson *lesson_load(const char *path) // читает .txt файл, выделяет Lesson с текстом, имя из имени файла без расширения
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

    // имя из имени файла без расширения
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

void lesson_free(Lesson *lesson) // освобождает text и сам Lesson
{
    if (!lesson) return;
    free(lesson->text);
    free(lesson);
}

static double elapsed_sec(const Metrics *m) // секунды с момента старта урока
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec - m->start.tv_sec)
         + (now.tv_nsec - m->start.tv_nsec) / 1e9;
}

static void metrics_update(Metrics *m) // пересчитывает wpm и accuracy по текущему elapsed
{
    if (!m->started) return;
    double mins = elapsed_sec(m) / 60.0;
    m->wpm      = (mins > 0) ? (int)((m->correct / 5.0) / mins) : 0;
    int total   = m->correct + m->errors;
    m->accuracy = (total > 0) ? (m->correct * 100.0f / total) : 100.0f;
}

void lesson_draw(const Lesson *lesson, int cursor_pos,
                 const CharState *states, const Metrics *metrics) // рисует заголовок, текст с цветами по состояниям, строку метрик внизу
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

    // строка метрик внизу
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
        mvprintw(LINES - 1, COLS - 22, "Esc - finish early");
        attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
    } else {
        attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        mvprintw(LINES - 1, TEXT_MARGIN, "Esc - finish early");
        attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
    }

    for (int i = 0; i < lesson->len && text_row < max_row; i++) {
        char ch = lesson->text[i];

        if (ch == '\n') {
            text_row++;
            text_col = TEXT_MARGIN;
            continue;
        }

        if (ch == '\t') {
            // таб-стоп: до следующей позиции кратной 4
            int tab_stop = TEXT_MARGIN + ((text_col - TEXT_MARGIN + 4) / 4) * 4;
            if (i == cursor_pos) {
                attron(COLOR_PAIR(COLOR_YELLOW_ON_BLACK) | A_BOLD | A_REVERSE);
                mvaddch(text_row, text_col, '>');
                attroff(COLOR_PAIR(COLOR_YELLOW_ON_BLACK) | A_BOLD | A_REVERSE);
            } else if (states && states[i] == CHAR_CORRECT) {
                attron(COLOR_PAIR(COLOR_GREEN_ON_BLACK));
                for (int t = text_col; t < tab_stop; t++) mvaddch(text_row, t, ' ');
                attroff(COLOR_PAIR(COLOR_GREEN_ON_BLACK));
            } else if (states && states[i] == CHAR_WRONG) {
                attron(COLOR_PAIR(COLOR_RED_ON_BLACK) | A_BOLD);
                mvaddch(text_row, text_col, '>');
                attroff(COLOR_PAIR(COLOR_RED_ON_BLACK) | A_BOLD);
            } else {
                attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
                mvaddch(text_row, text_col, '>');
                attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
            }
            text_col = tab_stop;
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

ResultAction lesson_show_results(const Metrics *metrics, const char *name) // показывает итоговую рамку со статистикой, ждёт R/Esc/Q
{
    int w = 50;
    int h = 12;
    int row = (LINES - h) / 2;
    int col = (COLS  - w) / 2;

    int mm = metrics->duration_sec / 60;
    int ss = metrics->duration_sec % 60;

    clear();

    attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
    mvaddch(row, col, ACS_ULCORNER);
    mvhline(row, col + 1, ACS_HLINE, w - 2);
    mvaddch(row, col + w - 1, ACS_URCORNER);
    attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

    for (int i = 1; i < h - 1; i++) {
        attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
        mvaddch(row + i, col, ACS_VLINE);
        mvaddch(row + i, col + w - 1, ACS_VLINE);
        attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
    }

    attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
    mvaddch(row + h - 1, col, ACS_LLCORNER);
    mvhline(row + h - 1, col + 1, ACS_HLINE, w - 2);
    mvaddch(row + h - 1, col + w - 1, ACS_LRCORNER);
    attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

    // заголовок
    attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK) | A_BOLD);
    mvprintw(row + 1, col + (w - (int)strlen(name)) / 2, "%s", name);
    attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK) | A_BOLD);

    // разделитель
    attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
    mvaddch(row + 2, col, ACS_LTEE);
    mvhline(row + 2, col + 1, ACS_HLINE, w - 2);
    mvaddch(row + 2, col + w - 1, ACS_RTEE);
    attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

    // метрики
    int mc = col + 6;
    int vc = col + 22;

    attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
    mvprintw(row + 4, mc, "WPM");
    mvprintw(row + 5, mc, "Accuracy");
    mvprintw(row + 6, mc, "Errors");
    mvprintw(row + 7, mc, "Time");
    attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));

    attron(COLOR_PAIR(COLOR_GREEN_ON_BLACK) | A_BOLD);
    mvprintw(row + 4, vc, "%d", metrics->wpm);
    attroff(COLOR_PAIR(COLOR_GREEN_ON_BLACK) | A_BOLD);

    attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
    mvprintw(row + 5, vc, "%.0f%%", metrics->accuracy);
    attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));

    attron(COLOR_PAIR(COLOR_RED_ON_BLACK) | A_BOLD);
    mvprintw(row + 6, vc, "%d", metrics->errors);
    attroff(COLOR_PAIR(COLOR_RED_ON_BLACK) | A_BOLD);

    attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
    mvprintw(row + 7, vc, "%d:%02d", mm, ss);
    attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));

    // разделитель
    attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
    mvaddch(row + h - 3, col, ACS_LTEE);
    mvhline(row + h - 3, col + 1, ACS_HLINE, w - 2);
    mvaddch(row + h - 3, col + w - 1, ACS_RTEE);
    attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

    // кнопки действий
    attron(COLOR_PAIR(COLOR_GREEN_ON_BLACK) | A_BOLD);
    mvprintw(row + h - 2, col + 4, "[R] Retry");
    attroff(COLOR_PAIR(COLOR_GREEN_ON_BLACK) | A_BOLD);

    attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
    mvprintw(row + h - 2, col + 18, "[Esc] Lessons");
    attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

    attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
    mvprintw(row + h - 2, col + 36, "[Q] Main menu");
    attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));

    refresh();

    int ch;
    while ((ch = getch()) != ERR) {
        if (ch == 'r' || ch == 'R')                       return RESULT_REPEAT;
        if (ch == 27 || ch == '\n' || ch == KEY_ENTER)    return RESULT_LESSONS;
        if (ch == 'q' || ch == 'Q')                       return RESULT_MAIN_MENU;
    }
    return RESULT_LESSONS;
}

ResultAction lesson_run(const char *path) // основной цикл урока: ввод символов, подсчёт метрик, повтор при R
{
    Lesson *lesson = lesson_load(path);
    if (!lesson) return RESULT_LESSONS;

    CharState *states = calloc(lesson->len, sizeof(CharState));
    if (!states) { lesson_free(lesson); return RESULT_LESSONS; }

    ResultAction action;
    do {
        memset(states, 0, lesson->len * sizeof(CharState));
        Metrics metrics = {0};
        int cursor_pos  = 0;

        // пропустить ведущие переносы строк
        while (cursor_pos < lesson->len && lesson->text[cursor_pos] == '\n') {
            states[cursor_pos] = CHAR_CORRECT;
            cursor_pos++;
        }

        lesson_draw(lesson, cursor_pos, states, &metrics);

        int ch;
        while ((ch = getch()) != ERR) {
            if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
                // пропустить переносы при откате
                while (cursor_pos > 0 && lesson->text[cursor_pos - 1] == '\n') {
                    cursor_pos--;
                    states[cursor_pos] = CHAR_UNTYPED;
                }
                if (cursor_pos > 0) {
                    cursor_pos--;
                    if (states[cursor_pos] == CHAR_CORRECT) metrics.correct--;
                    else if (states[cursor_pos] == CHAR_WRONG) metrics.errors--;
                    states[cursor_pos] = CHAR_UNTYPED;
                }
            } else if (ch == 27) { // Esc - досрочный выход
                break;
            } else if (ch == KEY_RESIZE) {
                // перерисовать при изменении размера терминала
            } else if (ch == '\t' || (ch >= 32 && ch < 127)) {
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
                // авто-пропуск переносов строк после символа
                while (cursor_pos < lesson->len && lesson->text[cursor_pos] == '\n') {
                    states[cursor_pos] = CHAR_CORRECT;
                    cursor_pos++;
                }
                if (cursor_pos >= lesson->len)
                    break;
            }

            metrics_update(&metrics);
            lesson_draw(lesson, cursor_pos, states, &metrics);
        }

        if (!metrics.started) break; // ни одной клавиши не нажато - выйти без результатов

        metrics.duration_sec = metrics.started ? (int)elapsed_sec(&metrics) : 0;
        action = lesson_show_results(&metrics, lesson->name);

    } while (action == RESULT_REPEAT);

    free(states);
    lesson_free(lesson);
    return action;
}

void lesson_select_menu(const char *dir) // сканирует dir на .txt файлы, сортирует, показывает список для выбора
{
    lessons_run_menu(dir);
}

LessonList *lessons_scan(const char *dir) {
    LessonList *list = malloc(sizeof(LessonList));
    if (!list) return NULL;
    list->count = 0;

    DIR *d = opendir(dir);
    if (!d) { free(list); return NULL; }

    struct dirent *ent;
    while ((ent = readdir(d)) && list->count < MAX_LESSONS) {
        char *dot = strrchr(ent->d_name, '.');
        if (!dot || strcmp(dot, ".txt") != 0) continue;

        LessonEntry *entry = &list->entries[list->count];
        snprintf(entry->path, LESSON_PATH_MAX, "%s/%s", dir, ent->d_name);

        // убрать расширение, заменить _ на пробел
        strncpy(entry->name, ent->d_name, LESSON_NAME_MAX - 1);
        entry->name[LESSON_NAME_MAX - 1] = '\0';
        char *d2 = strrchr(entry->name, '.');
        if (d2) *d2 = '\0';
        for (char *p = entry->name; *p; p++)
            if (*p == '_') *p = ' ';

        list->count++;
    }
    closedir(d);

    if (list->count == 0) { free(list); return NULL; }

    // сортировка по имени файла
    for (int i = 0; i < list->count - 1; i++)
        for (int j = i + 1; j < list->count; j++)
            if (strcmp(list->entries[i].path, list->entries[j].path) > 0) {
                LessonEntry tmp = list->entries[i];
                list->entries[i] = list->entries[j];
                list->entries[j] = tmp;
            }

    return list;
}

void lessons_free(LessonList *list) {
    if (list) free(list);
}

int lessons_run_menu(const char *dir) {
    LessonList *list = lessons_scan(dir);
    if (!list) return 0;

    int selected = 0;
    int ch;

    while (1) {
        clear();

        attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
        mvhline(0, 0, ACS_HLINE, COLS);
        mvprintw(1, 3, "Lessons");
        mvhline(2, 0, ACS_HLINE, COLS);
        attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

        for (int i = 0; i < list->count; i++) {
            if (i == selected) {
                attron(COLOR_PAIR(COLOR_GREEN_ON_BLACK) | A_BOLD | A_REVERSE);
                mvprintw(4 + i, 3, " %s ", list->entries[i].name);
                attroff(COLOR_PAIR(COLOR_GREEN_ON_BLACK) | A_BOLD | A_REVERSE);
            } else {
                attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
                mvprintw(4 + i, 3, " %s ", list->entries[i].name);
                attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
            }
        }

        attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        mvprintw(LINES - 1, 3, "up/dn - move   Enter - start   Esc - back");
        attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));

        refresh();

        ch = getch();
        if (ch == 27) break;
        if (ch == KEY_UP)   selected = (selected - 1 + list->count) % list->count;
        if (ch == KEY_DOWN) selected = (selected + 1) % list->count;
        if (ch == '\n' || ch == KEY_ENTER) {
            ResultAction a = lesson_run(list->entries[selected].path);
            if (a == RESULT_MAIN_MENU) break;
        }
    }

    lessons_free(list);
    return 1;
}
