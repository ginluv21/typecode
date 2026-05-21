#define _DEFAULT_SOURCE
#include <ctype.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "heatmap.h"
#include "typecode.h"
#include "ui.h"

Heatmap global_heatmap = {0};

static void build_path(char *buf, size_t size, const char *suffix)
{
    const char *home = getenv("HOME");
    snprintf(buf, size, "%s%s", home ? home : "", suffix);
}

static void ensure_heatmap_dir(void)
{
    const char *home = getenv("HOME");
    if (!home) return;
    char path[512];
    snprintf(path, sizeof(path), "%s/.typecode", home);
    mkdir(path, 0755);
}

void heatmap_load(Heatmap *h)
{
    if (!h) return;
    memset(h, 0, sizeof(*h));
    ensure_heatmap_dir();

    char path[512];
    build_path(path, sizeof(path), HEATMAP_FILE);
    FILE *f = fopen(path, "r");
    if (!f) return;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char *sep = strchr(line, ':');
        if (!sep) continue;
        *sep = '\0';
        char *value = sep + 1;
        int count = atoi(value);
        if (count < 0) count = 0;

        int index = -1;
        if (line[0] && line[1] == '\0') {
            unsigned char ch = (unsigned char)line[0];
            if (ch < 128) index = ch;
        } else {
            char *end;
            long code = strtol(line, &end, 10);
            if (end != line && *end == '\0' && code >= 0 && code < 128)
                index = (int)code;
        }

        if (index >= 0)
            h->counts[index] = count;
    }

    fclose(f);
}

void heatmap_save(const Heatmap *h)
{
    if (!h) return;
    ensure_heatmap_dir();

    char path[512];
    build_path(path, sizeof(path), HEATMAP_FILE);
    FILE *f = fopen(path, "w");
    if (!f) return;

    for (int i = 0; i < 128; i++) {
        if (h->counts[i] <= 0) continue;
        if (isprint(i) && i != ':')
            fprintf(f, "%c:%d\n", (char)i, h->counts[i]);
        else
            fprintf(f, "%d:%d\n", i, h->counts[i]);
    }

    fclose(f);
}

void heatmap_record_error(Heatmap *h, char expected, char typed)
{
    (void)typed;
    if (!h) return;
    unsigned char index = (unsigned char)expected;
    if (index >= 128) return;
    h->counts[index]++;
}

void heatmap_reset(Heatmap *h)
{
    if (!h) return;
    memset(h, 0, sizeof(*h));
    heatmap_save(h);
}

static int heatmap_color(int count)
{
    if (count == 0) return COLOR_WHITE_ON_BLACK;
    if (count <= 3) return COLOR_GREEN_ON_BLACK;
    if (count <= 9) return COLOR_YELLOW_ON_BLACK;
    return COLOR_RED_ON_BLACK;
}

static int heatmap_bold(int count)
{
    return count >= 10;
}

void heatmap_draw_screen(const Heatmap *h)
{
    static const char *rows[] = {
        "`1234567890-=",
        "qwertyuiop[]\\",
        "asdfghjkl;'",
        "zxcvbnm,./"
    };
    static const int indents[] = {2, 1, 2, 3};

    for (;;) {
        if (ui_too_small()) {
            int c = getch();
            if (c == KEY_RESIZE) ui_on_resize();
            else if (c == 27) break;
            continue;
        }

        clear();
        attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
        mvhline(0, 0, ACS_HLINE, COLS);
        mvprintw(1, 4, "Keyboard Heatmap");
        mvhline(2, 0, ACS_HLINE, COLS);
        attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

        int row = 4;
        int base_col = 4;

        for (int r = 0; r < 4; r++) {
            int x = base_col + indents[r];
            const char *line = rows[r];
            for (int j = 0; line[j] != '\0'; j++) {
                unsigned char key = (unsigned char)line[j];
                int count = (key < 128) ? h->counts[key] : 0;
                int color = heatmap_color(count);
                attron(COLOR_PAIR(color));
                if (heatmap_bold(count)) attron(A_BOLD);
                mvaddch(row, x, (chtype)key);
                if (heatmap_bold(count)) attroff(A_BOLD);
                attroff(COLOR_PAIR(color));
                x++;
                if (line[j + 1] != '\0') {
                    mvaddch(row, x, ' ');
                    x++;
                }
            }
            row += 2;
        }

        attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        mvprintw(row + 1, base_col, "Color: green < 3 err.  yellow < 10  red >= 10");
        mvprintw(row + 3, base_col, "R - reset   Esc - back");
        attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));

        refresh();

        int ch = getch();
        if (ch == KEY_RESIZE) {
            ui_on_resize();
            continue;
        }
        if (ch == 'r' || ch == 'R') {
            heatmap_reset(&global_heatmap);
            continue;
        }
        if (ch == 27) break;
    }
}
