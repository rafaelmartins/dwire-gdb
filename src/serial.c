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
#include <glob.h>

#include "debug.h"
#include "error.h"
#include "utils.h"
#include "serial.h"


static int
serial_read(int fd, uint8_t *buf, size_t len, dg_error_t **err)
{
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


static uint8_t
serial_read_byte(int fd, dg_error_t **err)
{
    uint8_t c;

    if (1 != serial_read(fd, &c, 1, err))
        return 0;

    return c;
}


static int
serial_flush(int fd, dg_error_t **err)
{
    int rv = ioctl(fd, TCFLSH, TCIOFLUSH);
    if (rv != 0) {
        *err = dg_error_new_errno_printf(DG_ERROR_SERIAL, errno,
            "Failed to flush serial port");
    }

    return rv;
}


static bool
serial_break(int fd, dg_error_t **err)
{
    dg_debug_printf("> break\n");

    if (0 != serial_flush(fd, err))
        return false;

    int rv = ioctl(fd, TIOCSBRK);
    if (rv != 0) {
        *err = dg_error_new_errno_printf(DG_ERROR_SERIAL, errno,
            "Failed to start break in serial port");
        return false;
    }

    // 15ms is a delay big enough for all supported baud rates
    if (0 != usleep(15000)) {
        *err = dg_error_new_errno_printf(DG_ERROR_SERIAL, errno,
            "Failed to start break delay in serial port");
        return false;
    }

    rv = ioctl(fd, TIOCCBRK);
    if (rv != 0) {
        *err = dg_error_new_errno_printf(DG_ERROR_SERIAL, errno,
            "Failed to finish break in serial port");
        return false;
    }

    uint8_t b;

    do {
        b = serial_read_byte(fd, err);
        if (*err != NULL)
            return false;
    }
    while (b == 0x00);

    if (b != 0x55) {
        *err = dg_error_new_printf(DG_ERROR_SERIAL,
            "Bad break response from MCU. Expected 0x55, got 0x%02x", b);
        return false;
    }

    return true;
}


static int
serial_open(const char *device, uint32_t baudrate, dg_error_t **err)
{
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
    cfg.c_cc[VTIME] = 10;

    int rv = ioctl(fd, TCSETS2, &cfg);
    if (rv != 0) {
        *err = dg_error_new_errno_printf(DG_ERROR_SERIAL, errno,
            "Failed to set termios2 properties (%s [%d])", device, baudrate);
        close(fd);
        return rv;
    }

    usleep(30000);

    rv = serial_flush(fd, err);
    if (rv != 0) {
        *err = dg_error_new_printf(DG_ERROR_SERIAL,
            "Failed to flush serial port for startup (%s [%d])", device,
            baudrate);
        close(fd);
        return rv;
    }

    return fd;
}


static char*
guess_port(dg_error_t **err)
{
    glob_t globbuf;
    glob("/dev/ttyUSB*", 0, NULL, &globbuf);

    if (globbuf.gl_pathc == 1) {
        dg_debug_printf(" * Detected serial port: %s\n", globbuf.gl_pathv[0]);
        return dg_strdup(globbuf.gl_pathv[0]);
    }

    if (globbuf.gl_pathc == 0) {
        *err = dg_error_new_printf(DG_ERROR_SERIAL, "No serial port found");
        return NULL;
    }

    dg_string_t *msg = dg_string_new();
    dg_string_append(msg, "More than one serial port found, please select one: ");
    for (size_t i = 0; i < globbuf.gl_pathc; i++) {
        dg_string_append(msg, globbuf.gl_pathv[i]);
        if (i < (globbuf.gl_pathc - 1))
            dg_string_append(msg, ", ");
    }
    dg_string_append_c(msg, '.');

    *err = dg_error_new_printf(DG_ERROR_SERIAL, msg->str);

    dg_string_free(msg, true);

    return NULL;
}


static uint32_t
guess_baudrate(const char *device, dg_error_t **err)
{
    // max supported cpu freq is 20mhz. there are faster avrs, but their usually
    // have PDI

    dg_error_t *tmp_err = NULL;
    dg_error_t *last_err = NULL;

    for (size_t i = 20; i > 0; i--) {
        uint32_t baudrate = (i * 1000000) / 128;

        int fd = serial_open(device, baudrate, &tmp_err);
        if (tmp_err != NULL) {
            dg_error_free(last_err);
            last_err = tmp_err;
            tmp_err = NULL;
            continue;
        }

        int rv = serial_break(fd, &tmp_err);
        close(fd);
        if (tmp_err != NULL) {
            dg_error_free(last_err);
            last_err = tmp_err;
            tmp_err = NULL;
            continue;
        }

        if (rv) {
            dg_debug_printf(" * Detected baudrate: %d\n", baudrate);
            return baudrate;
        }

        usleep(10000);
    }

    if (last_err == NULL) {
        *err = dg_error_new_printf(DG_ERROR_SERIAL,
            "Failed to detect baudrate for serial port (%s)", device);
    }
    else {
        *err = dg_error_new_printf(DG_ERROR_SERIAL,
            "Failed to detect baudrate for serial port (%s): %s", device,
            last_err->msg);
    }

    dg_error_free(last_err);

    return 0;
}


dg_serial_port_t*
dg_serial_port_new(const char *device, uint32_t baudrate, dg_error_t **err)
{
    if (err == NULL || *err != NULL)
        return NULL;

    char *dev = NULL;
    if (device == NULL) {
        dev = guess_port(err);
        if (dev == NULL || *err != NULL)
            return NULL;
    }
    else {
        dev = dg_strdup(device);
    }

    if (baudrate == 0) {
        baudrate = guess_baudrate(dev, err);
        if (baudrate == 0 || *err != NULL) {
            free(dev);
            return NULL;
        }
    }

    int fd = serial_open(dev, baudrate, err);
    if (fd < 0 || *err != NULL) {
        free(dev);
        return NULL;
    }

    int r = serial_break(fd, err);
    if (*err != NULL || !r) {
        free(dev);
        close(fd);
        return NULL;
    }

    dg_serial_port_t *rv = dg_malloc(sizeof(dg_serial_port_t));
    rv->device = dev;
    rv->baudrate = baudrate;
    rv->fd = fd;

    return rv;
}


void
dg_serial_port_free(dg_serial_port_t *sp)
{
    if (sp == NULL)
        return;

    free(sp->device);
    close(sp->fd);
    free(sp);
}


int
dg_serial_port_flush(dg_serial_port_t *sp, dg_error_t **err)
{
    if (sp == NULL || err == NULL || *err != NULL)
        return -1;

    return serial_flush(sp->fd, err);
}


bool
dg_serial_port_break(dg_serial_port_t *sp, dg_error_t **err)
{
    if (sp == NULL || err == NULL || *err != NULL)
        return false;

    return serial_break(sp->fd, err);
}


int
dg_serial_port_read(dg_serial_port_t *sp, uint8_t *buf, size_t len, dg_error_t **err)
{
    if (sp == NULL || err == NULL || *err != NULL)
        return -1;

    return serial_read(sp->fd, buf, len, err);
}


uint8_t
dg_serial_port_read_byte(dg_serial_port_t *sp, dg_error_t **err)
{
    if (sp == NULL || err == NULL || *err != NULL)
        return 0;

    return serial_read_byte(sp->fd, err);
}


uint16_t
dg_serial_port_read_word(dg_serial_port_t *sp, dg_error_t **err)
{
    if (sp == NULL || err == NULL || *err != NULL)
        return 0;

    uint8_t c[2];

    if (2 != serial_read(sp->fd, c, 2, err))
        return 0;

    return (((uint16_t) c[0]) << 8) | c[1];
}


int
dg_serial_port_write(dg_serial_port_t *sp, const uint8_t *buf, size_t len,
    dg_error_t **err)
{
    if (sp == NULL || err == NULL || *err != NULL)
        return -1;

    int n = 0;

    while (n < len) {
        int c = write(sp->fd, buf + n, len - n);
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
    int r = dg_serial_port_read(sp, b, len, err);
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
dg_serial_port_write_byte(dg_serial_port_t *sp, uint8_t b, dg_error_t **err)
{
    if (sp == NULL || err == NULL || *err != NULL)
        return false;

    return 1 == dg_serial_port_write(sp, &b, 1, err);
}
