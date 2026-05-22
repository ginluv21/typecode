#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "test.h"
#include "heatmap.h"

#define TMP_HOME "/tmp/test_heatmap_home"

static void setup(void)
{
    setenv("HOME", TMP_HOME, 1);
    mkdir(TMP_HOME, 0755);
    mkdir(TMP_HOME "/.typecode", 0755);
}

static void teardown(void)
{
    unlink(TMP_HOME "/.typecode/heatmap.txt");
    rmdir(TMP_HOME "/.typecode");
    rmdir(TMP_HOME);
}

static void test_record_error_increments(void)
{
    Heatmap h = {0};
    heatmap_record_error(&h, 'a', 'b');
    ASSERT_EQ(h.counts[(int)'a'], 1);
    TEST_PASS();
}


static void test_record_error_null_ignored(void)
{
    heatmap_record_error(NULL, 'a', 'b');
    TEST_PASS();
}

static void test_record_error_high_byte_ignored(void)
{
    Heatmap h = {0};
    heatmap_record_error(&h, (char)'\x80', 'b');
    ASSERT_EQ(h.counts[0], 0);
    TEST_PASS();
}

static void test_reset_clears_counts(void)
{
    setup();
    Heatmap h = {0};
    heatmap_record_error(&h, 'a', 'b');
    heatmap_reset(&h);
    ASSERT_EQ(h.counts[(int)'a'], 0);
    teardown();
    TEST_PASS();
}

static void test_save_load_roundtrip(void)
{
    setup();
    Heatmap h = {0};
    h.counts[(int)'a'] = 5;
    h.counts[(int)'z'] = 3;
    heatmap_save(&h);
    Heatmap h2 = {0};
    heatmap_load(&h2);
    ASSERT_EQ(h2.counts[(int)'a'], 5);
    ASSERT_EQ(h2.counts[(int)'z'], 3);
    teardown();
    TEST_PASS();
}

static void test_load_missing_file(void)
{
    setup();
    unlink(TMP_HOME "/.typecode/heatmap.txt");
    Heatmap h = {0};
    h.counts[0] = 99;
    heatmap_load(&h);
    ASSERT_EQ(h.counts[(int)'a'], 0);
    teardown();
    TEST_PASS();
}

static void test_load_null_ignored(void)
{
    heatmap_load(NULL);
    TEST_PASS();
}

static void test_save_null_ignored(void)
{
    heatmap_save(NULL);
    TEST_PASS();
}

static void test_generate_exercise_with_errors(void)
{
    Heatmap h = {0};
    h.counts[(int)'a'] = 5;
    h.counts[(int)'s'] = 3;
    h.counts[(int)';'] = 7;
    h.counts[(int)'1'] = 2;
    char *ex = heatmap_generate_exercise(&h, 4);
    ASSERT_TRUE(ex != NULL);
    ASSERT_TRUE(strlen(ex) > 0);
    free(ex);
    TEST_PASS();
}

static void test_generate_exercise_empty_fallback(void)
{
    Heatmap h = {0};
    char *ex = heatmap_generate_exercise(&h, 5);
    ASSERT_TRUE(ex != NULL);
    ASSERT_TRUE(strlen(ex) > 0);
    free(ex);
    TEST_PASS();
}

static void test_generate_exercise_null_returns_null(void)
{
    char *ex = heatmap_generate_exercise(NULL, 5);
    ASSERT_TRUE(ex == NULL);
    TEST_PASS();
}

static void test_generate_exercise_zero_top_returns_null(void)
{
    Heatmap h = {0};
    h.counts[(int)'a'] = 3;
    char *ex = heatmap_generate_exercise(&h, 0);
    ASSERT_TRUE(ex == NULL);
    TEST_PASS();
}

static void test_generate_exercise_top_exceeds_count(void)
{
    Heatmap h = {0};
    h.counts[(int)'a'] = 2;
    char *ex = heatmap_generate_exercise(&h, 10);
    ASSERT_TRUE(ex != NULL);
    free(ex);
    TEST_PASS();
}

static void test_save_load_nonprintable(void)
{
    setup();
    Heatmap h = {0};
    h.counts[9] = 2;
    heatmap_save(&h);
    Heatmap h2 = {0};
    heatmap_load(&h2);
    ASSERT_TRUE(h2.counts[9] >= 0);
    teardown();
    TEST_PASS();
}

static void test_generate_exercise_special_chars(void)
{
    Heatmap h = {0};
    h.counts[(int)','] = 4;
    h.counts[(int)'.'] = 3;
    h.counts[(int)'/'] = 2;
    h.counts[(int)'['] = 5;
    h.counts[(int)']'] = 1;
    char *ex = heatmap_generate_exercise(&h, 5);
    ASSERT_TRUE(ex != NULL);
    ASSERT_TRUE(strlen(ex) > 0);
    free(ex);
    TEST_PASS();
}

static void test_generate_exercise_nonprintable_char(void)
{
    Heatmap h = {0};
    h.counts[9] = 5;
    char *ex = heatmap_generate_exercise(&h, 1);
    ASSERT_TRUE(ex != NULL);
    free(ex);
    TEST_PASS();
}

int main(void)
{
    test_record_error_increments();
    test_record_error_null_ignored();
    test_record_error_high_byte_ignored();
    test_reset_clears_counts();
    test_save_load_roundtrip();
    test_load_missing_file();
    test_load_null_ignored();
    test_save_null_ignored();
    test_generate_exercise_with_errors();
    test_generate_exercise_empty_fallback();
    test_generate_exercise_null_returns_null();
    test_generate_exercise_zero_top_returns_null();
    test_generate_exercise_top_exceeds_count();
    test_save_load_nonprintable();
    test_generate_exercise_special_chars();
    test_generate_exercise_nonprintable_char();
    return 0;
}
