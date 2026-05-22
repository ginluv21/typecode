#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    int vim_mode; /* 0 = off, 1 = on */
} AppConfig;

void config_defaults(AppConfig *c);
void config_load(AppConfig *c);
void config_save(const AppConfig *c);

#endif
