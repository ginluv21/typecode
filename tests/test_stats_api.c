#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "test.h"
#include "stats.h"

#define TMP_HOME "/tmp/test_stats_api_home"

static void setup(void)
{
    setenv("HOME", TMP_HOME, 1);
    mkdir(TMP_HOME, 0755);
    stats_ensure_dir();
    unlink(TMP_HOME STATS_FILE);
}

static void teardown(void)
{
    unlink(TMP_HOME STATS_FILE);
    rmdir(TMP_HOME "/.typecode");
    rmdir(TMP_HOME);
}

static void test_save_load_roundtrip(void)
{
    // given
    setup();
    SessionResult r = {
        .timestamp    = 1700000000,
        .wpm          = 55,
        .accuracy     = 97.5f,
        .errors       = 2,
        .duration_sec = 60,
    };
    strncpy(r.lesson, "test_lesson", LESSON_MAX - 1);

    // when
    stats_save(&r);
    SessionResult buf[10];
    int n = stats_load(buf, 10);

    // then
    ASSERT_EQ(n, 1);
    ASSERT_EQ(buf[0].wpm, 55);
    ASSERT_EQ(buf[0].errors, 2);
    ASSERT_STR(buf[0].lesson, "test_lesson");
    teardown();
    TEST_PASS();
}

static void test_load_empty(void)
{
    // given
    setup();

    // when
    SessionResult buf[10];
    int n = stats_load(buf, 10);

    // then
    ASSERT_EQ(n, 0);
    teardown();
    TEST_PASS();
}

static void test_best_wpm(void)
{
    // given
    setup();
    SessionResult r1 = {.timestamp=1, .wpm=30, .accuracy=90.0f, .errors=5, .duration_sec=60};
    SessionResult r2 = {.timestamp=2, .wpm=80, .accuracy=95.0f, .errors=2, .duration_sec=60};
    SessionResult r3 = {.timestamp=3, .wpm=50, .accuracy=92.0f, .errors=3, .duration_sec=60};
    strncpy(r1.lesson, "a", LESSON_MAX - 1);
    strncpy(r2.lesson, "b", LESSON_MAX - 1);
    strncpy(r3.lesson, "c", LESSON_MAX - 1);

    // when
    stats_save(&r1);
    stats_save(&r2);
    stats_save(&r3);
    int best = stats_best_wpm();

    // then
    ASSERT_EQ(best, 80);
    teardown();
    TEST_PASS();
}

static void test_avg_wpm(void)
{
    setup();
    SessionResult r1 = {.timestamp=1, .wpm=40, .accuracy=90.0f, .errors=4, .duration_sec=60};
    SessionResult r2 = {.timestamp=2, .wpm=60, .accuracy=95.0f, .errors=2, .duration_sec=60};
    strncpy(r1.lesson, "x", LESSON_MAX - 1);
    strncpy(r2.lesson, "y", LESSON_MAX - 1);

    stats_save(&r1);
    stats_save(&r2);
    float avg = stats_avg_wpm(10);

    ASSERT_EQ((int)avg, 50);
    teardown();
    TEST_PASS();
}

static void test_ensure_dir_creates_directory(void)
{
    setenv("HOME", TMP_HOME, 1);
    mkdir(TMP_HOME, 0755);
    rmdir(TMP_HOME "/.typecode");

    stats_ensure_dir();

    struct stat st;
    ASSERT_EQ(stat(TMP_HOME "/.typecode", &st), 0);
    ASSERT_TRUE(S_ISDIR(st.st_mode));
    rmdir(TMP_HOME "/.typecode");
    rmdir(TMP_HOME);
    TEST_PASS();
}

static void test_reset_clears_stats(void)
{
    setup();
    SessionResult r = {.timestamp=1, .wpm=50, .accuracy=95.0f, .errors=1, .duration_sec=60};
    strncpy(r.lesson, "x", LESSON_MAX - 1);
    stats_save(&r);

    stats_reset();

    SessionResult buf[10];
    int n = stats_load(buf, 10);
    ASSERT_EQ(n, 0);
    teardown();
    TEST_PASS();
}

int main(void)
{
    test_save_load_roundtrip();
    test_load_empty();
    test_best_wpm();
    test_avg_wpm();
    test_ensure_dir_creates_directory();
    test_reset_clears_stats();
    return 0;
}
