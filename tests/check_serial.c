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
#include <stdlib.h>
#include <sys/ioctl.h>
#include <asm/termbits.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include "../src/utils.h"
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
    errno = 0;
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
    errno = 0;
    return 0;
}


ssize_t
__wrap_read(int fd, void *buf, size_t count)
{
    assert_int_equal(fd, mock_type(int));
    assert_int_equal(count, mock_type(size_t));
    ssize_t l = mock_type(ssize_t);
    uint8_t *b = mock_type(uint8_t*);
    ssize_t i = 0;
    for (i = 0; i < count && i < l; i++) {
        ((uint8_t*)buf)[i] = b[i];
    }
    errno = 0;
    return l <= 0 ? l : i;
}


ssize_t
__wrap_write(int fd, const void *buf, size_t count)
{
    assert_int_equal(fd, mock_type(int));
    assert_int_equal(count, mock_type(size_t));
    uint8_t *b = dg_malloc(sizeof(uint8_t) * (count + 1));
    ssize_t l = mock_type(ssize_t);
    ssize_t i = 0;
    for (i = 0; i < count && i < l; i++) {
        b[i] = ((uint8_t*)buf)[i];
    }
    b[i] = 0;
    errno = 0;
    if (l > 0)
        assert_string_equal(b, mock_type(uint8_t*));
    free(b);
    return l <= 0 ? l : i;
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
    assert_int_equal(err->type, DG_ERROR_SERIAL);
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
    assert_int_equal(err->type, DG_ERROR_SERIAL);
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
    assert_int_equal(err->type, DG_ERROR_SERIAL);
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


static void
test_read1(void **state)
{
    will_return(__wrap_read, 44);
    will_return(__wrap_read, 21);
    will_return(__wrap_read, -1);
    will_return(__wrap_read, "abcdefghijklmnopqrstuvxyz");

    dg_error_t *err = NULL;
    uint8_t buf[21];
    int n = dg_serial_read(44, buf, 21, &err);
    assert_int_equal(n, -1);
    assert_non_null(err);
    assert_int_equal(err->type, DG_ERROR_SERIAL);
    assert_string_equal(err->msg, "Failed to read from serial port: (unset)");
    dg_error_free(err);
}


static void
test_read2(void **state)
{
    will_return(__wrap_read, 44);
    will_return(__wrap_read, 21);
    will_return(__wrap_read, 0);
    will_return(__wrap_read, "abcdefghijklmnopqrstuvxyz");

    dg_error_t *err = NULL;
    uint8_t buf[21];
    int n = dg_serial_read(44, buf, 21, &err);
    assert_int_equal(n, -1);
    assert_non_null(err);
    assert_int_equal(err->type, DG_ERROR_SERIAL);
    assert_string_equal(err->msg, "Got unexpected EOF from serial port");
    dg_error_free(err);
}


static void
test_read3(void **state)
{
    will_return(__wrap_read, 44);
    will_return(__wrap_read, 21);
    will_return(__wrap_read, 10);
    will_return(__wrap_read, "abcdefghijklmnopqrstuvxyz");
    will_return(__wrap_read, 44);
    will_return(__wrap_read, 11);
    will_return(__wrap_read, 10);
    will_return(__wrap_read, "abcdefghijklmnopqrstuvxyz");
    will_return(__wrap_read, 44);
    will_return(__wrap_read, 1);
    will_return(__wrap_read, 10);
    will_return(__wrap_read, "abcdefghijklmnopqrstuvxyz");

    dg_error_t *err = NULL;
    uint8_t buf[22];
    int n = dg_serial_read(44, buf, 21, &err);
    assert_int_equal(n, 21);
    assert_null(err);
    buf[21] = 0;
    assert_string_equal(buf, "abcdefghijabcdefghija");
}


static void
test_read_byte1(void **state)
{
    will_return(__wrap_read, 44);
    will_return(__wrap_read, 1);
    will_return(__wrap_read, -1);
    will_return(__wrap_read, "abcdefghijklmnopqrstuvxyz");

    dg_error_t *err = NULL;
    uint8_t c = dg_serial_read_byte(44, &err);
    assert_int_equal(c, 0);
    assert_non_null(err);
    assert_int_equal(err->type, DG_ERROR_SERIAL);
    assert_string_equal(err->msg, "Failed to read from serial port: (unset)");
    dg_error_free(err);
}


static void
test_read_byte2(void **state)
{
    will_return(__wrap_read, 44);
    will_return(__wrap_read, 1);
    will_return(__wrap_read, 0);
    will_return(__wrap_read, "abcdefghijklmnopqrstuvxyz");

    dg_error_t *err = NULL;
    uint8_t c = dg_serial_read_byte(44, &err);
    assert_int_equal(c, 0);
    assert_non_null(err);
    assert_int_equal(err->type, DG_ERROR_SERIAL);
    assert_string_equal(err->msg, "Got unexpected EOF from serial port");
    dg_error_free(err);
}


static void
test_read_byte3(void **state)
{
    will_return(__wrap_read, 44);
    will_return(__wrap_read, 1);
    will_return(__wrap_read, 1);
    will_return(__wrap_read, "abcdefghijklmnopqrstuvxyz");

    dg_error_t *err = NULL;
    uint8_t c = dg_serial_read_byte(44, &err);
    assert_int_equal(c, 'a');
    assert_null(err);
}


static void
test_read_word1(void **state)
{
    will_return(__wrap_read, 44);
    will_return(__wrap_read, 2);
    will_return(__wrap_read, -1);
    will_return(__wrap_read, "abcdefghijklmnopqrstuvxyz");

    dg_error_t *err = NULL;
    uint16_t c = dg_serial_read_word(44, &err);
    assert_int_equal(c, 0);
    assert_non_null(err);
    assert_int_equal(err->type, DG_ERROR_SERIAL);
    assert_string_equal(err->msg, "Failed to read from serial port: (unset)");
    dg_error_free(err);
}


static void
test_read_word2(void **state)
{
    will_return(__wrap_read, 44);
    will_return(__wrap_read, 2);
    will_return(__wrap_read, 0);
    will_return(__wrap_read, "abcdefghijklmnopqrstuvxyz");

    dg_error_t *err = NULL;
    uint16_t c = dg_serial_read_word(44, &err);
    assert_int_equal(c, 0);
    assert_non_null(err);
    assert_int_equal(err->type, DG_ERROR_SERIAL);
    assert_string_equal(err->msg, "Got unexpected EOF from serial port");
    dg_error_free(err);
}


static void
test_read_word3(void **state)
{
    will_return(__wrap_read, 44);
    will_return(__wrap_read, 2);
    will_return(__wrap_read, 2);
    will_return(__wrap_read, "abcdefghijklmnopqrstuvxyz");

    dg_error_t *err = NULL;
    uint16_t c = dg_serial_read_word(44, &err);
    assert_int_equal(c, ('a' << 8) | 'b');
    assert_null(err);
}


static void
test_write1(void **state)
{
    will_return(__wrap_write, 44);
    will_return(__wrap_write, 21);
    will_return(__wrap_write, -1);

    dg_error_t *err = NULL;
    uint8_t buf[21] = "abcdefghijklmnopqrstu";
    int n = dg_serial_write(44, buf, 21, &err);
    assert_int_equal(n, -1);
    assert_non_null(err);
    assert_int_equal(err->type, DG_ERROR_SERIAL);
    assert_string_equal(err->msg, "Failed to wrote to serial port: (unset)");
    dg_error_free(err);
}


static void
test_write2(void **state)
{
    will_return(__wrap_write, 44);
    will_return(__wrap_write, 21);
    will_return(__wrap_write, 0);

    dg_error_t *err = NULL;
    uint8_t buf[21] = "abcdefghijklmnopqrstu";
    int n = dg_serial_write(44, buf, 21, &err);
    assert_int_equal(n, -1);
    assert_non_null(err);
    assert_int_equal(err->type, DG_ERROR_SERIAL);
    assert_string_equal(err->msg, "Got unexpected EOF from serial port");
    dg_error_free(err);
}


static void
test_write3(void **state)
{
    will_return(__wrap_write, 44);
    will_return(__wrap_write, 21);
    will_return(__wrap_write, 10);
    will_return(__wrap_write, "abcdefghij");
    will_return(__wrap_write, 44);
    will_return(__wrap_write, 11);
    will_return(__wrap_write, 10);
    will_return(__wrap_write, "klmnopqrst");
    will_return(__wrap_write, 44);
    will_return(__wrap_write, 1);
    will_return(__wrap_write, 10);
    will_return(__wrap_write, "u");
    will_return(__wrap_read, 44);
    will_return(__wrap_read, 21);
    will_return(__wrap_read, 30);
    will_return(__wrap_read, "abcdefghijklmnopqrstuvxyz");

    dg_error_t *err = NULL;
    uint8_t buf[21] = "abcdefghijklmnopqrstu";
    int n = dg_serial_write(44, buf, 21, &err);
    assert_int_equal(n, 21);
    assert_null(err);
}


static void
test_write_byte1(void **state)
{
    will_return(__wrap_write, 44);
    will_return(__wrap_write, 1);
    will_return(__wrap_write, -1);

    dg_error_t *err = NULL;
    int n = dg_serial_write_byte(44, 'a', &err);
    assert_int_equal(n, 0);
    assert_non_null(err);
    assert_int_equal(err->type, DG_ERROR_SERIAL);
    assert_string_equal(err->msg, "Failed to wrote to serial port: (unset)");
    dg_error_free(err);
}


static void
test_write_byte2(void **state)
{
    will_return(__wrap_write, 44);
    will_return(__wrap_write, 1);
    will_return(__wrap_write, 0);

    dg_error_t *err = NULL;
    int n = dg_serial_write_byte(44, 'a', &err);
    assert_int_equal(n, 0);
    assert_non_null(err);
    assert_int_equal(err->type, DG_ERROR_SERIAL);
    assert_string_equal(err->msg, "Got unexpected EOF from serial port");
    dg_error_free(err);
}


static void
test_write_byte3(void **state)
{
    will_return(__wrap_write, 44);
    will_return(__wrap_write, 1);
    will_return(__wrap_write, 10);
    will_return(__wrap_write, "c");
    will_return(__wrap_read, 44);
    will_return(__wrap_read, 1);
    will_return(__wrap_read, 10);
    will_return(__wrap_read, "cccccccccc");

    dg_error_t *err = NULL;
    int n = dg_serial_write_byte(44, 'c', &err);
    assert_int_equal(n, true);
    assert_null(err);
}


static void
test_send_break1(void **state)
{
    will_return(__wrap_ioctl, 44);
    will_return(__wrap_ioctl, TCFLSH);
    will_return(__wrap_ioctl, 1);
    will_return(__wrap_ioctl, TCIOFLUSH);
    will_return(__wrap_ioctl, -1);

    dg_error_t *err = NULL;
    uint8_t b = dg_serial_send_break(44, &err);
    assert_int_equal(b, 0);
    assert_non_null(err);
    assert_int_equal(err->type, DG_ERROR_SERIAL);
    assert_string_equal(err->msg, "Failed to flush serial port: (unset)");
    dg_error_free(err);
}


static void
test_send_break2(void **state)
{
    will_return(__wrap_ioctl, 44);
    will_return(__wrap_ioctl, TCFLSH);
    will_return(__wrap_ioctl, 1);
    will_return(__wrap_ioctl, TCIOFLUSH);
    will_return(__wrap_ioctl, 0);
    will_return(__wrap_ioctl, 44);
    will_return(__wrap_ioctl, TIOCSBRK);
    will_return(__wrap_ioctl, -1);  // disable 3rd arg
    will_return(__wrap_ioctl, -1);

    dg_error_t *err = NULL;
    uint8_t b = dg_serial_send_break(44, &err);
    assert_int_equal(b, 0);
    assert_non_null(err);
    assert_int_equal(err->type, DG_ERROR_SERIAL);
    assert_string_equal(err->msg, "Failed to start break in serial port: (unset)");
    dg_error_free(err);
}


static void
test_send_break3(void **state)
{
    will_return(__wrap_ioctl, 44);
    will_return(__wrap_ioctl, TCFLSH);
    will_return(__wrap_ioctl, 1);
    will_return(__wrap_ioctl, TCIOFLUSH);
    will_return(__wrap_ioctl, 0);
    will_return(__wrap_ioctl, 44);
    will_return(__wrap_ioctl, TIOCSBRK);
    will_return(__wrap_ioctl, -1);  // disable 3rd arg
    will_return(__wrap_ioctl, 0);
    will_return(__wrap_usleep, 15000);
    will_return(__wrap_ioctl, 44);
    will_return(__wrap_ioctl, TIOCCBRK);
    will_return(__wrap_ioctl, -1);  // disable 3rd arg
    will_return(__wrap_ioctl, -1);

    dg_error_t *err = NULL;
    uint8_t b = dg_serial_send_break(44, &err);
    assert_int_equal(b, 0);
    assert_non_null(err);
    assert_int_equal(err->type, DG_ERROR_SERIAL);
    assert_string_equal(err->msg, "Failed to finish break in serial port: (unset)");
    dg_error_free(err);
}


static void
test_send_break4(void **state)
{
    uint8_t z = 0x00;
    uint8_t f = 0xff;
    uint8_t c = 0x55;
    will_return(__wrap_ioctl, 44);
    will_return(__wrap_ioctl, TCFLSH);
    will_return(__wrap_ioctl, 1);
    will_return(__wrap_ioctl, TCIOFLUSH);
    will_return(__wrap_ioctl, 0);
    will_return(__wrap_ioctl, 44);
    will_return(__wrap_ioctl, TIOCSBRK);
    will_return(__wrap_ioctl, -1);  // disable 3rd arg
    will_return(__wrap_ioctl, 0);
    will_return(__wrap_usleep, 15000);
    will_return(__wrap_ioctl, 44);
    will_return(__wrap_ioctl, TIOCCBRK);
    will_return(__wrap_ioctl, -1);  // disable 3rd arg
    will_return(__wrap_ioctl, 0);
    will_return(__wrap_read, 44);
    will_return(__wrap_read, 1);
    will_return(__wrap_read, 1);
    will_return(__wrap_read, &z);
    will_return(__wrap_read, 44);
    will_return(__wrap_read, 1);
    will_return(__wrap_read, 1);
    will_return(__wrap_read, &z);
    will_return(__wrap_read, 44);
    will_return(__wrap_read, 1);
    will_return(__wrap_read, 1);
    will_return(__wrap_read, &f);
    will_return(__wrap_read, 44);
    will_return(__wrap_read, 1);
    will_return(__wrap_read, 1);
    will_return(__wrap_read, &c);

    dg_error_t *err = NULL;
    uint8_t b = dg_serial_send_break(44, &err);
    assert_int_equal(b, 0x55);
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
        unit_test(test_read1),
        unit_test(test_read2),
        unit_test(test_read3),
        unit_test(test_read_byte1),
        unit_test(test_read_byte2),
        unit_test(test_read_byte3),
        unit_test(test_read_word1),
        unit_test(test_read_word2),
        unit_test(test_read_word3),
        unit_test(test_write1),
        unit_test(test_write2),
        unit_test(test_write3),
        unit_test(test_write_byte1),
        unit_test(test_write_byte2),
        unit_test(test_write_byte3),
        unit_test(test_send_break1),
        unit_test(test_send_break2),
        unit_test(test_send_break3),
        unit_test(test_send_break4),

        // dg_serial_flush and dg_serial_recv_break are tested as side effect.
    };
    return run_tests(tests);
}
