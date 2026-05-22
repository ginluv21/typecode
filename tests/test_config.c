#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "test.h"
#include "config.h"

#define TMP_HOME "/tmp/test_config_home"

static void setup(void)
{
    setenv("HOME", TMP_HOME, 1);
    mkdir(TMP_HOME, 0755);
    mkdir(TMP_HOME "/.typecode", 0755);
}

static void teardown(void)
{
    unlink(TMP_HOME "/.typecode/config.conf");
    rmdir(TMP_HOME "/.typecode");
    rmdir(TMP_HOME);
}

static void test_defaults_vim_mode_off(void)
{
    AppConfig c;
    c.vim_mode = 99;
    config_defaults(&c);
    ASSERT_EQ(c.vim_mode, 0);
    TEST_PASS();
}

static void test_load_missing_gives_defaults(void)
{
    setup();
    unlink(TMP_HOME "/.typecode/config.conf");
    AppConfig c;
    config_load(&c);
    ASSERT_EQ(c.vim_mode, 0);
    teardown();
    TEST_PASS();
}

static void test_save_load_roundtrip_vim_on(void)
{
    setup();
    AppConfig c;
    c.vim_mode = 1;
    config_save(&c);
    AppConfig c2;
    config_load(&c2);
    ASSERT_EQ(c2.vim_mode, 1);
    teardown();
    TEST_PASS();
}

static void test_save_load_roundtrip_vim_off(void)
{
    setup();
    AppConfig c;
    c.vim_mode = 0;
    config_save(&c);
    AppConfig c2;
    c2.vim_mode = 1;
    config_load(&c2);
    ASSERT_EQ(c2.vim_mode, 0);
    teardown();
    TEST_PASS();
}

int main(void)
{
    test_defaults_vim_mode_off();
    test_load_missing_gives_defaults();
    test_save_load_roundtrip_vim_on();
    test_save_load_roundtrip_vim_off();
    return 0;
}
