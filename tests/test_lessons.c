#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "test.h"
#include "lesson.h"

#define TMP_DIR "/tmp/test_lessons_scan_dir"

static void create_file(const char *path)
{
    FILE *f = fopen(path, "w");
    if (f) { fputs("hello world\n", f); fclose(f); }
}

static void setup(void)
{
    mkdir(TMP_DIR, 0755);
    create_file(TMP_DIR "/01_basics.txt");
    create_file(TMP_DIR "/02_loops.txt");
    create_file(TMP_DIR "/03_functions.txt");
}

static void teardown(void)
{
    unlink(TMP_DIR "/01_basics.txt");
    unlink(TMP_DIR "/02_loops.txt");
    unlink(TMP_DIR "/03_functions.txt");
    rmdir(TMP_DIR);
}

static void test_scan_counts_txt_files(void)
{
    setup();

    LessonList *list = lessons_scan(TMP_DIR);

    ASSERT_TRUE(list != NULL);
    ASSERT_EQ(list->count, 3);
    lessons_free(list);
    teardown();
    TEST_PASS();
}

static void test_scan_returns_null_on_missing_dir(void)
{
    LessonList *list = lessons_scan("/tmp/nonexistent_dir_xyzxyz");

    ASSERT_TRUE(list == NULL);
    TEST_PASS();
}

static void test_scan_returns_null_on_empty_dir(void)
{
    mkdir(TMP_DIR, 0755);

    LessonList *list = lessons_scan(TMP_DIR);

    ASSERT_TRUE(list == NULL);
    rmdir(TMP_DIR);
    TEST_PASS();
}

static void test_scan_strips_extension_from_name(void)
{
    setup();

    LessonList *list = lessons_scan(TMP_DIR);

    ASSERT_TRUE(list != NULL);
    int found = 0;
    for (int i = 0; i < list->count; i++) {
        if (strstr(list->entries[i].name, ".txt")) found = 1;
    }
    ASSERT_EQ(found, 0);
    lessons_free(list);
    teardown();
    TEST_PASS();
}

static void test_scan_replaces_underscores_in_name(void)
{
    setup();

    LessonList *list = lessons_scan(TMP_DIR);

    ASSERT_TRUE(list != NULL);
    int found = 0;
    for (int i = 0; i < list->count; i++) {
        if (strcmp(list->entries[i].name, "01 basics") == 0) found = 1;
    }
    ASSERT_EQ(found, 1);
    lessons_free(list);
    teardown();
    TEST_PASS();
}

static void test_scan_sorts_entries_by_path(void)
{
    setup();

    LessonList *list = lessons_scan(TMP_DIR);

    ASSERT_TRUE(list != NULL);
    ASSERT_EQ(list->count, 3);
    for (int i = 0; i < list->count - 1; i++) {
        ASSERT_TRUE(strcmp(list->entries[i].path, list->entries[i+1].path) < 0);
    }
    lessons_free(list);
    teardown();
    TEST_PASS();
}

int main(void)
{
    test_scan_counts_txt_files();
    test_scan_returns_null_on_missing_dir();
    test_scan_returns_null_on_empty_dir();
    test_scan_strips_extension_from_name();
    test_scan_replaces_underscores_in_name();
    test_scan_sorts_entries_by_path();
    return 0;
}
