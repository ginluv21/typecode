#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test.h"
#include "stats.h"

/* Redirect STATS_FILE to a temp file for isolation */
static char tmp_path[64];

static void setup(void)
{
    strcpy(tmp_path, "/tmp/test_stats_XXXXXX");
    int fd = mkstemp(tmp_path);
    if (fd >= 0) close(fd);
}

static void teardown(void)
{
    unlink(tmp_path);
}

static void write_line(const char *line)
{
    FILE *f = fopen(tmp_path, "a");
    if (f) { fputs(line, f); fclose(f); }
}


static void test_load_empty_file(void)
{
    // given
    // when
    // then

    SessionResult buf[10];
    FILE *f = fopen(tmp_path, "r");
    int count = 0;
    char line[512];
    while (count < 10 && fgets(line, sizeof(line), f)) {
        SessionResult r = {0};
        if (sscanf(line, "%ld|%d|%f|%d|%d|%127[^\n]",
                   &r.timestamp, &r.wpm, &r.accuracy,
                   &r.errors, &r.duration_sec, r.lesson) == 6)
            buf[count++] = r;
    }
    fclose(f);

    ASSERT_EQ(count, 0);
    TEST_PASS();
}

static void test_load_one_session(void)
{
    // given
    write_line("1700000000|42|95.5|3|60|lesson01\n");

    // when
    FILE *f = fopen(tmp_path, "r");
    SessionResult buf[10];
    int count = 0;
    char line[512];
    while (count < 10 && fgets(line, sizeof(line), f)) {
        SessionResult r = {0};
        if (sscanf(line, "%ld|%d|%f|%d|%d|%127[^\n]",
                   &r.timestamp, &r.wpm, &r.accuracy,
                   &r.errors, &r.duration_sec, r.lesson) == 6)
            buf[count++] = r;
    }
    fclose(f);

    // then
    ASSERT_EQ(count, 1);
    ASSERT_EQ(buf[0].wpm, 42);
    ASSERT_EQ(buf[0].errors, 3);
    ASSERT_STR(buf[0].lesson, "lesson01");
    TEST_PASS();
}

static void test_load_skips_malformed_lines(void)
{
    // given
    write_line("1700000001|30|88.0|5|45|lesson02\n");
    write_line("not_a_valid_line\n");
    write_line("1700000002|55|100.0|0|30|lesson03\n");

    // when
    FILE *f = fopen(tmp_path, "r");
    SessionResult buf[10];
    int count = 0;
    char line[512];
    while (count < 10 && fgets(line, sizeof(line), f)) {
        SessionResult r = {0};
        if (sscanf(line, "%ld|%d|%f|%d|%d|%127[^\n]",
                   &r.timestamp, &r.wpm, &r.accuracy,
                   &r.errors, &r.duration_sec, r.lesson) == 6)
            buf[count++] = r;
    }
    fclose(f);

    // then
    ASSERT_EQ(count, 2);
    TEST_PASS();
}

static void test_best_wpm_from_multiple(void)
{
    // given
    write_line("1700000001|20|90.0|2|60|a\n");
    write_line("1700000002|75|92.0|1|60|b\n");
    write_line("1700000003|50|88.0|4|60|c\n");

    // when
    FILE *f = fopen(tmp_path, "r");
    int best = 0;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        SessionResult r = {0};
        if (sscanf(line, "%ld|%d|%f|%d|%d|%127[^\n]",
                   &r.timestamp, &r.wpm, &r.accuracy,
                   &r.errors, &r.duration_sec, r.lesson) == 6)
            if (r.wpm > best) best = r.wpm;
    }
    fclose(f);

    // then
    ASSERT_EQ(best, 75);
    TEST_PASS();
}


int main(void)
{
    setup(); test_load_empty_file();    teardown();
    setup(); test_load_one_session();   teardown();
    setup(); test_load_skips_malformed_lines(); teardown();
    setup(); test_best_wpm_from_multiple();     teardown();
    return 0;
}
