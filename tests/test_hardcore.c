#include <stdio.h>
#include <stdlib.h>
#include "test.h"
#include "settings.h"

static int backspace_allowed(const Settings *s)
{
    return !s->hardcore;
}

static void test_default_allows_backspace(void)
{
    // given
    Settings s;
    settings_defaults(&s);

    // when
    int allowed = backspace_allowed(&s);

    // then
    ASSERT_EQ(allowed, 1);
    TEST_PASS();
}

static void test_hardcore_blocks_backspace(void)
{
    // given
    Settings s;
    settings_defaults(&s);
    s.hardcore = 1;

    // when
    int allowed = backspace_allowed(&s);

    // then
    ASSERT_EQ(allowed, 0);
    TEST_PASS();
}

static void test_hardcore_default_off(void)
{
    // given
    Settings s;

    // when
    settings_defaults(&s);

    // then
    ASSERT_EQ(s.hardcore, 0);
    TEST_PASS();
}

static void test_hardcore_toggle(void)
{
    // given
    Settings s;
    settings_defaults(&s);

    // when
    s.hardcore = 1;

    // then
    ASSERT_EQ(backspace_allowed(&s), 0);

    // when
    s.hardcore = 0;

    // then
    ASSERT_EQ(backspace_allowed(&s), 1);
    TEST_PASS();
}

int main(void)
{
    test_default_allows_backspace();
    test_hardcore_blocks_backspace();
    test_hardcore_default_off();
    test_hardcore_toggle();
    return 0;
}
