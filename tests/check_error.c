/*
 * dwire-gdb: A GDB server for AVR 8 bit microcontrollers, using debugWire
 *            protocol through USB-to-TTL adapters.
 * Copyright (C) 2019 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 *
 * This program can be distributed under the terms of the BSD License.
 * See the file LICENSE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>
#include "../src/error.h"


char*
__wrap_strerror(int errnum)
{
    int e = mock_type(int);
    assert_int_equal(errnum, e);
    return "foo";
}


static void
test_error_new(void **state)
{
    dg_error_t *error = dg_error_new(1, "bola %s");
    assert_non_null(error);
    assert_int_equal(error->type, 1);
    assert_string_equal(error->msg, "bola %s");
    dg_error_free(error);
}


static void
test_error_new_printf(void **state)
{
    dg_error_t *error = dg_error_new_printf(2, "bola %s", "guda");
    assert_non_null(error);
    assert_int_equal(error->type, 2);
    assert_string_equal(error->msg, "bola guda");
    dg_error_free(error);
}


static void
test_error_new_errno(void **state)
{
    will_return(__wrap_strerror, 4);
    dg_error_t *error = dg_error_new_errno(1, 4, "bola %s");
    assert_non_null(error);
    assert_int_equal(error->type, 1);
    assert_string_equal(error->msg, "bola %s: foo");
    dg_error_free(error);
}


static void
test_error_new_errno_printf(void **state)
{
    will_return(__wrap_strerror, 3);
    dg_error_t *error = dg_error_new_errno_printf(2, 3, "bola %s", "guda");
    assert_non_null(error);
    assert_int_equal(error->type, 2);
    assert_string_equal(error->msg, "bola guda: foo");
    dg_error_free(error);
}


static void
test_error_new_errno_unset(void **state)
{
    dg_error_t *error = dg_error_new_errno(1, 0, "bola %s");
    assert_non_null(error);
    assert_int_equal(error->type, 1);
    assert_string_equal(error->msg, "bola %s: (unset)");
    dg_error_free(error);
}


static void
test_error_new_errno_printf_unset(void **state)
{
    dg_error_t *error = dg_error_new_errno_printf(2, 0, "bola %s", "guda");
    assert_non_null(error);
    assert_int_equal(error->type, 2);
    assert_string_equal(error->msg, "bola guda: (unset)");
    dg_error_free(error);
}


int
main(void)
{
    const UnitTest tests[] = {
        unit_test(test_error_new),
        unit_test(test_error_new_printf),
        unit_test(test_error_new_errno),
        unit_test(test_error_new_errno_printf),
        unit_test(test_error_new_errno_unset),
        unit_test(test_error_new_errno_printf_unset),
    };
    return run_tests(tests);
}
