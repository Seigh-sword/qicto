#ifndef QICTO_TEST_H
#define QICTO_TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_BEGIN(name) \
    static void name(void); \
    static void name(void)

#define ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("  FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
            g_tests_failed++; \
            return; \
        } \
        g_tests_passed++; \
    } while (0)

#define ASSERT_EQ(a, b, msg) \
    do { \
        if ((a) != (b)) { \
            printf("  FAIL: %s - expected %ld, got %ld (%s:%d)\n", msg, (long)(b), (long)(a), __FILE__, __LINE__); \
            g_tests_failed++; \
            return; \
        } \
        g_tests_passed++; \
    } while (0)

#define ASSERT_STR_EQ(a, b, msg) \
    do { \
        if (strcmp((a), (b)) != 0) { \
            printf("  FAIL: %s - expected '%s', got '%s' (%s:%d)\n", msg, b, a, __FILE__, __LINE__); \
            g_tests_failed++; \
            return; \
        } \
        g_tests_passed++; \
    } while (0)

#define RUN_TEST(name) \
    do { \
        printf("test: %s\n", #name); \
        name(); \
    } while (0)

#define TEST_SUMMARY() \
    do { \
        printf("\n--- Test Summary ---\n"); \
        printf("Passed: %d\n", g_tests_passed); \
        printf("Failed: %d\n", g_tests_failed); \
        printf("Total:  %d\n", g_tests_passed + g_tests_failed); \
        return g_tests_failed > 0 ? 1 : 0; \
    } while (0)

#endif
