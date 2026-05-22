#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "test.h"
#include "lesson.h"

static void test_elapsed_sec_returns_nonnegative(void)
{
    Metrics m = {0};
    m.started = 1;
    clock_gettime(CLOCK_MONOTONIC, &m.start);

    double e = elapsed_sec(&m);

    ASSERT_TRUE(e >= 0.0);
    TEST_PASS();
}

static void test_metrics_update_skips_when_not_started(void)
{
    Metrics m = {0};
    m.started = 0;
    m.wpm = 0;
    m.accuracy = 0.0f;

    metrics_update(&m);

    ASSERT_EQ(m.wpm, 0);
    ASSERT_EQ((int)m.accuracy, 0);
    TEST_PASS();
}

static void test_metrics_update_sets_wpm_and_accuracy(void)
{
    Metrics m = {0};
    m.started = 1;
    m.correct = 50;
    m.errors  = 5;
    clock_gettime(CLOCK_MONOTONIC, &m.start);

    metrics_update(&m);

    ASSERT_TRUE(m.wpm >= 0);
    ASSERT_TRUE(m.accuracy > 0.0f && m.accuracy <= 100.0f);
    TEST_PASS();
}

static void test_metrics_update_accuracy_with_no_input(void)
{
    Metrics m = {0};
    m.started = 1;
    m.correct = 0;
    m.errors  = 0;
    clock_gettime(CLOCK_MONOTONIC, &m.start);

    metrics_update(&m);

    ASSERT_EQ((int)m.accuracy, 100);
    TEST_PASS();
}

int main(void)
{
    test_elapsed_sec_returns_nonnegative();
    test_metrics_update_skips_when_not_started();
    test_metrics_update_sets_wpm_and_accuracy();
    test_metrics_update_accuracy_with_no_input();
    return 0;
}
