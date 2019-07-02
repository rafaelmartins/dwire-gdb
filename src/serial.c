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

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stropts.h>
#include <asm/termbits.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "debug.h"
#include "error.h"
#include "utils.h"
#include "serial.h"


int
dg_serial_open(const char *device, uint32_t baudrate, dg_error_t **err)
{
    if (err == NULL || *err != NULL)
        return -1;

    int fd = open(device, O_RDWR);
    if (fd < 0) {
        *err = dg_error_new_errno_printf(DG_ERROR_SERIAL, errno,
            "Failed to open serial port (%s [%d])", device, baudrate);
        return fd;
    }

    struct termios2 cfg = {
        .c_cflag = BOTHER | CS8 | CLOCAL,
        .c_iflag = IGNPAR,
        .c_oflag = 0,
        .c_lflag = 0,
        .c_ispeed = baudrate,
        .c_ospeed = baudrate,
    };
    cfg.c_cc[VMIN] = 0;
    cfg.c_cc[VTIME] = 5;

    int rv = ioctl(fd, TCSETS2, &cfg);
    if (rv != 0) {
        *err = dg_error_new_errno_printf(DG_ERROR_SERIAL, errno,
            "Failed to set termios2 properties (%s [%d])", device, baudrate);
        close(fd);
        return rv;
    }

    usleep(30000);

    rv = dg_serial_flush(fd, err);
    if (rv != 0) {
        *err = dg_error_new_printf(DG_ERROR_SERIAL,
            "Failed to flush serial port for startup (%s [%d])", device,
            baudrate);
        close(fd);
        return rv;
    }

    return fd;
}


int
dg_serial_read(int fd, uint8_t *buf, size_t len, dg_error_t **err)
{
    if (err == NULL || *err != NULL)
        return 0;

    int n = 0;

    while (n < len) {
        int c = read(fd, buf + n, len - n);
        if (c < 0) {
            *err = dg_error_new_errno_printf(DG_ERROR_SERIAL, errno,
                "Failed to read from serial port");
            return c;
        }
        if (c == 0 && n != len) {
            *err = dg_error_new_printf(DG_ERROR_SERIAL,
                "Got unexpected EOF from serial port");
            return -1;
        }
        for (size_t i = n; i < n + c; i++)
            dg_debug_printf("<<< 0x%02x\n", buf[i]);
        n += c;
    }

    return n;
}


uint8_t
dg_serial_read_byte(int fd, dg_error_t **err)
{
    if (err == NULL || *err != NULL)
        return 0;

    uint8_t c;

    if (1 != dg_serial_read(fd, &c, 1, err))
        return 0;

    return c;
}


uint16_t
dg_serial_read_word(int fd, dg_error_t **err)
{
    if (err == NULL || *err != NULL)
        return 0;

    uint8_t c[2];

    if (2 != dg_serial_read(fd, c, 2, err))
        return 0;

    return (((uint16_t) c[0]) << 8) | c[1];
}


int
dg_serial_write(int fd, const uint8_t *buf, size_t len, dg_error_t **err)
{
    if (err == NULL || *err != NULL)
        return 0;

    int n = 0;

    while (n < len) {
        int c = write(fd, buf + n, len - n);
        if (c < 0) {
            *err = dg_error_new_errno_printf(DG_ERROR_SERIAL, errno,
                "Failed to wrote to serial port");
            return c;
        }
        if (c == 0 && n != len) {
            *err = dg_error_new_printf(DG_ERROR_SERIAL,
                "Got unexpected EOF from serial port");
            return -1;
        }
        for (size_t i = n; i < n + c; i++)
            dg_debug_printf(">>> 0x%02x\n", buf[i]);
        n += c;
    }

    uint8_t *b = dg_malloc(sizeof(uint8_t) * len);
    int r = dg_serial_read(fd, b, len, err);
    if (*err != NULL) {
        free(b);
        return -1;
    }

    for (size_t i = 0; i < len; i++) {
        if (buf[i] != b[i]) {
            *err = dg_error_new_printf(DG_ERROR_SERIAL,
                "Got unexpected byte echoed back. Expected 0x%02x, got 0x%02x",
                buf[i], b[i]);
            free(b);
            return -1;
        }
    }

    free(b);

    return n;
}


bool
dg_serial_write_byte(int fd, uint8_t b, dg_error_t **err)
{
    if (err == NULL || *err != NULL)
        return false;

    return 1 == dg_serial_write(fd, &b, 1, err);
}


int
dg_serial_flush(int fd, dg_error_t **err)
{
    if (err == NULL || *err != NULL)
        return 0;

    int rv = ioctl(fd, TCFLSH, TCIOFLUSH);
    if (rv != 0) {
        *err = dg_error_new_errno_printf(DG_ERROR_SERIAL, errno,
            "Failed to flush serial port");
    }

    return rv;
}


uint8_t
dg_serial_send_break(int fd, dg_error_t **err)
{
    if (err == NULL || *err != NULL)
        return 0;

    dg_debug_printf("> break\n");

    if (0 != dg_serial_flush(fd, err))
        return 0;

    int rv = ioctl(fd, TIOCSBRK);
    if (rv != 0) {
        *err = dg_error_new_errno_printf(DG_ERROR_SERIAL, errno,
            "Failed to start break in serial port");
        return 0;
    }

    // 15ms is a delay big enough for all supported baud rates
    if (0 != usleep(15000)) {
        *err = dg_error_new_errno_printf(DG_ERROR_SERIAL, errno,
            "Failed to start break delay in serial port");
        return 0;
    }

    rv = ioctl(fd, TIOCCBRK);
    if (rv != 0) {
        *err = dg_error_new_errno_printf(DG_ERROR_SERIAL, errno,
            "Failed to finish break in serial port");
        return false;
    }

    return dg_serial_recv_break(fd, err);
}


uint8_t
dg_serial_recv_break(int fd, dg_error_t **err)
{
    if (err == NULL || *err != NULL)
        return 0;

    uint8_t b;
    do {
        b = dg_serial_read_byte(fd, err);
        if (*err != NULL)
            return false;
    }
    while (b == 0x00 || b == 0xff);

    return b;
}
