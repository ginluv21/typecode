#define _POSIX_C_SOURCE 200809L
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "heatmap.h"
#include "stats.h"
#include "typecode.h"
#include "ui.h"

#define PAGE_SIZE 10
#define MARGIN    3

#define LOAD_MAX 1024

static void build_path(char *buf, int size, const char *suffix)
{
    const char *home = getenv("HOME");
    snprintf(buf, size, "%s%s", home ? home : "", suffix);
}

void stats_ensure_dir(void)
{
    char path[512];
    build_path(path, sizeof(path), STATS_DIR);
    mkdir(path, 0755);
}

void stats_save(const SessionResult *r)
{
    char path[512];
    build_path(path, sizeof(path), STATS_FILE);
    FILE *f = fopen(path, "a");
    if (!f) return;
    fprintf(f, "%ld|%d|%.1f|%d|%d|%s\n",
            r->timestamp, r->wpm, r->accuracy,
            r->errors, r->duration_sec, r->lesson);
    fclose(f);
}

int stats_load(SessionResult *out, int max)
{
    char path[512];
    build_path(path, sizeof(path), STATS_FILE);
    FILE *f = fopen(path, "r");
    if (!f) return 0;

    char line[512];
    int count = 0;
    while (count < max && fgets(line, sizeof(line), f)) {
        SessionResult r = {0};
        if (sscanf(line, "%ld|%d|%f|%d|%d|%127[^\n]",
                   &r.timestamp, &r.wpm, &r.accuracy,
                   &r.errors, &r.duration_sec, r.lesson) == 6) {
            out[count++] = r;
        }
    }
    fclose(f);
    return count;
}

int stats_best_wpm(void)
{
    SessionResult buf[LOAD_MAX];
    int n = stats_load(buf, LOAD_MAX);
    int best = 0;
    for (int i = 0; i < n; i++)
        if (buf[i].wpm > best) best = buf[i].wpm;
    return best;
}

float stats_avg_wpm(int last_n)
{
    SessionResult buf[LOAD_MAX];
    int n = stats_load(buf, LOAD_MAX);
    if (n == 0) return 0.0f;
    int from = (n > last_n) ? n - last_n : 0;
    float sum = 0;
    for (int i = from; i < n; i++) sum += buf[i].wpm;
    return sum / (n - from);
}

// GCOVR_EXCL_START
void stats_draw_screen(void)
{
    SessionResult sessions[LOAD_MAX];
    int total = stats_load(sessions, LOAD_MAX);

    int best_wpm = 0;
    float avg_wpm_10 = 0.0f, avg_acc = 0.0f;
    if (total > 0) {
        int from = (total > 10) ? total - 10 : 0;
        float wpm_sum = 0, acc_sum = 0;
        for (int i = from; i < total; i++) wpm_sum += sessions[i].wpm;
        avg_wpm_10 = wpm_sum / (total - from);
        for (int i = 0; i < total; i++) {
            if (sessions[i].wpm > best_wpm) best_wpm = sessions[i].wpm;
            acc_sum += sessions[i].accuracy;
        }
        avg_acc = acc_sum / total;
    }

    int scroll = 0;
    int max_scroll = (total > PAGE_SIZE) ? total - PAGE_SIZE : 0;

    for (;;) {
        if (ui_too_small()) {
            int c = getch();
            if (c == KEY_RESIZE) ui_on_resize();
            continue;
        }
        clear();

        attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
        mvhline(0, 0, ACS_HLINE, COLS);
        mvprintw(1, MARGIN, "Statistics");
        mvhline(2, 0, ACS_HLINE, COLS);
        attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

        if (total == 0) {
            attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
            mvprintw(4, MARGIN, "No stats yet. Complete at least one lesson!");
            attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        } else {
            attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
            mvprintw(4, MARGIN, "Best WPM:         %d", best_wpm);
            mvprintw(5, MARGIN, "Avg WPM:          %.0f  (last 10)", avg_wpm_10);
            mvprintw(6, MARGIN, "Avg accuracy:     %.1f%%", avg_acc);
            mvprintw(7, MARGIN, "Total sessions:   %d", total);
            attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));

            attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
            mvhline(9, 0, ACS_HLINE, COLS);
            attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

            attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK) | A_BOLD);
            mvprintw(10, MARGIN, "Recent sessions:");
            attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK) | A_BOLD);

            attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
            mvprintw(12, MARGIN, "%-20s  %4s  %8s  %6s", "Lesson", "WPM", "Accuracy", "Errors");
            mvhline(13, MARGIN, ACS_HLINE, COLS - MARGIN * 2);
            attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

            for (int i = 0; i < PAGE_SIZE; i++) {
                int idx = total - 1 - scroll - i;
                if (idx < 0) break;
                char name[21];
                strncpy(name, sessions[idx].lesson, 20);
                name[20] = '\0';
                attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
                mvprintw(14 + i, MARGIN, "%-20s  %4d  %7.0f%%  %6d",
                         name, sessions[idx].wpm, sessions[idx].accuracy, sessions[idx].errors);
                attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
            }
        }

        attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK) | A_DIM);
        if (total > PAGE_SIZE)
            mvprintw(LINES - 1, MARGIN, "Esc / Q - back    H - heatmap    W - weak spots    up/down - scroll");
        else
            mvprintw(LINES - 1, MARGIN, "Esc / Q - back    H - heatmap    W - weak spots");
        attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK) | A_DIM);

        refresh();

        int ch = getch();
        if (ch == KEY_RESIZE) { ui_on_resize(); continue; }
        if (ch == 27 || ch == 'q' || ch == 'Q') break;
        if (ch == 'h' || ch == 'H') {
            heatmap_draw_screen(&global_heatmap);
            continue;
        }
        if (ch == 'w' || ch == 'W') {
            heatmap_draw_weakspots(&global_heatmap);
            continue;
        }
        if (ch == KEY_UP   && scroll > 0)          scroll--;
        if (ch == KEY_DOWN && scroll < max_scroll)  scroll++;
    }
}
// GCOVR_EXCL_STOP
