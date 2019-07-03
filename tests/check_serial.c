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
#include <sys/ioctl.h>
#include <asm/termbits.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include "../src/serial.h"


int
__wrap_open(const char *pathname, int flags)
{
    assert_string_equal(pathname, mock_type(const char*));
    assert_int_equal(flags, O_RDWR);
    errno = 0;
    return mock_type(int);
}


int
__wrap_close(int fd)
{
    assert_int_equal(fd, mock_type(int));
    return 0;
}


int
__wrap_ioctl(int fd, unsigned long request, void *arg)
{
    assert_int_equal(fd, mock_type(int));
    assert_int_equal(request, mock_type(unsigned long));

    switch(mock_type(int)) {
        case 0:
            {
                uint32_t br = mock_type(uint32_t);
                struct termios2 *cfg = arg;
                assert_non_null(cfg);
                assert_int_equal(cfg->c_ispeed, br);
                assert_int_equal(cfg->c_ospeed, br);
            }
            break;
        case 1:
            assert_int_equal((unsigned long) arg, mock_type(unsigned long));
            break;
    }

    errno = 0;
    return mock_type(int);
}


int
__wrap_usleep(useconds_t usec)
{
    assert_int_equal(usec, mock_type(useconds_t));
    return 0;
}


static void
test_open1(void **state)
{
    will_return(__wrap_open, "asd");
    will_return(__wrap_open, -1);

    dg_error_t *err = NULL;
    int fd = dg_serial_open("asd", 123456, &err);
    assert_int_equal(fd, -1);
    assert_non_null(err);
    assert_string_equal(err->msg, "Failed to open serial port (asd [123456]): (unset)");
    dg_error_free(err);
}


static void
test_open2(void **state)
{
    will_return(__wrap_open, "asd");
    will_return(__wrap_open, 44);
    will_return(__wrap_ioctl, 44);
    will_return(__wrap_ioctl, TCSETS2);
    will_return(__wrap_ioctl, 123456);
    will_return(__wrap_ioctl, -1);
    will_return(__wrap_close, 44);

    dg_error_t *err = NULL;
    int fd = dg_serial_open("asd", 123456, &err);
    assert_int_equal(fd, -1);
    assert_non_null(err);
    assert_string_equal(err->msg, "Failed to set termios2 properties (asd [123456]): (unset)");
    dg_error_free(err);
}


static void
test_open3(void **state)
{
    will_return(__wrap_open, "asd");
    will_return(__wrap_open, 44);
    will_return(__wrap_ioctl, 44);
    will_return(__wrap_ioctl, TCSETS2);
    will_return(__wrap_ioctl, 0);
    will_return(__wrap_ioctl, 123456);
    will_return(__wrap_ioctl, 0);
    will_return(__wrap_usleep, 30000);

    will_return(__wrap_ioctl, 44);
    will_return(__wrap_ioctl, TCFLSH);
    will_return(__wrap_ioctl, 1);
    will_return(__wrap_ioctl, TCIOFLUSH);
    will_return(__wrap_ioctl, -1);
    will_return(__wrap_close, 44);

    dg_error_t *err = NULL;
    int fd = dg_serial_open("asd", 123456, &err);
    assert_int_equal(fd, -1);
    assert_non_null(err);
    assert_string_equal(err->msg, "Failed to flush serial port for startup (asd "
        "[123456]): Failed to flush serial port: (unset)");
    dg_error_free(err);
}


static void
test_open4(void **state)
{
    will_return(__wrap_open, "asd");
    will_return(__wrap_open, 44);
    will_return(__wrap_ioctl, 44);
    will_return(__wrap_ioctl, TCSETS2);
    will_return(__wrap_ioctl, 0);
    will_return(__wrap_ioctl, 123456);
    will_return(__wrap_ioctl, 0);
    will_return(__wrap_usleep, 30000);

    will_return(__wrap_ioctl, 44);
    will_return(__wrap_ioctl, TCFLSH);
    will_return(__wrap_ioctl, 1);
    will_return(__wrap_ioctl, TCIOFLUSH);
    will_return(__wrap_ioctl, 0);

    dg_error_t *err = NULL;
    int fd = dg_serial_open("asd", 123456, &err);
    assert_int_equal(fd, 44);
    assert_null(err);
}


int
main(void)
{
    const UnitTest tests[] = {
        unit_test(test_open1),
        unit_test(test_open2),
        unit_test(test_open3),
        unit_test(test_open4),
    };
    return run_tests(tests);
}
