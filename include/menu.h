#ifndef MENU_H
#define MENU_H

#include "typecode.h"
#include "config.h"

void       draw_main_menu(int selected);
MenuOption menu_run(const AppConfig *cfg);

#endif
