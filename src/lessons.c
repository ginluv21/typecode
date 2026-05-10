#define _POSIX_C_SOURCE 200809L
#include <ncurses.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "lesson.h"
#include "typecode.h"

static const char *languages[] = {
    "c",
    "python",
    "javascript",
    "bash",
    "go",
};

static const char *language_labels[] = {
    "C",
    "Python",
    "JavaScript",
    "Bash",
    "Go",
};

const char *language_select_menu(void) {
    int count = sizeof(languages) / sizeof(languages[0]);
    int selected = 0;
    int ch;

    while (1) {
        clear();

        int w = 44;
        int h = 11;
        int row = (LINES - h) / 2;
        int col = (COLS - w) / 2;

        attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
        mvaddch(row, col, ACS_ULCORNER);
        mvhline(row, col + 1, ACS_HLINE, w - 2);
        mvaddch(row, col + w - 1, ACS_URCORNER);
        for (int i = 1; i < h - 1; i++) {
            mvaddch(row + i, col, ACS_VLINE);
            mvaddch(row + i, col + w - 1, ACS_VLINE);
        }
        mvaddch(row + h - 1, col, ACS_LLCORNER);
        mvhline(row + h - 1, col + 1, ACS_HLINE, w - 2);
        mvaddch(row + h - 1, col + w - 1, ACS_LRCORNER);
        attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

        attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK) | A_BOLD);
        mvprintw(row + 1, col + 3, "Programming Languages");
        attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK) | A_BOLD);

        attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
        mvhline(row + 2, col + 1, ACS_HLINE, w - 2);
        attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

        for (int i = 0; i < count; i++) {
            if (i == selected) {
                attron(COLOR_PAIR(COLOR_GREEN_ON_BLACK) | A_BOLD | A_REVERSE);
                mvprintw(row + 3 + i, col + 3, " %d. %s", i + 1, language_labels[i]);
                attroff(COLOR_PAIR(COLOR_GREEN_ON_BLACK) | A_BOLD | A_REVERSE);
            } else {
                attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
                mvprintw(row + 3 + i, col + 3, " %d. %s", i + 1, language_labels[i]);
                attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
            }
        }

        attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        mvprintw(row + h - 2, col + 3, "Enter - выбрать язык   Esc - назад");
        attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));

        refresh();

        ch = getch();
        if (ch == 27) return NULL;
        if (ch == KEY_UP) selected = (selected - 1 + count) % count;
        if (ch == KEY_DOWN) selected = (selected + 1) % count;
        if (ch == '\n' || ch == KEY_ENTER) return languages[selected];
        if (ch >= '1' && ch <= '0' + count) return languages[ch - '1'];
    }
}

LessonList *lessons_scan(const char *dir) {
    LessonList *list = malloc(sizeof(LessonList));
    if (!list) return NULL;
    list->count = 0;

    DIR *d = opendir(dir);
    if (!d) { free(list); return NULL; }

    struct dirent *ent;
    while ((ent = readdir(d)) && list->count < MAX_LESSONS) {
        char *dot = strrchr(ent->d_name, '.');
        if (!dot || strcmp(dot, ".txt") != 0) continue;

        LessonEntry *entry = &list->entries[list->count];
        snprintf(entry->path, LESSON_PATH_MAX, "%s/%s", dir, ent->d_name);

        strncpy(entry->name, ent->d_name, LESSON_NAME_MAX - 1);
        entry->name[LESSON_NAME_MAX - 1] = '\0';
        char *d2 = strrchr(entry->name, '.');
        if (d2) *d2 = '\0';
        for (char *p = entry->name; *p; p++)
            if (*p == '_') *p = ' ';

        list->count++;
    }
    closedir(d);

    if (list->count == 0) {
        free(list);
        return NULL;
    }

    for (int i = 0; i < list->count - 1; i++) {
        for (int j = i + 1; j < list->count; j++) {
            if (strcmp(list->entries[i].path, list->entries[j].path) > 0) {
                LessonEntry tmp = list->entries[i];
                list->entries[i] = list->entries[j];
                list->entries[j] = tmp;
            }
        }
    }

    return list;
}

void lessons_free(LessonList *list) {
    free(list);
}

int lessons_run_menu(const char *dir) {
    LessonList *list = lessons_scan(dir);
    if (!list) return 0;

    int selected = 0;
    int ch;

    while (1) {
        clear();

        attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
        mvhline(0, 0, ACS_HLINE, COLS);
        mvprintw(1, 3, "Lessons");
        mvhline(2, 0, ACS_HLINE, COLS);
        attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));

        for (int i = 0; i < list->count; i++) {
            if (i == selected) {
                attron(COLOR_PAIR(COLOR_GREEN_ON_BLACK) | A_BOLD | A_REVERSE);
                mvprintw(4 + i, 3, " %s ", list->entries[i].name);
                attroff(COLOR_PAIR(COLOR_GREEN_ON_BLACK) | A_BOLD | A_REVERSE);
            } else {
                attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
                mvprintw(4 + i, 3, " %s ", list->entries[i].name);
                attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
            }
        }

        attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        mvprintw(LINES - 1, 3, "up/dn - move   Enter - start   Esc - back");
        attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));

        refresh();

        ch = getch();
        if (ch == 27) break;
        if (ch == KEY_UP) selected = (selected - 1 + list->count) % list->count;
        if (ch == KEY_DOWN) selected = (selected + 1) % list->count;
        if (ch == '\n' || ch == KEY_ENTER) {
            ResultAction a = lesson_run(list->entries[selected].path);
            if (a == RESULT_MAIN_MENU) break;
        }
    }

    lessons_free(list);
    return 1;
}
