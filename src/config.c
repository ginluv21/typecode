#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

#define CONFIG_FILE "/.typecode/config.conf"

static void build_path(char *buf, int size, const char *suffix)
{
    const char *home = getenv("HOME");
    snprintf(buf, size, "%s%s", home ? home : "", suffix);
}

void config_defaults(AppConfig *c)
{
    c->vim_mode = 0;
}

void config_load(AppConfig *c)
{
    config_defaults(c);
    char path[512];
    build_path(path, sizeof(path), CONFIG_FILE);
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[64];
    while (fgets(line, sizeof(line), f)) {
        int v;
        if (sscanf(line, "vim_mode=%d", &v) == 1) c->vim_mode = v;
    }
    fclose(f);
}

void config_save(const AppConfig *c)
{
    char path[512];
    build_path(path, sizeof(path), CONFIG_FILE);
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "vim_mode=%d\n", c->vim_mode);
    fclose(f);
}
