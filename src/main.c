#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include "typecode.h"
#include "ui.h"
#include "menu.h"
#include "lesson.h"
#include "stats.h"
#include "heatmap.h"
#include "settings.h"
#include "config.h"

#define FILE_PATH_MAX 512
#define MAX_DISPLAY_LINES 500
#define WARNING_FILE_SIZE (50 * 1024)

static void draw_box(int row, int col, int height, int width)
{
    attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
    mvaddch(row, col, ACS_ULCORNER);
    mvaddch(row, col + width - 1, ACS_URCORNER);
    mvaddch(row + height - 1, col, ACS_LLCORNER);
    mvaddch(row + height - 1, col + width - 1, ACS_LRCORNER);
    for (int x = col + 1; x < col + width - 1; x++) {
        mvaddch(row, x, ACS_HLINE);
        mvaddch(row + height - 1, x, ACS_HLINE);
    }
    for (int y = row + 1; y < row + height - 1; y++) {
        mvaddch(y, col, ACS_VLINE);
        mvaddch(y, col + width - 1, ACS_VLINE);
    }
    mvaddch(row + 2, col, ACS_LTEE);
    mvaddch(row + 2, col + width - 1, ACS_RTEE);
    for (int x = col + 1; x < col + width - 1; x++)
        mvaddch(row + 2, x, ACS_HLINE);
    attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK));
}

static const char *path_basename(const char *path)
{
    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

static void show_message_box(const char *title, const char *line1, const char *line2)
{
    int width = 60;
    int height = 9;
    int row = (LINES - height) / 2;
    int col = (COLS - width) / 2;

    clear();
    draw_box(row, col, height, width);
    attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK) | A_BOLD);
    mvprintw(row + 1, col + 3, "%s", title);
    attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK) | A_BOLD);
    attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
    mvprintw(row + 4, col + 3, "%s", line1);
    if (line2) mvprintw(row + 5, col + 3, "%s", line2);
    mvprintw(row + 7, col + 3, "Press Enter to continue");
    attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
    refresh();
    int ch;
    while ((ch = getch()) != '\n' && ch != KEY_ENTER) {
        if (ch == 27) break;
    }
}

static int prompt_yes_no(const char *message, int default_yes)
{
    int width = 64;
    int height = 9;
    int row = (LINES - height) / 2;
    int col = (COLS - width) / 2;

    clear();
    draw_box(row, col, height, width);
    attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK) | A_BOLD);
    mvprintw(row + 1, col + 3, "Confirmation");
    attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK) | A_BOLD);
    attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
    mvprintw(row + 4, col + 3, "%s", message);
    mvprintw(row + 6, col + 3, default_yes ? "Press Y/Enter to continue, N to cancel" : "Press N/Enter to cancel, Y to continue");
    attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
    refresh();

    int ch;
    while (1) {
        ch = getch();
        if (ch == 'y' || ch == 'Y') return 1;
        if (ch == 'n' || ch == 'N') return 0;
        if (ch == '\n' || ch == KEY_ENTER) return default_yes;
        if (ch == 27) return 0;
    }
}

static int prompt_file_path(char *out, int maxlen)
{
    int width = 60;
    int height = 10;
    int row = (LINES - height) / 2;
    int col = (COLS - width) / 2;

    while (1) {
        clear();
        draw_box(row, col, height, width);
        attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK) | A_BOLD);
        mvprintw(row + 1, col + 3, "Load custom file");
        attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK) | A_BOLD);
        attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        mvprintw(row + 4, col + 3, "Enter file path:");
        mvprintw(row + 5, col + 3, "> ");
        mvprintw(row + 8, col + 3, "Esc - back");
        attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        refresh();

        if (!tui_readline(row + 5, col + 5, maxlen, out))
            return 0;

        if (out[0] == '\0') {
            show_message_box("Ошибка", "Путь не может быть пустым.", "Нажмите Enter, чтобы попробовать снова.");
            continue;
        }
        return 1;
    }
}

static char *create_truncated_temp_file(const char *path)
{
    char template[] = "/tmp/typecode-XXXXXX";
    int fd = mkstemp(template);
    if (fd < 0) return NULL;

    FILE *in = fopen(path, "r");
    if (!in) { close(fd); return NULL; }
    FILE *out = fdopen(fd, "w");
    if (!out) { fclose(in); close(fd); return NULL; }

    char buffer[1024];
    int lines = 0;
    while (lines < MAX_DISPLAY_LINES && fgets(buffer, sizeof(buffer), in)) {
        fputs(buffer, out);
        lines++;
    }

    fclose(in);
    fclose(out);
    return strdup(template);
}

