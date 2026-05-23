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
#define MAX_RECORDS    5
#define CHALLENGE_ROUNDS 10

static const ChallengeRound g_rounds[CHALLENGE_ROUNDS] = {
    { 1, 15, 15, "Home row"},
    { 2, 20, 18, "Home row words"},
    { 3, 25, 20, "All letters"},
    { 4, 30, 22, "Punctuation"},
    { 5, 35, 25, "Numbers"},
    { 6, 38, 25, "Symbols"},
    { 7, 42, 28, "Simple code"},
    { 8, 46, 30, "Conditions"},
    { 9, 50, 33, "Loops"},
    {10, 55, 35, "Full function"},
};

static const char *g_round_texts[CHALLENGE_ROUNDS] = {
    "asdf jkl; asdf jkl; asdf jkl; asdf jkl; asdf jkl;",
    "add fall glad flask shall lass had flag all glass",
    "the quick brown fox jumps over the lazy dog",
    "Hello, world. Type fast, be clean. Good job!",
    "pin 1234, code 5678, id 90, score: 42, rank 7",
    "(a + b) * c; {x = y}; [0, 1, 2]; val = true;",
    "int x = 10; float y = 3.14; char c = 'A';",
    "if (x > 0) { return x; } else { return -x; }",
    "for (int i = 0; i < 10; i++) { printf(\"%d\\n\", i); }",
    "void run(int n) { while (n-- > 0) { printf(\"ok %d\\n\", n); } }",
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
    char line[64];
    while (count < max && fgets(line, sizeof(line), f)) {
        int wpm;
        long ts;
        if (sscanf(line, "%d %ld", &wpm, &ts) != 2) continue;
        out[count].wpm = wpm;
        out[count].timestamp = (time_t)ts;
        count++;
    }
    fclose(f);
    qsort(out, count, sizeof(ChallengeRecord), cmp_record_desc);
    return count;
}

// GCOVR_EXCL_START
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
    int height = 10;
    int row = (LINES - height) / 2;
    int col = (COLS - width) / 2;

    for (;;) {
        clear();
        draw_centered_box(row, col, height, width);

        attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK) | A_BOLD);
        mvprintw(row + 1, col + 3, "Speed Challenge - Round %d of %d", rnd->round, CHALLENGE_ROUNDS);
        attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK) | A_BOLD);

        attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        mvprintw(row + 3, col + 3, "Theme:          %s", rnd->lesson_path);
        mvprintw(row + 4, col + 3, "Required WPM:   >= %d", rnd->min_wpm);
        mvprintw(row + 5, col + 3, "Time:           %d seconds", rnd->time_sec);
        mvprintw(row + 7, col + 3, "Enter - start   Esc - exit");
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

static int prompt_round_passed(int round, int wpm)
{
    int width = 40;
    int height = 9;
    int row = (LINES - height) / 2;
    int col = (COLS - width) / 2;

    for (;;) {
        clear();
        draw_centered_box(row, col, height, width);

        attron(COLOR_PAIR(COLOR_GREEN_ON_BLACK) | A_BOLD);
        mvprintw(row + 1, col + 3, "Round %d - Passed!", round);
        attroff(COLOR_PAIR(COLOR_GREEN_ON_BLACK) | A_BOLD);

        attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        mvprintw(row + 3, col + 3, "WPM: %d", wpm);
        if (round < CHALLENGE_ROUNDS)
            mvprintw(row + 6, col + 3, "Enter - next round   Esc - exit");
        else
            mvprintw(row + 6, col + 3, "Enter - results      Esc - exit");
        attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));

        refresh();

        int ch = getch();
        if (ch == '\n' || ch == KEY_ENTER) return 1;
        if (ch == 27) return 0;
        if (ch == KEY_RESIZE) { ui_on_resize(); continue; }
    }
}

static int play_round(int index)
{
    if (index < 0 || index >= CHALLENGE_ROUNDS) return 0;
    const ChallengeRound *rnd = &g_rounds[index];
    return lesson_run_challenge_round(g_round_texts[index], rnd->time_sec);
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

        for (; round_index < CHALLENGE_ROUNDS; round_index++) {
            const ChallengeRound *rnd = &g_rounds[round_index];
            if (!prompt_round_start(rnd)) {
                return;
            }

            int wpm = play_round(round_index);
            last_wpm = wpm;
            sums += wpm;
            if (wpm > best_wpm) best_wpm = wpm;
            if (wpm < rnd->min_wpm) {
                save_record(best_wpm);
                count = challenge_load_records(records, MAX_RECORDS);
                if (!prompt_game_over(rnd->round, wpm, rnd->min_wpm)) {
                    return;
                }
                success = 0;
                break;
            }
            if (!prompt_round_passed(rnd->round, wpm)) {
                return;
            }
        }

        if (!success) continue;

        if (round_index == CHALLENGE_ROUNDS) {
            int avg = sums / CHALLENGE_ROUNDS;
            save_record(best_wpm);
            count = challenge_load_records(records, MAX_RECORDS);
            if (!prompt_final(last_wpm, best_wpm, avg, records, count)) {
                return;
            }
        }
    }
}
// GCOVR_EXCL_STOP
