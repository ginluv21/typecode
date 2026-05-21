#include <ncurses.h>
#include <stdlib.h>
#include <stdio.h>
#include "ui.h"
#include "typecode.h"

void ui_init(void) // инициализирует ncurses, включает цвета, регистрирует 6 цветовых пар
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    if (!has_colors()) {
        endwin();
        fprintf(stderr, "typecode: terminal does not support colors\n");
        exit(1);
    }

    start_color();
    use_default_colors();
    init_pair(COLOR_GREEN_ON_BLACK,  COLOR_GREEN,  -1);
    init_pair(COLOR_RED_ON_BLACK,    COLOR_RED,    -1);
    init_pair(COLOR_YELLOW_ON_BLACK, COLOR_YELLOW, -1);
    init_pair(COLOR_WHITE_ON_BLACK,  COLOR_WHITE,  -1);
    init_pair(COLOR_CYAN_ON_BLACK,   COLOR_CYAN,   -1);
    init_pair(COLOR_BLUE_ON_BLACK,   COLOR_BLUE,   -1);
}

void ui_cleanup(void) // завершает ncurses, возвращает терминал в нормальный режим
{
    endwin();
}

void ui_on_resize(void) // вызывается при KEY_RESIZE: обновляет размер stdscr, восстанавливает настройки
{
    wresize(stdscr, LINES, COLS);
    clearok(stdscr, TRUE);
    keypad(stdscr, TRUE);
    curs_set(0);
}

int ui_too_small(void)
{
    if (LINES >= 24 && COLS >= 80) return 0;
    clear();
    attron(COLOR_PAIR(COLOR_YELLOW_ON_BLACK) | A_BOLD);
    int msg_col = (COLS - 42) / 2;
    if (msg_col < 0) msg_col = 0;
    mvprintw(LINES / 2, msg_col,
             "Terminal too small (min 80x24, now %dx%d)", COLS, LINES);
    attroff(COLOR_PAIR(COLOR_YELLOW_ON_BLACK) | A_BOLD);
    refresh();
    return 1;
}

int tui_readline(int row, int col, int maxlen, char *out)
{
    if (maxlen <= 0 || !out) return 0;

    curs_set(1);
    int pos = 0;
    out[0] = '\0';
    move(row, col);
    clrtoeol();
    refresh();

    while (1) {
        int ch = getch();
        if (ch == 27) {
            out[0] = '\0';
            break;
        }
        if (ch == '\n' || ch == KEY_ENTER) {
            break;
        }
        if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
            if (pos > 0) {
                pos--;
                out[pos] = '\0';
                mvaddch(row, col + pos, ' ');
                move(row, col + pos);
            }
        } else if (ch >= 32 && ch < 127) {
            if (pos < maxlen - 1) {
                out[pos++] = (char)ch;
                out[pos] = '\0';
                mvaddch(row, col + pos - 1, ch);
                move(row, col + pos);
            }
        }
        refresh();
    }

    curs_set(0);
    return out[0] != '\0' || (getcurx(stdscr) != col);
}
