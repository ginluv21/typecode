#define _POSIX_C_SOURCE 200809L
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include "heatmap.h"
#include "lesson.h"
#include "settings.h"
#include "stats.h"
#include "typecode.h"
#include "ui.h"
#include "config.h"
#include "challenge.h"

#define PAGE_SIZE 10
#define MARGIN    3
#define LOAD_MAX  1024

#define NUM_TABS 4
static const char *tab_names[] = { "Overview", "Heatmap", "Weak Spots", "Challenge" };

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

void stats_reset(void)
{
    char path[512];
    build_path(path, sizeof(path), STATS_FILE);
    FILE *f = fopen(path, "w");
    if (f) fclose(f);
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

static int confirm_dialog(const AppConfig *cfg, const char *line1, const char *line2)
{
    int w = 42, h = 6;
    int r0 = (LINES - h) / 2;
    int c0 = (COLS  - w) / 2;
    int sel = 1; // 0 = Yes, 1 = No (default No - safer)

    for (;;) {
        attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        for (int i = 0; i < h; i++)
            mvhline(r0 + i, c0, ' ', w);
        attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));

        attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
        mvaddch(r0,         c0,         ACS_ULCORNER);
        mvhline(r0,         c0 + 1,     ACS_HLINE, w - 2);
        mvaddch(r0,         c0 + w - 1, ACS_URCORNER);
        for (int i = 1; i < h - 1; i++) {
            mvaddch(r0 + i, c0,         ACS_VLINE);
            mvaddch(r0 + i, c0 + w - 1, ACS_VLINE);
        }
        mvaddch(r0 + h - 1, c0,         ACS_LLCORNER);
        mvhline(r0 + h - 1, c0 + 1,     ACS_HLINE, w - 2);
        mvaddch(r0 + h - 1, c0 + w - 1, ACS_LRCORNER);
        attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

        attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK) | A_BOLD);
        mvprintw(r0 + 1, c0 + 2, "%s", line1);
        attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK) | A_BOLD);

        attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        mvprintw(r0 + 2, c0 + 2, "%s", line2);
        attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));

        int yes_attr = (sel == 0)
            ? COLOR_PAIR(COLOR_GREEN_ON_BLACK) | A_BOLD | A_REVERSE
            : COLOR_PAIR(COLOR_GREEN_ON_BLACK) | A_BOLD;
        int no_attr  = (sel == 1)
            ? COLOR_PAIR(COLOR_WHITE_ON_BLACK) | A_BOLD | A_REVERSE
            : COLOR_PAIR(COLOR_WHITE_ON_BLACK);

        attron(yes_attr);
        mvprintw(r0 + 4, c0 + 8, "  Yes  ");
        attroff(yes_attr);

        attron(no_attr);
        mvprintw(r0 + 4, c0 + 26, "  No   ");
        attroff(no_attr);

        attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK) | A_DIM);
        mvprintw(r0 + h - 1, c0 + 2, " <-/-> move   Enter confirm   y/n ");
        attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK) | A_DIM);

        refresh();

        int ch = getch();
        if (ch == 'y' || ch == 'Y')                          return 1;
        if (ch == 'n' || ch == 'N' || ch == 27)              return 0;
        int left = (ch == KEY_LEFT) || (cfg && cfg->vim_mode && ch == 'h');
        int right = (ch == KEY_RIGHT) || (cfg && cfg->vim_mode && ch == 'l');
        if (left || right)               sel = !sel;
        if (ch == '\n' || ch == KEY_ENTER)                    return sel == 0;
    }
}

