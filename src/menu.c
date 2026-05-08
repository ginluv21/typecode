#include <ncurses.h>
#include "menu.h"
#include "typecode.h"

#define MENU_ITEMS  6
#define MENU_WIDTH  32
#define MENU_HEIGHT 10

static const char *items[MENU_ITEMS] = {
    "Lessons",
    "Programming Languages",
    "Practice",
    "Statistics",
    "Settings",
    "Exit",
};

static void draw_hline(int row, int col, chtype left, chtype right)
{
    mvaddch(row, col, left);
    for (int i = 1; i < MENU_WIDTH - 1; i++)
        mvaddch(row, col + i, ACS_HLINE);
    mvaddch(row, col + MENU_WIDTH - 1, right);
}

void draw_main_menu(int selected)
{
    int row = (LINES - MENU_HEIGHT) / 2;
    int col = (COLS  - MENU_WIDTH)  / 2;

    clear();

    /* top border */
    attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
    draw_hline(row, col, ACS_ULCORNER, ACS_URCORNER);

    /* title row */
    mvaddch(row + 1, col, ACS_VLINE);
    attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

    attron(COLOR_PAIR(COLOR_YELLOW_ON_BLACK) | A_BOLD);
    mvprintw(row + 1, col + (MENU_WIDTH - 8) / 2, "typecode");
    attroff(COLOR_PAIR(COLOR_YELLOW_ON_BLACK) | A_BOLD);

    attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
    mvaddch(row + 1, col + MENU_WIDTH - 1, ACS_VLINE);

    /* separator */
    draw_hline(row + 2, col, ACS_LTEE, ACS_RTEE);
    attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

    /* menu items */
    for (int i = 0; i < MENU_ITEMS; i++) {
        attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
        mvaddch(row + 3 + i, col, ACS_VLINE);
        mvaddch(row + 3 + i, col + MENU_WIDTH - 1, ACS_VLINE);
        attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

        if (i == selected) {
            attron(COLOR_PAIR(COLOR_GREEN_ON_BLACK) | A_BOLD | A_REVERSE);
            mvprintw(row + 3 + i, col + 1, " %-*s", MENU_WIDTH - 3, items[i]);
            attroff(COLOR_PAIR(COLOR_GREEN_ON_BLACK) | A_BOLD | A_REVERSE);
        } else {
            attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
            mvprintw(row + 3 + i, col + 1, " %-*s", MENU_WIDTH - 3, items[i]);
            attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        }
    }

    /* bottom border */
    attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
    draw_hline(row + 9, col, ACS_LLCORNER, ACS_LRCORNER);
    attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

    /* hint */
    mvprintw(row + MENU_HEIGHT + 1, col + 3, "up/dn move   Enter select   q quit");

    refresh();
}

int menu_run(void)
{
    int selected = 0;
    draw_main_menu(selected);

    int ch;
    while ((ch = getch()) != 'q') {
        switch (ch) {
            case KEY_UP:
                selected = (selected - 1 + MENU_ITEMS) % MENU_ITEMS;
                break;
            case KEY_DOWN:
                selected = (selected + 1) % MENU_ITEMS;
                break;
            case '\n':
            case KEY_ENTER:
                if (selected == MENU_ITEMS - 1)
                    return selected;
                break;
            case KEY_RESIZE:
                break;
        }
        draw_main_menu(selected);
    }

    return selected;
}
