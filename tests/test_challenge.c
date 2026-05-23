#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "test.h"
#include "challenge.h"

#define TMP_HOME      "/tmp/test_challenge_home"
#define CHALLENGE_REL "/.typecode/challenge.txt"

static void setup(void)
{
    setenv("HOME", TMP_HOME, 1);
    mkdir(TMP_HOME, 0755);
    mkdir(TMP_HOME "/.typecode", 0755);
    unlink(TMP_HOME CHALLENGE_REL);
}

static void teardown(void)
{
    unlink(TMP_HOME CHALLENGE_REL);
    rmdir(TMP_HOME "/.typecode");
    rmdir(TMP_HOME);
}

static void write_record(int wpm, long ts)
{
    FILE *f = fopen(TMP_HOME CHALLENGE_REL, "a");
    if (f) { fprintf(f, "%d %ld\n", wpm, ts); fclose(f); }
}

static void test_load_missing_file(void)
{
    setup();
    unlink(TMP_HOME CHALLENGE_REL);

    ChallengeRecord buf[5];
    int n = challenge_load_records(buf, 5);

    ASSERT_EQ(n, 0);
    teardown();
    TEST_PASS();
}

static void test_load_empty_file(void)
{
    setup();
    FILE *f = fopen(TMP_HOME CHALLENGE_REL, "w");
    if (f) fclose(f);

    ChallengeRecord buf[5];
    int n = challenge_load_records(buf, 5);

    ASSERT_EQ(n, 0);
    teardown();
    TEST_PASS();
}

static void test_load_one_record(void)
{
    setup();
    write_record(42, 1700000000L);

    ChallengeRecord buf[5];
    int n = challenge_load_records(buf, 5);

    ASSERT_EQ(n, 1);
    ASSERT_EQ(buf[0].wpm, 42);
    ASSERT_EQ((long)buf[0].timestamp, 1700000000L);
    teardown();
    TEST_PASS();
}

static void test_load_sorted_descending(void)
{
    setup();
    write_record(30, 1700000001L);
    write_record(70, 1700000002L);
    write_record(50, 1700000003L);

    ChallengeRecord buf[5];
    int n = challenge_load_records(buf, 5);

    ASSERT_EQ(n, 3);
    ASSERT_EQ(buf[0].wpm, 70);
    ASSERT_EQ(buf[1].wpm, 50);
    ASSERT_EQ(buf[2].wpm, 30);
    teardown();
    TEST_PASS();
}

static void test_load_respects_max(void)
{
    setup();
    write_record(10, 1L);
    write_record(20, 2L);
    write_record(30, 3L);
    write_record(40, 4L);
    write_record(50, 5L);

    ChallengeRecord buf[3];
    int n = challenge_load_records(buf, 3);

    ASSERT_EQ(n, 3);
    teardown();
    TEST_PASS();
}

static void test_load_skips_malformed(void)
{
    setup();
    FILE *f = fopen(TMP_HOME CHALLENGE_REL, "w");
    if (f) {
        fprintf(f, "55 1700000001\n");
        fprintf(f, "not_valid_line\n");
        fprintf(f, "40 1700000002\n");
        fclose(f);
    }

    ChallengeRecord buf[5];
    int n = challenge_load_records(buf, 5);

    ASSERT_EQ(n, 2);
    teardown();
    TEST_PASS();
}

static void test_load_max_zero(void)
{
    setup();
    write_record(55, 1700000001L);

    ChallengeRecord buf[1];
    int n = challenge_load_records(buf, 0);

    ASSERT_EQ(n, 0);
    teardown();
    TEST_PASS();
}

int main(void)
{
    test_load_missing_file();
    test_load_empty_file();
    test_load_one_record();
    test_load_sorted_descending();
    test_load_respects_max();
    test_load_skips_malformed();
    test_load_max_zero();
    return 0;
}
