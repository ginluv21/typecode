#define _POSIX_C_SOURCE 200809L
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "lesson.h"
#include "typecode.h"
#include "stats.h"
#include "heatmap.h"
#include "ui.h"

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

double elapsed_sec(const Metrics *m)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec - m->start.tv_sec)
         + (now.tv_nsec - m->start.tv_nsec) / 1e9;
}

void metrics_update(Metrics *m)
{
    if (!m->started) return;
    m->wpm      = metrics_calc_wpm(m->correct, elapsed_sec(m));
    m->accuracy = metrics_calc_accuracy(m->correct, m->errors);
}

// GCOVR_EXCL_START
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

    // строка аннотации спецсимволов
    {
        int has_newline = 0, has_tab = 0;
        for (int i = 0; i < lesson->len; i++) {
            if (lesson->text[i] == '\n') has_newline = 1;
            if (lesson->text[i] == '\t') has_tab = 1;
        }
        if (has_newline || has_tab) {
            attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK) | A_DIM);
            int x = TEXT_MARGIN;
            if (has_newline) {
                mvprintw(LINES - 2, x, "$ Enter");
                x += 10;
            }
            if (has_tab) {
                mvprintw(LINES - 2, x, "> Tab");
            }
            attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK) | A_DIM);
        }
    }

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
            if (states == NULL || states[i] == CHAR_UNTYPED) {
                if (i == cursor_pos) {
                    attron(COLOR_PAIR(COLOR_YELLOW_ON_BLACK) | A_BOLD | A_REVERSE);
                    mvaddch(text_row, text_col, '$');
                    attroff(COLOR_PAIR(COLOR_YELLOW_ON_BLACK) | A_BOLD | A_REVERSE);
                } else {
                    attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK) | A_DIM);
                    mvaddch(text_row, text_col, '$');
                    attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK) | A_DIM);
                }
            } else if (states[i] == CHAR_WRONG) {
                attron(COLOR_PAIR(COLOR_RED_ON_BLACK) | A_BOLD);
                mvaddch(text_row, text_col, '$');
                attroff(COLOR_PAIR(COLOR_RED_ON_BLACK) | A_BOLD);
            }
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

static void draw_results(int row, int col, int w, int h,
                         const Metrics *metrics, const char *name)
{
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

    attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK) | A_BOLD);
    mvprintw(row + 1, col + (w - (int)strlen(name)) / 2, "%s", name);
    attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK) | A_BOLD);

    attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
    mvaddch(row + 2, col, ACS_LTEE);
    mvhline(row + 2, col + 1, ACS_HLINE, w - 2);
    mvaddch(row + 2, col + w - 1, ACS_RTEE);
    attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

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

    attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
    mvaddch(row + h - 3, col, ACS_LTEE);
    mvhline(row + h - 3, col + 1, ACS_HLINE, w - 2);
    mvaddch(row + h - 3, col + w - 1, ACS_RTEE);
    attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

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
}

ResultAction lesson_show_results(const Metrics *metrics, const char *name) // показывает итоговую рамку со статистикой, ждёт R/Esc/Q
{
    int w = 50;
    int h = 12;
    int row = (LINES - h) / 2;
    int col = (COLS  - w) / 2;

    draw_results(row, col, w, h, metrics, name);

    int ch;
    while ((ch = getch()) != ERR) {
        if (ch == 'r' || ch == 'R')                       return RESULT_REPEAT;
        if (ch == 27 || ch == '\n' || ch == KEY_ENTER)    return RESULT_LESSONS;
        if (ch == 'q' || ch == 'Q')                       return RESULT_MAIN_MENU;
        if (ch == KEY_RESIZE) {
            ui_on_resize();
            row = (LINES - h) / 2;
            col = (COLS  - w) / 2;
            if (!ui_too_small())
                draw_results(row, col, w, h, metrics, name);
        }
    }
    return RESULT_LESSONS;
}

static Metrics g_last_metrics = {0};

