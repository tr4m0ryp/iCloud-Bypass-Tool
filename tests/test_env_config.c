/*
 * test_env_config.c -- Unit tests for util/env_config.
 *
 * These tests manipulate env state with setenv/unsetenv and assert
 * that env_get / env_get_int / env_require_all behave the way Phase
 * 2B/2C callers expect.  No external libraries are required, so the
 * suite links identically against make test and make test-mocks.
 */

#include "test_framework.h"
#include "util/env_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define E1 "TR4MPASS_TEST_VAR_1"
#define E2 "TR4MPASS_TEST_VAR_2"
#define E3 "TR4MPASS_TEST_VAR_3"

static void reset_test_vars(void)
{
    unsetenv(E1);
    unsetenv(E2);
    unsetenv(E3);
}

/* -------------------------------------------------------------- */
/* env_get                                                        */
/* -------------------------------------------------------------- */

static void test_env_get_set_var(void)
{
    reset_test_vars();
    setenv(E1, "hello-world", 1);
    const char *v = env_get(E1, "fallback");
    ASSERT_NOTNULL(v);
    ASSERT_STREQ(v, "hello-world");
}

static void test_env_get_unset_returns_default(void)
{
    reset_test_vars();
    const char *v = env_get(E1, "default-value");
    ASSERT_NOTNULL(v);
    ASSERT_STREQ(v, "default-value");

    /* NULL default must pass through. */
    ASSERT_NULL(env_get(E1, NULL));
}

static void test_env_get_empty_returns_empty(void)
{
    reset_test_vars();
    setenv(E1, "", 1);
    const char *v = env_get(E1, "fallback");
    ASSERT_NOTNULL(v);
    ASSERT_STREQ(v, "");
}

/* -------------------------------------------------------------- */
/* env_get_int                                                    */
/* -------------------------------------------------------------- */

static void test_env_get_int_valid(void)
{
    reset_test_vars();
    setenv(E1, "42", 1);
    ASSERT_EQ(env_get_int(E1, -1), 42);

    setenv(E1, "-7", 1);
    ASSERT_EQ(env_get_int(E1, 0), -7);
}

static void test_env_get_int_invalid(void)
{
    reset_test_vars();
    setenv(E1, "abc", 1);
    ASSERT_EQ(env_get_int(E1, 99), 99);

    setenv(E1, "12abc", 1);
    ASSERT_EQ(env_get_int(E1, 99), 99);

    setenv(E1, "", 1);
    ASSERT_EQ(env_get_int(E1, 99), 99);
}

static void test_env_get_int_unset(void)
{
    reset_test_vars();
    ASSERT_EQ(env_get_int(E1, 123), 123);
    ASSERT_EQ(env_get_int(E1, -456), -456);
}

/* -------------------------------------------------------------- */
/* env_require_all                                                */
/* -------------------------------------------------------------- */

static void test_env_require_all_present(void)
{
    reset_test_vars();
    setenv(E1, "v1", 1);
    setenv(E2, "v2", 1);
    setenv(E3, "v3", 1);
    const char *names[] = { E1, E2, E3 };
    ASSERT_EQ(env_require_all(names, 3), 0);
}

static void test_env_require_all_missing_one(void)
{
    reset_test_vars();
    setenv(E1, "v1", 1);
    setenv(E2, "", 1);      /* empty counts as missing */
    setenv(E3, "v3", 1);
    const char *names[] = { E1, E2, E3 };
    ASSERT_EQ(env_require_all(names, 3), -1);

    /* Fully absent also returns -1. */
    reset_test_vars();
    ASSERT_EQ(env_require_all(names, 3), -1);
}

void run_env_config_tests(void)
{
    printf("--- Section 9: env_config ---\n");
    test_env_get_set_var();
    test_env_get_unset_returns_default();
    test_env_get_empty_returns_empty();
    test_env_get_int_valid();
    test_env_get_int_invalid();
    test_env_get_int_unset();
    test_env_require_all_present();
    test_env_require_all_missing_one();
    reset_test_vars();
}
