#define _POSIX_C_SOURCE 200809L
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include "challenge.h"
#include "ui.h"
#include "stats.h"
#include "typecode.h"

#define CHALLENGE_FILE "/.typecode/challenge.txt"
#define MAX_RECORDS 5

static const ChallengeRound g_rounds[5] = {
    {1, 20, 10, "Home row"},
    {2, 30, 15, "Mixed letters"},
    {3, 40, 20, "Punctuation"},
    {4, 50, 25, "Code"},
    {5, 60, 30, "Full text"},
};

static const char *g_round_texts[5] = {
    "asdf jkl; asdf jkl; asdf jkl; asdf jkl; asdf jkl; asdf jkl;",
    "qwe asd zxc poi lkj mnb rfg tnh yuj ik, ol.; p?",
    "Hello, world! This is a speed challenge: type fast, type clean.",
    "for (int i = 0; i < 10; i++) { printf(\"%d\\n\", i); }",
    "The quick brown fox jumps over the lazy dog. Speed and accuracy matter."
};

static void build_path(char *buf, size_t size, const char *suffix)
{
    const char *home = getenv("HOME");
    snprintf(buf, size, "%s%s", home ? home : "", suffix);
}

static void ensure_history_dir(void)
{
    char path[512];
    build_path(path, sizeof(path), "/.typecode");
    mkdir(path, 0755);
}

static int cmp_record_desc(const void *a, const void *b)
{
    const ChallengeRecord *ra = a;
    const ChallengeRecord *rb = b;
    return rb->wpm - ra->wpm;
}

int challenge_load_records(ChallengeRecord out[], int max)
{
    ensure_history_dir();
    char path[512];
    build_path(path, sizeof(path), CHALLENGE_FILE);
    FILE *f = fopen(path, "r");
    if (!f) return 0;

    int count = 0;
    while (count < max) {
        int wpm;
        long ts;
        if (fscanf(f, "%d %ld\n", &wpm, &ts) != 2) break;
        out[count].wpm = wpm;
        out[count].timestamp = (time_t)ts;
        count++;
    }
    fclose(f);
    qsort(out, count, sizeof(ChallengeRecord), cmp_record_desc);
    return count;
}

static void save_record(int wpm)
{
    ensure_history_dir();
    char path[512];
    build_path(path, sizeof(path), CHALLENGE_FILE);
    FILE *f = fopen(path, "a");
    if (!f) return;
    fprintf(f, "%d %ld\n", wpm, (long)time(NULL));
    fclose(f);
}

static const char *rank_for_wpm(int wpm)
{
    if (wpm >= 80) return "S";
    if (wpm >= 60) return "A";
    if (wpm >= 45) return "B";
    return "C";
}

static void draw_centered_box(int row, int col, int height, int width)
{
    attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
    mvaddch(row, col, ACS_ULCORNER);
    mvhline(row, col + 1, ACS_HLINE, width - 2);
    mvaddch(row, col + width - 1, ACS_URCORNER);
    for (int i = 1; i < height - 1; i++) {
        mvaddch(row + i, col, ACS_VLINE);
        mvaddch(row + i, col + width - 1, ACS_VLINE);
    }
    mvaddch(row + height - 1, col, ACS_LLCORNER);
    mvhline(row + height - 1, col + 1, ACS_HLINE, width - 2);
    mvaddch(row + height - 1, col + width - 1, ACS_LRCORNER);
    attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
}

static int prompt_round_start(const ChallengeRound *rnd)
{
    int width = 40;
    int height = 9;
    int row = (LINES - height) / 2;
    int col = (COLS - width) / 2;

    for (;;) {
        clear();
        draw_centered_box(row, col, height, width);

        attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK) | A_BOLD);
        mvprintw(row + 1, col + 3, "Speed Challenge - Round %d of 5", rnd->round);
        attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK) | A_BOLD);

        attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        mvprintw(row + 3, col + 3, "Required WPM:   >= %d", rnd->min_wpm);
        mvprintw(row + 4, col + 3, "Time:           %d seconds", rnd->time_sec);
        mvprintw(row + 6, col + 3, "Enter - start   Esc - exit");
        attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));

        refresh();

        int ch = getch();
        if (ch == '\n' || ch == KEY_ENTER) return 1;
        if (ch == 27) return 0;
        if (ch == KEY_RESIZE) { ui_on_resize(); continue; }
    }
}