static ResultAction lesson_run_internal(Lesson *lesson, const Settings *s, int show_results)
{
    CharState *states = calloc(lesson->len, sizeof(CharState));
    if (!states) { lesson_free(lesson); return RESULT_LESSONS; }

    ResultAction action;
    do {
        memset(states, 0, lesson->len * sizeof(CharState));
        Metrics metrics = {0};
        int cursor_pos  = 0;

        lesson_draw(lesson, cursor_pos, states, &metrics);
        if (s->hardcore) {
            attron(COLOR_PAIR(COLOR_RED_ON_BLACK) | A_BOLD);
            mvprintw(1, COLS - 12, "[HARDCORE]");
            attroff(COLOR_PAIR(COLOR_RED_ON_BLACK) | A_BOLD);
            refresh();
        }

        /* allow getch to time out briefly so we can update timer even when
           no key is pressed; restore blocking mode after the input loop */
        timeout(100); /* wait up to 100ms for input */

        if (s->mode == MODE_TIMED) {
            clock_gettime(CLOCK_MONOTONIC, &metrics.start);
            metrics.started = 1;
        }

        for (;;) {
            int ch = getch();

            if (ch == ERR) {
                /* no input this tick - update timer display if timed */
                if (s->mode == MODE_TIMED && metrics.started) {
                    int remaining = s->time_limit_sec - (int)elapsed_sec(&metrics);
                    if (remaining <= 0) break;
                    attron(COLOR_PAIR(COLOR_YELLOW_ON_BLACK) | A_BOLD);
                    mvprintw(LINES - 1, COLS - 30, "[%3ds]", remaining);
                    attroff(COLOR_PAIR(COLOR_YELLOW_ON_BLACK) | A_BOLD);
                    refresh();
                }
                continue;
            }

            if ((ch == KEY_BACKSPACE || ch == 127 || ch == '\b') && !s->hardcore) {
                if (cursor_pos > 0) {
                    cursor_pos--;
                    if (states[cursor_pos] == CHAR_CORRECT) metrics.correct--;
                    else if (states[cursor_pos] == CHAR_WRONG) metrics.errors--;
                    states[cursor_pos] = CHAR_UNTYPED;
                }
            } else if (ch == 27) { // досрочный выход
                break;
            } else if (ch == KEY_RESIZE) {
                ui_on_resize();
                if (ui_too_small()) continue;
            } else if (ch == '\t' || ch == '\n' || ch == KEY_ENTER || (ch >= 32 && ch < 127)) {
                if (!metrics.started) {
                    clock_gettime(CLOCK_MONOTONIC, &metrics.start);
                    metrics.started = 1;
                }
                if (cursor_pos < lesson->len) {
                    char input = (ch == KEY_ENTER) ? '\n' : (char)ch;
                    if (input == lesson->text[cursor_pos]) {
                        states[cursor_pos] = CHAR_CORRECT;
                        metrics.correct++;
                    } else {
                        states[cursor_pos] = CHAR_WRONG;
                        metrics.errors++;
                        heatmap_record_error(&global_heatmap, lesson->text[cursor_pos], input);
                    }
                    cursor_pos++;
                }
                if (cursor_pos >= lesson->len) {
                    if (s->mode == MODE_INFINITE) {
                        memset(states, 0, lesson->len * sizeof(CharState));
                        cursor_pos = 0;
                    } else {
                        break;
                    }
                }
            }

            metrics_update(&metrics);
            lesson_draw(lesson, cursor_pos, states, &metrics);
            if (s->hardcore) {
                attron(COLOR_PAIR(COLOR_RED_ON_BLACK) | A_BOLD);
                mvprintw(1, COLS - 12, "[HARDCORE]");
                attroff(COLOR_PAIR(COLOR_RED_ON_BLACK) | A_BOLD);
                refresh();
            }
            if (s->mode == MODE_TIMED && metrics.started) {
                int remaining = s->time_limit_sec - (int)elapsed_sec(&metrics);
                if (remaining <= 0) break;
                attron(COLOR_PAIR(COLOR_YELLOW_ON_BLACK) | A_BOLD);
                mvprintw(LINES - 1, COLS - 30, "[%3ds]", remaining);
                attroff(COLOR_PAIR(COLOR_YELLOW_ON_BLACK) | A_BOLD);
                refresh();
            }
        }
        /* restore blocking getch for other screens */
        timeout(-1);

        if (!metrics.started) break; // ни одной клавиши не нажато - выйти без результатов

        metrics.duration_sec = metrics.started ? (int)elapsed_sec(&metrics) : 0;
        g_last_metrics = metrics;
        heatmap_save(&global_heatmap);
        if (!show_results) {
            action = RESULT_LESSONS;
            break;
        }
        action = lesson_show_results(&metrics, lesson->name);
        if (action != RESULT_REPEAT) {
            SessionResult r = {
                .timestamp    = time(NULL),
                .wpm          = metrics.wpm,
                .accuracy     = metrics.accuracy,
                .errors       = metrics.errors,
                .duration_sec = metrics.duration_sec,
            };
            strncpy(r.lesson, lesson->name, LESSON_MAX - 1);
            r.lesson[LESSON_MAX - 1] = '\0';
            stats_save(&r);
        }

    } while (action == RESULT_REPEAT);

    free(states);
    lesson_free(lesson);
    return action;
}