static void draw_tab_bar(int active)
{
    int total_w = COLS - 2 * MARGIN;
    int tab_w   = (total_w - (NUM_TABS - 1)) / NUM_TABS;
    if (tab_w < 4) tab_w = 4;

    for (int i = 0; i < NUM_TABS; i++) {
        int tab_start = MARGIN + i * (tab_w + 1);

        char label[32];
        snprintf(label, sizeof(label), "%d %s", i + 1, tab_names[i]);
        int label_len = (int)strlen(label);
        int pad = (tab_w - label_len) / 2;
        if (pad < 0) pad = 0;

        if (i == active) {
            attron(COLOR_PAIR(COLOR_GREEN_ON_BLACK) | A_BOLD | A_REVERSE);
        } else {
            attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK) | A_DIM);
        }

        for (int j = 0; j < tab_w; j++)
            mvaddch(3, tab_start + j, ' ');
        mvprintw(3, tab_start + pad, "%s", label);

        if (i == active) {
            attroff(COLOR_PAIR(COLOR_GREEN_ON_BLACK) | A_BOLD | A_REVERSE);
        } else {
            attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK) | A_DIM);
        }

        if (i < NUM_TABS - 1) {
            attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK) | A_DIM);
            mvaddch(3, tab_start + tab_w, ACS_VLINE);
            attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK) | A_DIM);
        }
    }
}

static void draw_tab0(const SessionResult *sessions, int total,
                      int best_wpm, float avg_wpm_10, float avg_acc,
                      int scroll, int y0)
{
    if (total == 0) {
        attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        mvprintw(y0, MARGIN, "No stats yet. Complete at least one lesson!");
        attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        return;
    }

    attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
    mvprintw(y0,     MARGIN, "Best WPM:        %d", best_wpm);
    mvprintw(y0 + 1, MARGIN, "Avg WPM:         %.0f  (last 10)", avg_wpm_10);
    mvprintw(y0 + 2, MARGIN, "Avg accuracy:    %.1f%%", avg_acc);
    mvprintw(y0 + 3, MARGIN, "Total sessions:  %d", total);
    attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));

    attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
    mvhline(y0 + 5, 0, ACS_HLINE, COLS);
    mvprintw(y0 + 6, MARGIN, "%-20s  %4s  %8s  %6s", "Lesson", "WPM", "Accuracy", "Errors");
    mvhline(y0 + 7, MARGIN, ACS_HLINE, COLS - MARGIN * 2);
    attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

    for (int i = 0; i < PAGE_SIZE; i++) {
        int idx = total - 1 - scroll - i;
        if (idx < 0) break;
        char name[21];
        strncpy(name, sessions[idx].lesson, 20);
        name[20] = '\0';
        attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        mvprintw(y0 + 8 + i, MARGIN, "%-20s  %4d  %7.0f%%  %6d",
                 name, sessions[idx].wpm, sessions[idx].accuracy, sessions[idx].errors);
        attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
    }
}

static void draw_challenge_tab(const ChallengeRecord *records, int count, int y0)
{
    int best = 0;
    int sum = 0;
    for (int i = 0; i < count; i++) {
        if (records[i].wpm > best) best = records[i].wpm;
        sum += records[i].wpm;
    }
    int avg = count ? (sum + count / 2) / count : 0;

    attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
    mvprintw(y0,     MARGIN, "Challenge runs: %d", count);
    mvprintw(y0 + 1, MARGIN, "Best WPM:       %d", best);
    mvprintw(y0 + 2, MARGIN, "Average WPM:    %d", avg);
    attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));

    attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
    mvhline(y0 + 4, 0, ACS_HLINE, COLS);
    mvprintw(y0 + 5, MARGIN, "%-4s  %-5s  %s", "Pos", "WPM", "Date");
    mvhline(y0 + 6, MARGIN, ACS_HLINE, COLS - MARGIN * 2);
    attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

    for (int i = 0; i < count && i < 5; i++) {
        char date[32];
        struct tm tm_info;
        localtime_r(&records[i].timestamp, &tm_info);
        strftime(date, sizeof(date), "%Y-%m-%d", &tm_info);
        attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        mvprintw(y0 + 7 + i, MARGIN, "%2d.    %4d   %s", i + 1, records[i].wpm, date);
        attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
    }
}

