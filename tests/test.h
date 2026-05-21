#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GIVEN(desc)
#define WHEN(desc)
#define THEN(desc)

#define ASSERT_EQ(a, b) \
    do { if ((a) != (b)) { \
        fprintf(stderr, "FAIL %s:%d: expected %d, got %d\n", \
                __FILE__, __LINE__, (int)(b), (int)(a)); \
        exit(1); \
    } } while (0)

#define ASSERT_TRUE(x)  ASSERT_EQ(!!(x), 1)
#define ASSERT_FALSE(x) ASSERT_EQ(!!(x), 0)

#define ASSERT_STR(a, b) \
    do { if (strcmp((a), (b)) != 0) { \
        fprintf(stderr, "FAIL %s:%d: \"%s\" != \"%s\"\n", \
                __FILE__, __LINE__, (a), (b)); \
        exit(1); \
    } } while (0)

#define TEST_PASS() fprintf(stdout, "PASS: %s\n", __func__)

#endif /* TEST_H */
