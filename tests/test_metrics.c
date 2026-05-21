#include <stdio.h>
#include "test.h"
#include "lesson.h"

static void test_wpm_one_minute(void)
{
    // given
    // when
    // then
    ASSERT_EQ(metrics_calc_wpm(50, 60.0), 10);
    TEST_PASS();
}

static void test_wpm_half_minute(void)
{
    // given
    // when
    // then
    ASSERT_EQ(metrics_calc_wpm(50, 30.0), 20);
    TEST_PASS();
}

static void test_wpm_zero_elapsed(void)
{
    // given
    // when
    // then
    ASSERT_EQ(metrics_calc_wpm(100, 0.0), 0);
    TEST_PASS();
}

static void test_accuracy_perfect(void)
{
    // given
    // when
    // then
    ASSERT_EQ((int)metrics_calc_accuracy(100, 0), 100);
    TEST_PASS();
}

static void test_accuracy_half(void)
{
    // given
    // when
    // then
    ASSERT_EQ((int)metrics_calc_accuracy(50, 50), 50);
    TEST_PASS();
}

static void test_accuracy_all_errors(void)
{
    // given
    // when
    // then
    ASSERT_EQ((int)metrics_calc_accuracy(0, 10), 0);
    TEST_PASS();
}

static void test_accuracy_no_input(void)
{
    // given
    // when
    // then
    ASSERT_EQ((int)metrics_calc_accuracy(0, 0), 100);
    TEST_PASS();
}

int main(void)
{
    test_wpm_one_minute();
    test_wpm_half_minute();
    test_wpm_zero_elapsed();
    test_accuracy_perfect();
    test_accuracy_half();
    test_accuracy_all_errors();
    test_accuracy_no_input();
    return 0;
}
