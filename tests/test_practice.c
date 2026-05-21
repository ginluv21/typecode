#include <stdio.h>
#include <stdlib.h>
#include "test.h"
#include "settings.h"

static void test_default_mode_normal(void)
{
    // given
    Settings s;

    // when
    settings_defaults(&s);

    // then
    ASSERT_EQ(s.mode, MODE_NORMAL);
    TEST_PASS();
}

static void test_timed_mode_set(void)
{
    // given
    Settings s;
    settings_defaults(&s);

    // when
    s.mode = MODE_TIMED;

    // then
    ASSERT_EQ(s.mode, MODE_TIMED);
    TEST_PASS();
}

static void test_infinite_mode_set(void)
{
    // given
    Settings s;
    settings_defaults(&s);

    // when
    s.mode = MODE_INFINITE;

    // then
    ASSERT_EQ(s.mode, MODE_INFINITE);
    TEST_PASS();
}

static void test_default_time_limit(void)
{
    // given
    Settings s;

    // when
    settings_defaults(&s);

    // then
    ASSERT_EQ(s.time_limit_sec, 60);
    TEST_PASS();
}

static void test_timed_break_condition(void)
{
    // given
    int time_limit = 30;
    int elapsed    = 31;

    // when
    int should_break = (elapsed >= time_limit);

    // then
    ASSERT_EQ(should_break, 1);
    TEST_PASS();
}

static void test_timed_no_break_before_limit(void)
{
    // given
    int time_limit = 30;
    int elapsed    = 15;

    // when
    int should_break = (elapsed >= time_limit);

    // then
    ASSERT_EQ(should_break, 0);
    TEST_PASS();
}

static void test_infinite_reset_cursor(void)
{
    // given
    int cursor_pos = 10;
    int text_len   = 10;
    int mode       = MODE_INFINITE;

    // when
    if (cursor_pos >= text_len && mode == MODE_INFINITE)
        cursor_pos = 0;

    // then
    ASSERT_EQ(cursor_pos, 0);
    TEST_PASS();
}

int main(void)
{
    test_default_mode_normal();
    test_timed_mode_set();
    test_infinite_mode_set();
    test_default_time_limit();
    test_timed_break_condition();
    test_timed_no_break_before_limit();
    test_infinite_reset_cursor();
    return 0;
}
