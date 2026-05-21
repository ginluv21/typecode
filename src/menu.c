#include <ncurses.h>
#include <string.h>
#include "menu.h"
#include "typecode.h"
#include "ui.h"

#define LOGO_LINES      5
#define MENU_ITEMS      6
#define MENU_WIDTH      50
#define MENU_HEIGHT     14  // top + 5 logo + sep + 6 items + bottom

// цвета Google/Rubik - по одному на каждую строку лого
static const int logo_colors[LOGO_LINES] = {
    COLOR_RED_ON_BLACK,
    COLOR_BLUE_ON_BLACK,
    COLOR_YELLOW_ON_BLACK,
    COLOR_GREEN_ON_BLACK,
    COLOR_CYAN_ON_BLACK,
};

static const char *logo[LOGO_LINES] = {
    " _                              _",
    "| |_ _  _ _ __  ___  __ ___  __| | ___",
    "|  _| || | '_ \\/ -_)/ _/ _ \\/ _` |/ -_)",
    " \\__|\\_, | .__/\\___|\\__\\___/\\__,_|\\___|",
    "     |__/|_|",
};

static const char *items[MENU_ITEMS] = {
    "Lessons",
    "Programming Languages",
    "Practice",
    "Statistics",
    "Settings",
    "Exit",
};

static void draw_hline(int row, int col, chtype left, chtype right) // рисует горизонтальную линию рамки с заданными угловыми символами
{
    mvaddch(row, col, left);
    for (int i = 1; i < MENU_WIDTH - 1; i++)
        mvaddch(row, col + i, ACS_HLINE);
    mvaddch(row, col + MENU_WIDTH - 1, right);
}

void draw_main_menu(int selected) // рисует рамку с лого и пунктами меню, выделяет выбранный пункт
{
    int row = (LINES - MENU_HEIGHT) / 2;
    int col = (COLS  - MENU_WIDTH)  / 2;

    int max_logo_w = 0;
    for (int i = 0; i < LOGO_LINES; i++) {
        int len = (int)strlen(logo[i]);
        if (len > max_logo_w) max_logo_w = len;
    }
    int logo_col = col + 1 + (MENU_WIDTH - 2 - max_logo_w) / 2;

    clear();

    // верхняя граница рамки
    attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
    draw_hline(row, col, ACS_ULCORNER, ACS_URCORNER);
    attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

    // строки лого - по центру, каждая в своём цвете
    for (int i = 0; i < LOGO_LINES; i++) {
        attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
        mvaddch(row + 1 + i, col, ACS_VLINE);
        mvaddch(row + 1 + i, col + MENU_WIDTH - 1, ACS_VLINE);
        attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

        attron(COLOR_PAIR(logo_colors[i]) | A_BOLD);
        mvprintw(row + 1 + i, logo_col, "%s", logo[i]);
        attroff(COLOR_PAIR(logo_colors[i]) | A_BOLD);

        // восстановить линии рамки, перекрытые лого
        attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
        mvaddch(row + 1 + i, col, ACS_VLINE);
        mvaddch(row + 1 + i, col + MENU_WIDTH - 1, ACS_VLINE);
        attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
    }

    // разделитель
    attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
    draw_hline(row + 6, col, ACS_LTEE, ACS_RTEE);
    attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

    // пункты меню
    for (int i = 0; i < MENU_ITEMS; i++) {
        attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
        mvaddch(row + 7 + i, col, ACS_VLINE);
        mvaddch(row + 7 + i, col + MENU_WIDTH - 1, ACS_VLINE);
        attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

        if (i == selected) {
            attron(COLOR_PAIR(COLOR_GREEN_ON_BLACK) | A_BOLD | A_REVERSE);
            mvprintw(row + 7 + i, col + 1, " %d. %s", i + 1, items[i]);
            attroff(COLOR_PAIR(COLOR_GREEN_ON_BLACK) | A_BOLD | A_REVERSE);
        } else {
            attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
            mvprintw(row + 7 + i, col + 1, " %d. %s", i + 1, items[i]);
            attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        }
    }

    // нижняя граница рамки
    attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
    draw_hline(row + MENU_HEIGHT - 1, col, ACS_LLCORNER, ACS_LRCORNER);
    attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

    // подсказка по клавишам
    mvprintw(row + MENU_HEIGHT + 1, col + 3, "up/dn move   Enter select   q quit");

    refresh();
}

MenuOption menu_run(void) // цикл ввода главного меню, возвращает выбранный пункт MenuOption
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
                return (MenuOption)selected;
            case '1': case '2': case '3':
            case '4': case '5': case '6':
                return (MenuOption)(ch - '1');
            case KEY_RESIZE:
                ui_on_resize();
                if (ui_too_small()) continue;
                draw_main_menu(selected);
                continue;
        }
        draw_main_menu(selected);
    }

    return MENU_EXIT;
}
