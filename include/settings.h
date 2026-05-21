#ifndef SETTINGS_H
#define SETTINGS_H

typedef enum { MODE_NORMAL = 0, MODE_TIMED, MODE_INFINITE } PracticeMode;

typedef struct {
    int          hardcore;
    PracticeMode mode;
    int          time_limit_sec;
} Settings;

void settings_defaults(Settings *s);
void settings_load(Settings *s);
void settings_save(const Settings *s);
void settings_draw_screen(Settings *s);

#endif
