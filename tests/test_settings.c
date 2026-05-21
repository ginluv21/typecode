#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "test.h"
#include "settings.h"

#define TMP_HOME "/tmp/test_settings_home"

static void setup(void)
{
    setenv("HOME", TMP_HOME, 1);
    mkdir(TMP_HOME, 0755);
    mkdir(TMP_HOME "/.typecode", 0755);
}

static void teardown(void)
{
    unlink(TMP_HOME "/.typecode/settings.conf");
    rmdir(TMP_HOME "/.typecode");
    rmdir(TMP_HOME);
}

static void test_defaults(void)
{
    // given
    Settings s;

    // when
    settings_defaults(&s);

    // then
    ASSERT_EQ(s.hardcore, 0);
    ASSERT_EQ(s.mode, MODE_NORMAL);
    ASSERT_EQ(s.time_limit_sec, 60);
    TEST_PASS();
}

static void test_save_load_roundtrip(void)
{
    // given
    setup();
    Settings s = { .hardcore = 1, .mode = MODE_TIMED, .time_limit_sec = 120 };

    // when
    settings_save(&s);
    Settings loaded;
    settings_load(&loaded);

    // then
    ASSERT_EQ(loaded.hardcore, 1);
    ASSERT_EQ(loaded.mode, MODE_TIMED);
    ASSERT_EQ(loaded.time_limit_sec, 120);
    teardown();
    TEST_PASS();
}

static void test_load_missing_gives_defaults(void)
{
    // given
    setup();

    // when
    Settings s;
    settings_load(&s);

    // then
    ASSERT_EQ(s.hardcore, 0);
    ASSERT_EQ(s.mode, MODE_NORMAL);
    ASSERT_EQ(s.time_limit_sec, 60);
    teardown();
    TEST_PASS();
}

int main(void)
{
    test_defaults();
    test_save_load_roundtrip();
    test_load_missing_gives_defaults();
    return 0;
}
