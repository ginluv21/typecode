#include <signal.h>
#include "typecode.h"
#include "ui.h"
#include "menu.h"

static volatile sig_atomic_t g_resized = 0;

static void handle_sigwinch(int sig)
{
    (void)sig;
    g_resized = 1;
}

int main(void)
{
    signal(SIGWINCH, handle_sigwinch);
    ui_init();
    MenuOption choice = menu_run();
    (void)choice; /* будет использоваться при добавлении экранов */
    ui_cleanup();
    return 0;
}
