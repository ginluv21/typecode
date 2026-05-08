#include <ncurses.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

#define COLOR_GREEN_ON_BLACK  1
#define COLOR_RED_ON_BLACK    2
#define COLOR_YELLOW_ON_BLACK 3
#define COLOR_WHITE_ON_BLACK  4
#define COLOR_CYAN_ON_BLACK   5

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

static void handle_resize(void)
{
    g_resized = 0;
    endwin();
    refresh();
    clear();
}

int main(void)
{
    signal(SIGWINCH, handle_sigwinch);

    init_ncurses();

    /* placeholder until menu_run() is implemented in menu.c */
    mvprintw(LINES / 2, (COLS - 24) / 2, "typecode — press q to quit");
    refresh();

    int ch;
    while ((ch = getch()) != 'q') {
        if (g_resized) {
            handle_resize();
            mvprintw(LINES / 2, (COLS - 24) / 2, "typecode — press q to quit");
            refresh();
        }
    }

    endwin();
    return 0;
}
