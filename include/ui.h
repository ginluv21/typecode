#ifndef UI_H
#define UI_H

void ui_init(void);
void ui_cleanup(void);
void ui_on_resize(void);
int  ui_too_small(void);
int  tui_readline(int row, int col, int maxlen, char *out);

#endif
