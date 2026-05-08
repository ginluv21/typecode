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

void draw_main_menu(int selected)
{
    int row = (LINES - MENU_HEIGHT) / 2;
    int col = (COLS  - MENU_WIDTH)  / 2;

    clear();

    /* box */
    attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
    mvprintw(row, col, "╔");
    for (int i = 1; i < MENU_WIDTH - 1; i++)
        mvprintw(row, col + i, "═");
    mvprintw(row, col + MENU_WIDTH - 1, "╗");

    /* title row */
    mvprintw(row + 1, col, "║");
    attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

    attron(COLOR_PAIR(COLOR_YELLOW_ON_BLACK) | A_BOLD);
    mvprintw(row + 1, col + (MENU_WIDTH - 8) / 2, "typecode");
    attroff(COLOR_PAIR(COLOR_YELLOW_ON_BLACK) | A_BOLD);

    attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
    mvprintw(row + 1, col + MENU_WIDTH - 1, "║");

    /* separator */
    mvprintw(row + 2, col, "╠");
    for (int i = 1; i < MENU_WIDTH - 1; i++)
        mvprintw(row + 2, col + i, "═");
    mvprintw(row + 2, col + MENU_WIDTH - 1, "╣");
    attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

    /* menu items */
    for (int i = 0; i < MENU_ITEMS; i++) {
        attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
        mvprintw(row + 3 + i, col, "║");
        mvprintw(row + 3 + i, col + MENU_WIDTH - 1, "║");
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
    mvprintw(row + 9, col, "╚");
    for (int i = 1; i < MENU_WIDTH - 1; i++)
        mvprintw(row + 9, col + i, "═");
    mvprintw(row + 9, col + MENU_WIDTH - 1, "╝");
    attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

    /* hint */
    attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
    mvprintw(row + MENU_HEIGHT + 1, col + 4, "↑↓ move   Enter select   q quit");
    attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));

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
            case KEY_RESIZE:
                break;
        }
        draw_main_menu(selected);
    }

    return selected;
}
