#define _POSIX_C_SOURCE 200809L
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "settings.h"
#include "typecode.h"
#include "ui.h"

#define SETTINGS_FILE "/.typecode/settings.conf"

static void build_path(char *buf, int size, const char *suffix)
{
    const char *home = getenv("HOME");
    snprintf(buf, size, "%s%s", home ? home : "", suffix);
}

void settings_defaults(Settings *s)
{
    s->hardcore       = 0;
    s->mode           = MODE_NORMAL;
    s->time_limit_sec = 60;
}

void settings_save(const Settings *s)
{
    char path[512];
    build_path(path, sizeof(path), SETTINGS_FILE);
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "hardcore=%d\nmode=%d\ntime_limit=%d\n",
            s->hardcore, (int)s->mode, s->time_limit_sec);
    fclose(f);
}

void settings_load(Settings *s)
{
    settings_defaults(s);
    char path[512];
    build_path(path, sizeof(path), SETTINGS_FILE);
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[64];
    while (fgets(line, sizeof(line), f)) {
        int v;
        if (sscanf(line, "hardcore=%d", &v) == 1)    s->hardcore       = v;
        if (sscanf(line, "mode=%d",     &v) == 1)    s->mode           = (PracticeMode)v;
        if (sscanf(line, "time_limit=%d", &v) == 1)  s->time_limit_sec = v;
    }
    fclose(f);
}

// GCOVR_EXCL_START
void settings_draw_screen(Settings *s)
{
    static const char *mode_names[] = {"Normal", "Timed", "Infinite"};
    int sel = 0;

    for (;;) {
        if (ui_too_small()) {
            int c = getch();
            if (c == KEY_RESIZE) ui_on_resize();
            continue;
        }
        clear();
        attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
        mvhline(0, 0, ACS_HLINE, COLS);
        mvprintw(1, 3, "Settings");
        mvhline(2, 0, ACS_HLINE, COLS);
        attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

        const char *rows[3];
        char hc[32], md[32], tl[32];
        snprintf(hc, sizeof(hc), "Hardcore:    %s", s->hardcore ? "On" : "Off");
        snprintf(md, sizeof(md), "Mode:        %s", mode_names[s->mode]);
        snprintf(tl, sizeof(tl), "Time limit:  %ds", s->time_limit_sec);
        rows[0] = hc; rows[1] = md; rows[2] = tl;

        int nrows = (s->mode == MODE_TIMED) ? 3 : 2;
        for (int i = 0; i < nrows; i++) {
            if (i == sel) {
                attron(COLOR_PAIR(COLOR_GREEN_ON_BLACK) | A_BOLD | A_REVERSE);
                mvprintw(4 + i, 3, " %s", rows[i]);
                attroff(COLOR_PAIR(COLOR_GREEN_ON_BLACK) | A_BOLD | A_REVERSE);
            } else {
                attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
                mvprintw(4 + i, 3, " %s", rows[i]);
                attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
            }
        }

        attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK) | A_DIM);
        mvprintw(LINES - 1, 3, "up/dn move   left/right change   Esc save & exit");
        attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK) | A_DIM);
        refresh();

        int ch = getch();
        if (ch == KEY_RESIZE) { ui_on_resize(); continue; }
        if (ch == 27) { settings_save(s); return; }
        if (ch == KEY_UP)   sel = (sel - 1 + nrows) % nrows;
        if (ch == KEY_DOWN) sel = (sel + 1) % nrows;

        if (ch == KEY_LEFT || ch == KEY_RIGHT) {
            int d = (ch == KEY_RIGHT) ? 1 : -1;
            if (sel == 0) {
                s->hardcore = !s->hardcore;
            } else if (sel == 1) {
                s->mode = (PracticeMode)(((int)s->mode + d + 3) % 3);
                if (sel >= nrows) sel = nrows - 1;
            } else if (sel == 2) {
                s->time_limit_sec += d * 15;
                if (s->time_limit_sec < 15)  s->time_limit_sec = 15;
                if (s->time_limit_sec > 300) s->time_limit_sec = 300;
            }
        }
    }
}
// GCOVR_EXCL_STOP