ResultAction lesson_run(const char *path, const Settings *s)
{
    Lesson *lesson = lesson_load(path);
    if (!lesson) return RESULT_LESSONS;
    return lesson_run_internal(lesson, s, 1);
}

ResultAction lesson_run_with_name(const char *path, const char *display_name, const Settings *s)
{
    Lesson *lesson = lesson_load(path);
    if (!lesson) return RESULT_LESSONS;

    if (display_name) {
        strncpy(lesson->name, display_name, LESSON_NAME_MAX - 1);
        lesson->name[LESSON_NAME_MAX - 1] = '\0';
    }

    return lesson_run_internal(lesson, s, 1);
}

int lesson_run_text_score(const char *text, const char *name, int hardcore, LessonMode mode, int time_limit)
{
    if (!text) return 0;

    g_last_metrics = (Metrics){0};

    Lesson *lesson = malloc(sizeof(Lesson));
    if (!lesson) return 0;

    lesson->len = (int)strlen(text);
    lesson->text = malloc(lesson->len + 1);
    if (!lesson->text) {
        free(lesson);
        return 0;
    }
    memcpy(lesson->text, text, lesson->len + 1);

    if (name) {
        strncpy(lesson->name, name, LESSON_NAME_MAX - 1);
        lesson->name[LESSON_NAME_MAX - 1] = '\0';
    } else {
        lesson->name[0] = '\0';
    }

    Settings s = {
        .hardcore = hardcore,
        .mode = mode,
        .time_limit_sec = time_limit,
    };

    lesson_run_internal(lesson, &s, 1);
    return g_last_metrics.wpm;
}

int lesson_run_challenge_round(const char *text, int time_sec)
{
    if (!text) return 0;
    g_last_metrics = (Metrics){0};

    Lesson *lesson = malloc(sizeof(Lesson));
    if (!lesson) return 0;

    lesson->len = (int)strlen(text);
    lesson->text = malloc(lesson->len + 1);
    if (!lesson->text) { free(lesson); return 0; }
    memcpy(lesson->text, text, lesson->len + 1);
    lesson->name[0] = '\0';

    Settings s = {
        .hardcore = 0,
        .mode = MODE_TIMED,
        .time_limit_sec = time_sec,
    };

    lesson_run_internal(lesson, &s, 0);
    return g_last_metrics.wpm;
}

void lesson_run_text(const char *text, const char *name, int hardcore, LessonMode mode, int time_limit)
{
    (void)lesson_run_text_score(text, name, hardcore, mode, time_limit);
}

void lesson_select_menu(const char *dir, const Settings *s, const AppConfig *cfg)
{
    lessons_run_menu(dir, s, cfg);
}
// GCOVR_EXCL_STOP