static int prompt_game_over(int round, int wpm, int required)
{
    int width = 40;
    int height = 9;
    int row = (LINES - height) / 2;
    int col = (COLS - width) / 2;

    for (;;) {
        clear();
        draw_centered_box(row, col, height, width);

        attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK) | A_BOLD);
        mvprintw(row + 1, col + 3, "Game Over");
        attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK) | A_BOLD);

        attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        mvprintw(row + 3, col + 3, "Failed on round %d", round);
        mvprintw(row + 4, col + 3, "WPM: %d  (needed >= %d)", wpm, required);
        mvprintw(row + 6, col + 3, "R - retry   Q - menu");
        attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));

        refresh();

        int ch = getch();
        if (ch == 'r' || ch == 'R') return 1;
        if (ch == 'q' || ch == 'Q' || ch == 27) return 0;
        if (ch == KEY_RESIZE) { ui_on_resize(); continue; }
    }
}

static int prompt_final(int last_wpm, int best_wpm, int avg_wpm, const ChallengeRecord records[], int count)
{
    int width = 50;
    int height = 16;
    int row = (LINES - height) / 2;
    int col = (COLS - width) / 2;

    for (;;) {
        clear();
        draw_centered_box(row, col, height, width);

        attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK) | A_BOLD);
        mvprintw(row + 1, col + 3, "Speed Challenge - Complete!");
        attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK) | A_BOLD);

        attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        mvprintw(row + 3, col + 3, "Rank:     %s", rank_for_wpm(last_wpm));
        mvprintw(row + 4, col + 3, "Best session WPM:  %d", best_wpm);
        mvprintw(row + 5, col + 3, "Average WPM:       %d", avg_wpm);
        mvprintw(row + 7, col + 3, "Top Records:");

        for (int i = 0; i < count && i < MAX_RECORDS; i++) {
            char date[32];
            struct tm tm_info;
            localtime_r(&records[i].timestamp, &tm_info);
            strftime(date, sizeof(date), "%Y-%m-%d", &tm_info);
            mvprintw(row + 8 + i, col + 3, "%d. %d WPM - %s", i + 1, records[i].wpm, date);
        }

        mvprintw(row + 14, col + 3, "R - retry   Q - menu");
        attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));

        refresh();

        int ch = getch();
        if (ch == 'r' || ch == 'R') return 1;
        if (ch == 'q' || ch == 'Q' || ch == 27) return 0;
        if (ch == KEY_RESIZE) { ui_on_resize(); continue; }
    }
}

static int play_round(int index)
{
    if (index < 0 || index >= 5) return 0;
    const ChallengeRound *rnd = &g_rounds[index];
    return lesson_run_text_score(g_round_texts[index], rnd->lesson_path, 0, MODE_TIMED, rnd->time_sec);
}

void challenge_run(const AppConfig *cfg)
{
    (void)cfg;
    ChallengeRecord records[MAX_RECORDS];
    int count = challenge_load_records(records, MAX_RECORDS);

    while (1) {
        int round_index = 0;
        int sums = 0;
        int last_wpm = 0;
        int best_wpm = 0;
        int success = 1;

        for (; round_index < 5; round_index++) {
            const ChallengeRound *rnd = &g_rounds[round_index];
            if (!prompt_round_start(rnd)) {
                return;
            }

            int wpm = play_round(round_index);
            last_wpm = wpm;
            sums += wpm;
            if (wpm > best_wpm) best_wpm = wpm;
            if (wpm < rnd->min_wpm) {
                if (!prompt_game_over(rnd->round, wpm, rnd->min_wpm)) {
                    return;
                }
                success = 0;
                break;
            }
        }

        if (!success) continue;

        if (round_index == 5) {
            int avg = sums / 5;
            save_record(last_wpm);
            count = challenge_load_records(records, MAX_RECORDS);
            if (!prompt_final(last_wpm, best_wpm, avg, records, count)) {
                return;
            }
        }
    }
}
