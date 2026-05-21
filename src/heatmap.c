#define _DEFAULT_SOURCE
#include <ctype.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "heatmap.h"
#include "lesson.h"
#include "settings.h"
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

static int heatmap_compare(const void *a, const void *b)
{
    const int *ia = a;
    const int *ib = b;
    return ib[1] - ia[1];
}

static void heatmap_symbol_label(char *out, size_t size, int code)
{
    if (code == ' ') {
        snprintf(out, size, "space");
    } else if (isprint(code)) {
        snprintf(out, size, "%c", (char)code);
    } else {
        snprintf(out, size, "%d", code);
    }
}

static void heatmap_add_pattern(char *out, size_t size, int code)
{
    if (!isprint(code)) {
        strncat(out, " ", size - strlen(out) - 1);
        return;
    }

    char chunk[64] = "";
    switch (code) {
        case ';':
            snprintf(chunk, sizeof(chunk), "a;%s s;%s d;%s f;%s ", ";", ";", ";", ";");
            break;
        case '[':
            snprintf(chunk, sizeof(chunk), "a[[ s[[ d[] f[1] ");
            break;
        case ']':
            snprintf(chunk, sizeof(chunk), "a]] s]] d]] f]1 ");
            break;
        case '\\':
            snprintf(chunk, sizeof(chunk), "a\\a s\\s d\\\\ f\\1 ");
            break;
        case ',':
            snprintf(chunk, sizeof(chunk), ",, ,., ,,, ,m, ,n, ");
            break;
        case '.':
            snprintf(chunk, sizeof(chunk), ".. ,. .,. /., ");
            break;
        case '/':
            snprintf(chunk, sizeof(chunk), "/// ./. // /? ");
            break;
        default:
            if (isalpha(code)) {
                char peer = (code == 'f') ? 'j' : (code == 'j') ? 'f' : 'k';
                snprintf(chunk, sizeof(chunk), "%c%c%c %c%c%c %c%c%c %c%c%c ", (char)code, (char)code, (char)code,
                         (char)code, peer, (char)code,
                         (char)code, 'a', (char)code,
                         (char)code, 's', (char)code);
            } else if (isdigit(code)) {
                snprintf(chunk, sizeof(chunk), "%c%c%c %c%c%c %c%c%c ", (char)code, (char)code, (char)code,
                         (char)code, '1', (char)code,
                         '0', (char)code, '0');
            } else {
                snprintf(chunk, sizeof(chunk), "%c%c %c%c ", (char)code, (char)code, (char)code, (char)code);
            }
            break;
    }
    strncat(out, chunk, size - strlen(out) - 1);
}

char *heatmap_generate_exercise(const Heatmap *h, int top_n)
{
    if (!h || top_n <= 0) return NULL;

    int symbols[128][2];
    int count = 0;
    for (int i = 0; i < 128; i++) {
        if (h->counts[i] > 0) {
            symbols[count][0] = i;
            symbols[count][1] = h->counts[i];
            count++;
        }
    }

    if (count == 0) {
        char *fallback = malloc(256);
        if (!fallback) return NULL;
        snprintf(fallback, 256, "asdf jkl; asdf jkl; asdf jkl;\n");
        return fallback;
    }

    qsort(symbols, count, sizeof(symbols[0]), heatmap_compare);
    if (top_n > count) top_n = count;

    int lines = 7;
    size_t buffer_size = lines * 128 + 1;
    char *result = malloc(buffer_size);
    if (!result) return NULL;
    result[0] = '\0';

    for (int row = 0; row < lines; row++) {
        for (int i = 0; i < top_n; i++) {
            heatmap_add_pattern(result, buffer_size, symbols[i][0]);
        }
        strncat(result, "\n", buffer_size - strlen(result) - 1);
    }

    return result;
}

void heatmap_draw_weakspots(const Heatmap *h)
{
    int item_count = 0;
    int values[128][2];
    for (int i = 0; i < 128; i++) {
        if (h->counts[i] > 0) {
            values[item_count][0] = i;
            values[item_count][1] = h->counts[i];
            item_count++;
        }
    }

    if (item_count > 0)
        qsort(values, item_count, sizeof(values[0]), heatmap_compare);

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
        mvprintw(1, 4, "Your weak spots");
        mvhline(2, 0, ACS_HLINE, COLS);
        attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

        int row = 4;
        mvprintw(row++, 4, "");
        mvprintw(row++, 4, "  Top problem symbols:");
        mvprintw(row++, 4, "");

        int top = item_count < 10 ? item_count : 10;
        for (int i = 0; i < top; i++) {
            char label[16];
            heatmap_symbol_label(label, sizeof(label), values[i][0]);
            attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
            mvprintw(row++, 4, "  %d. %-3s - %d errors", i + 1, label, values[i][1]);
            attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        }

        row += 1;
        attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        mvprintw(row++, 4, "");
        mvprintw(row++, 4, "  G - generate exercise");
        mvprintw(row++, 4, "  Esc - back");
        attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));

        refresh();

        int ch = getch();
        if (ch == KEY_RESIZE) {
            ui_on_resize();
            continue;
        }
        if (ch == 27) break;
        if (ch == 'g' || ch == 'G') {
            char *exercise = heatmap_generate_exercise(h, 5);
            if (exercise) {
                lesson_run_text(exercise, "Weak Spots Exercise", 0, MODE_NORMAL, 0);
                free(exercise);
            }
            continue;
        }
    }
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
