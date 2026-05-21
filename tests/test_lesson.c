#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test.h"
#include "lesson.h"

static char tmp_path[64];

static void write_tmp(const char *content)
{
    strcpy(tmp_path, "/tmp/test_lesson_XXXXXX");
    int fd = mkstemp(tmp_path);
    FILE *f = fdopen(fd, "w");
    fputs(content, f);
    fclose(f);
}

static void teardown(void) { unlink(tmp_path); }


static void test_load_valid_file(void)
{
    // given
    write_tmp("hello world");

    // when
    Lesson *l = lesson_load(tmp_path);

    // then
    ASSERT_TRUE(l != NULL);
    ASSERT_STR(l->text, "hello world");
    ASSERT_EQ(l->len, 11);
    lesson_free(l);
    teardown();
    TEST_PASS();
}

static void test_load_nonexistent_file(void)
{
    // given
    // when
    // then
    Lesson *l = lesson_load("/tmp/does_not_exist_typecode.txt");
    ASSERT_TRUE(l == NULL);
    TEST_PASS();
}

static void test_load_empty_file(void)
{
    // given
    write_tmp("");

    // when
    Lesson *l = lesson_load(tmp_path);

    // then
    ASSERT_TRUE(l == NULL);
    teardown();
    TEST_PASS();
}

static void test_load_name_from_filename(void)
{
    // given
    write_tmp("abc");

    // when
    Lesson *l = lesson_load(tmp_path);

    // then
    ASSERT_TRUE(l != NULL);
    ASSERT_TRUE(l->name[0] != '\0');
    lesson_free(l);
    teardown();
    TEST_PASS();
}


int main(void)
{
    test_load_valid_file();
    test_load_nonexistent_file();
    test_load_empty_file();
    test_load_name_from_filename();
    return 0;
}
