#include <signal.h>
#include "typecode.h"
#include "ui.h"
#include "menu.h"
#include "lesson.h"

static volatile sig_atomic_t g_resized = 0;

static void handle_sigwinch(int sig) // ловит SIGWINCH (resize терминала), ставит флаг g_resized
{
    (void)sig;
    g_resized = 1;
}

int main(void) // точка входа: инит ncurses, главный цикл меню до MENU_EXIT
{
    signal(SIGWINCH, handle_sigwinch);
    ui_init();

    MenuOption choice;
    do {
        choice = menu_run();
        if (choice == MENU_LESSONS)
            lesson_select_menu("lessons/latin");
    } while (choice != MENU_EXIT);

    ui_cleanup();
    return 0;
}
