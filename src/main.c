#include <ncurses.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include "typecode.h"
#include "menu.h"

static volatile sig_atomic_t g_resized = 0;

static void handle_sigwinch(int sig)
{
    (void)sig;
    g_resized = 1;
}

static void init_ncurses(void)
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
    init_pair(COLOR_GREEN_ON_BLACK,  COLOR_GREEN,  COLOR_BLACK);
    init_pair(COLOR_RED_ON_BLACK,    COLOR_RED,    COLOR_BLACK);
    init_pair(COLOR_YELLOW_ON_BLACK, COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLOR_WHITE_ON_BLACK,  COLOR_WHITE,  COLOR_BLACK);
    init_pair(COLOR_CYAN_ON_BLACK,   COLOR_CYAN,   COLOR_BLACK);
}

int main(void)
{
    setlocale(LC_ALL, "");
    signal(SIGWINCH, handle_sigwinch);

    init_ncurses();
    menu_run();
    endwin();

    return 0;
}