static void draw_hint(int tab, int total)
{
    const char *base = "<-/-> tabs   1-4 jump   q/Esc back";
    char hint[128];
    switch (tab) {
        case 0:
            if (total > PAGE_SIZE)
                snprintf(hint, sizeof(hint), "%s   up/dn scroll   R reset", base);
            else
                snprintf(hint, sizeof(hint), "%s   R reset", base);
            break;
        case 1:
            snprintf(hint, sizeof(hint), "%s   R reset", base);
            break;
        case 2:
            snprintf(hint, sizeof(hint), "%s   G generate exercise", base);
            break;
        default:
            snprintf(hint, sizeof(hint), "%s", base);
            break;
    }
    attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK) | A_DIM);
    mvprintw(LINES - 1, MARGIN, "%s", hint);
    attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK) | A_DIM);
}

void stats_draw_screen(const AppConfig *cfg)
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

    int tab = 0;
    int scroll_overview = 0;
    int max_scroll_ov = (total > PAGE_SIZE) ? total - PAGE_SIZE : 0;

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

        draw_tab_bar(tab);

        attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
        mvhline(4, 0, ACS_HLINE, COLS);
        attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

        int y0 = 6;
        ChallengeRecord challenge_records[10];
        int challenge_count = challenge_load_records(challenge_records, 10);

        switch (tab) {
            case 0: draw_tab0(sessions, total, best_wpm, avg_wpm_10, avg_acc, scroll_overview, y0); break;
            case 1: heatmap_draw_content(&global_heatmap, y0); break;
            case 2: heatmap_weakspots_content(&global_heatmap, y0); break;
            case 3: draw_challenge_tab(challenge_records, challenge_count, y0); break;
        }

        draw_hint(tab, total);
        refresh();

        int ch = getch();
        if (ch == KEY_RESIZE) { ui_on_resize(); continue; }
        if (ch == 27 || ch == 'q' || ch == 'Q') break;
        int left = (ch == KEY_LEFT) || (cfg && cfg->vim_mode && ch == 'h');
        int right = (ch == KEY_RIGHT) || (cfg && cfg->vim_mode && ch == 'l');
        if (left)                        tab = (tab + NUM_TABS - 1) % NUM_TABS;
        if (right)                       tab = (tab + 1) % NUM_TABS;
        if (ch >= '1' && ch <= '0' + NUM_TABS)     tab = ch - '1';

        if (tab == 0) {
            if ((ch == KEY_UP || (cfg && cfg->vim_mode && ch == 'k'))   && scroll_overview > 0)             scroll_overview--;
            if ((ch == KEY_DOWN || (cfg && cfg->vim_mode && ch == 'j')) && scroll_overview < max_scroll_ov) scroll_overview++;
            if (ch == 'r' || ch == 'R') {
                if (confirm_dialog(cfg, "Reset session history?",
                                       "All recorded sessions will be deleted.")) {
                    stats_reset();
                    total = 0;
                    best_wpm = 0;
                    avg_wpm_10 = 0.0f;
                    avg_acc = 0.0f;
                    scroll_overview = 0;
                    max_scroll_ov = 0;
                }
            }
        }
        if (tab == 1 && (ch == 'r' || ch == 'R')) {
            if (confirm_dialog(cfg, "Reset heatmap statistics?",
                               "All error counts will be cleared."))
                heatmap_reset(&global_heatmap);
        }
        if (tab == 2 && (ch == 'g' || ch == 'G')) {
            char *exercise = heatmap_generate_exercise(&global_heatmap, 5);
            if (exercise) {
                lesson_run_text(exercise, "Weak Spots Exercise", 0, MODE_NORMAL, 0);
                free(exercise);
                total = stats_load(sessions, LOAD_MAX);
                best_wpm = 0;
                avg_wpm_10 = 0.0f;
                avg_acc = 0.0f;
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
                max_scroll_ov = (total > PAGE_SIZE) ? total - PAGE_SIZE : 0;
            }
        }
    }
}
// GCOVR_EXCL_STOP