static void practice_load_file(const Settings *s)
{
    char path[FILE_PATH_MAX];

    while (1) {
        if (!prompt_file_path(path, sizeof(path)))
            return;

        if (access(path, R_OK) != 0) {
            show_message_box("Ошибка", "Файл не найден или недоступен для чтения.", "Нажмите Enter, чтобы попробовать снова.");
            continue;
        }

        FILE *file = fopen(path, "r");
        if (!file) {
            show_message_box("Ошибка", "Не удалось открыть файл.", "Нажмите Enter, чтобы попробовать снова.");
            continue;
        }

        if (fseek(file, 0, SEEK_END) != 0) {
            fclose(file);
            show_message_box("Ошибка", "Не удалось получить размер файла.", "Нажмите Enter, чтобы попробовать снова.");
            continue;
        }

        long size = ftell(file);
        if (size <= 0) {
            fclose(file);
            show_message_box("Ошибка", "Файл пустой.", "Нажмите Enter, чтобы попробовать снова.");
            continue;
        }

        if (size > WARNING_FILE_SIZE) {
            if (!prompt_yes_no("Файл больше 50 КБ. Продолжить?", 1)) {
                fclose(file);
                continue;
            }
        }

        rewind(file);
        int line_count = 0;
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), file))
            line_count++;
        fclose(file);

        const char *display_name = path_basename(path);
        if (line_count > MAX_DISPLAY_LINES) {
            char prompt[128];
            snprintf(prompt, sizeof(prompt), "Файл большой (%d строк). Загрузить первые %d?", line_count, MAX_DISPLAY_LINES);
            if (!prompt_yes_no(prompt, 1))
                continue;

            char *temp_path = create_truncated_temp_file(path);
            if (!temp_path) {
                show_message_box("Ошибка", "Не удалось создать временный файл.", "Нажмите Enter, чтобы попробовать снова.");
                continue;
            }

            lesson_run_with_name(temp_path, display_name, s);
            unlink(temp_path);
            free(temp_path);
            return;
        }

        lesson_run_with_name(path, display_name, s);
        return;
    }
}

static void draw_practice_menu(int selected)
{
    int height = 10;
    int width = 46;
    int row = (LINES - height) / 2;
    int col = (COLS - width) / 2;

    clear();
    draw_box(row, col, height, width);
    attron(COLOR_PAIR(COLOR_CYAN_ON_BLACK) | A_BOLD);
    mvprintw(row + 1, col + 3, "Practice");
    attroff(COLOR_PAIR(COLOR_CYAN_ON_BLACK) | A_BOLD);

    const char *items[] = {"Load file", "Back"};
    for (int i = 0; i < 2; i++) {
        if (i == selected) {
            attron(COLOR_PAIR(COLOR_GREEN_ON_BLACK) | A_BOLD | A_REVERSE);
            mvprintw(row + 3 + i, col + 3, " %s", items[i]);
            attroff(COLOR_PAIR(COLOR_GREEN_ON_BLACK) | A_BOLD | A_REVERSE);
        } else {
            attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
            mvprintw(row + 3 + i, col + 3, " %s", items[i]);
            attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
        }
    }

    attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
    mvprintw(row + height - 2, col + 3, "Use arrows and Enter. Esc to return.");
    attroff(COLOR_PAIR(COLOR_WHITE_ON_BLACK));
    refresh();
}

static int practice_menu(void)
{
    int selected = 0;
    draw_practice_menu(selected);

    int ch;
    while ((ch = getch()) != 27) {
        switch (ch) {
            case KEY_UP:
                selected = (selected - 1 + 2) % 2;
                break;
            case KEY_DOWN:
                selected = (selected + 1) % 2;
                break;
            case '\n':
            case KEY_ENTER:
                return selected;
            case KEY_RESIZE:
                ui_on_resize();
                if (ui_too_small()) continue;
                draw_practice_menu(selected);
                continue;
        }
        draw_practice_menu(selected);
    }
    return 1;
}

int main(void) // точка входа: инит ncurses, главный цикл меню до MENU_EXIT
{
    stats_ensure_dir();
    ui_init();
    heatmap_load(&global_heatmap);

    Settings g_settings;
    settings_load(&g_settings);
    AppConfig g_config;
    config_load(&g_config);

    MenuOption choice;
    do {
        choice = menu_run(&g_config);
        if (choice == MENU_LESSONS) {
            lesson_select_menu("lessons/latin", &g_settings);
        } else if (choice == MENU_LANGUAGES) {
            const char *lang = language_select_menu();
            if (lang) {
                char path[64];
                snprintf(path, sizeof(path), "lessons/%s", lang);
                lessons_run_menu(path, &g_settings);
            }
        } else if (choice == MENU_PRACTICE) {
            if (practice_menu() == 0) {
                practice_load_file(&g_settings);
            }
        } else if (choice == MENU_STATISTICS) {
            stats_draw_screen();
        } else if (choice == MENU_SETTINGS) {
            settings_draw_screen(&g_settings, &g_config);
        }
    } while (choice != MENU_EXIT);

    ui_cleanup();
    return 0;
}
