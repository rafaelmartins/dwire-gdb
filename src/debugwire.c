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

#include <glob.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "debug.h"
#include "error.h"
#include "serial.h"
#include "utils.h"
#include "debugwire.h"

// FIXME: I'm only listing here the devices I own.
static const dg_debugwire_device_t devices[] = {
    {"ATtiny84", 0x930c, 0x37},
    {"ATtiny85", 0x930b, 0x37},
    {NULL, 0, 0},
};

static uint8_t tmp_reg[4] = {0};
static uint16_t tmp_pc = 0;


static const dg_debugwire_device_t*
guess_device(dg_debugwire_t *dw, dg_error_t **err)
{
    if (dw == NULL || err == NULL || *err != NULL)
        return NULL;

    uint16_t sign = dg_debugwire_get_signature(dw, err);
    if (sign == 0 || *err != NULL)
        return NULL;

    for (size_t i = 0; devices[i].name != NULL; i++) {
        if (sign == devices[i].signature)
            return &devices[i];
    }

    *err = dg_error_new_printf(DG_ERROR_DEBUGWIRE,
        "Target device signature not recognized: 0x%04x", sign);

    return NULL;
}


static char*
guess_port(dg_error_t **err)
{
    if (err == NULL || *err != NULL)
        return NULL;

    glob_t globbuf;
    glob("/dev/ttyUSB*", 0, NULL, &globbuf);

    if (globbuf.gl_pathc == 1) {
        dg_debug_printf(" * Detected serial port: %s\n", globbuf.gl_pathv[0]);
        char *rv = dg_strdup(globbuf.gl_pathv[0]);
        globfree(&globbuf);
        return rv;
    }

    if (globbuf.gl_pathc == 0) {
        *err = dg_error_new_printf(DG_ERROR_DEBUGWIRE, "No serial port found");
        globfree(&globbuf);
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

    *err = dg_error_new_printf(DG_ERROR_DEBUGWIRE, msg->str);

    dg_string_free(msg, true);
    globfree(&globbuf);

    return NULL;
}


static uint32_t
guess_baudrate(const char *device, dg_error_t **err)
{
    if (err == NULL || *err != NULL)
        return 0;

    // max supported cpu freq is 20mhz. there are faster avrs, but their usually
    // have PDI

    for (size_t i = 20; i > 0; i--) {
        uint32_t baudrate = (i * 1000000) / 128;

        int fd = dg_serial_open(device, baudrate, err);
        if (*err != NULL)
            return 0;

        uint8_t b = dg_serial_send_break(fd, err);
        close(fd);
        if (*err != NULL)
            return 0;

        if (b == 0x55) {
            dg_debug_printf(" * Detected baudrate: %d\n", baudrate);
            return baudrate;
        }

        usleep(10000);
    }

    *err = dg_error_new_printf(DG_ERROR_DEBUGWIRE,
        "Failed to detect baudrate for serial port (%s)", device);

    return 0;
}


dg_debugwire_t*
dg_debugwire_new(const char *device, uint32_t baudrate, dg_error_t **err)
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

    int fd = dg_serial_open(dev, baudrate, err);
    if (fd < 0 || *err != NULL) {
        free(dev);
        return NULL;
    }

    uint8_t b = dg_serial_send_break(fd, err);
    if (*err != NULL) {
        free(dev);
        close(fd);
        return NULL;
    }

    if (b != 0x55) {
        *err = dg_error_new_printf(DG_ERROR_DEBUGWIRE,
            "Bad break response from MCU. Expected 0x55, got 0x%02x", b);
        return false;
    }

    dg_debugwire_t *rv = dg_malloc(sizeof(dg_debugwire_t));
    rv->device = dev;
    rv->baudrate = baudrate;
    rv->fd = fd;
    rv->dev = guess_device(rv, err);
    if (rv->dev == NULL || *err != NULL) {
        dg_debugwire_free(rv);
        return NULL;
    }

    return rv;
}


void
dg_debugwire_free(dg_debugwire_t *dw)
{
    if (dw == NULL)
        return;

    free(dw->device);
    close(dw->fd);
    free(dw);
}


uint16_t
dg_debugwire_get_signature(dg_debugwire_t *dw, dg_error_t **err)
{
    if (dw == NULL || err == NULL || *err != NULL)
        return 0;

    if (!dg_serial_write_byte(dw->fd, 0xf3, err))
        return 0;

    uint16_t rv = dg_serial_read_word(dw->fd, err);
    if (*err != NULL)
        return 0;

    return rv;
}


bool
dg_debugwire_set_pc(dg_debugwire_t *dw, uint16_t pc, dg_error_t **err)
{
    if (dw == NULL || err == NULL || *err != NULL)
        return false;

    const uint8_t b[3] = {
        0xd0,
        pc >> 8, pc,
    };
    return 3 == dg_serial_write(dw->fd, b, 3, err) && *err == NULL;
}


uint16_t
dg_debugwire_get_pc(dg_debugwire_t *dw, dg_error_t **err)
{
    if (dw == NULL || err == NULL || *err != NULL)
        return 0;

    if (!dg_serial_write_byte(dw->fd, 0xf0, err))
        return 0;

    uint16_t rv = dg_serial_read_word(dw->fd, err);
    if (*err != NULL)
        return 0;

    if (rv > 0)
        rv -= 1;  // is this *always* called after break?

    return rv;
}


bool
dg_debugwire_disable(dg_debugwire_t *dw, dg_error_t **err)
{
    if (dw == NULL || err == NULL || *err != NULL)
        return false;

    return dg_serial_write_byte(dw->fd, 0x06, err);
}


bool
dg_debugwire_reset(dg_debugwire_t *dw, dg_error_t **err)
{
    if (dw == NULL || err == NULL || *err != NULL)
        return false;

    if (!dg_serial_send_break(dw->fd, err))
        return false;

    if (!dg_serial_write_byte(dw->fd, 0x07, err))
        return false;

    uint8_t b = dg_serial_recv_break(dw->fd, err);

    if (b != 0x55) {
        *err = dg_error_new_printf(DG_ERROR_DEBUGWIRE,
            "Bad break sent from MCU. Expected 0x55, got 0x%02x", b);
        return false;
    }

    return true;
}


bool
dg_debugwire_write_registers(dg_debugwire_t *dw, uint8_t start,
    const uint8_t *values, uint8_t values_len, dg_error_t **err)
{
    if (dw == NULL || err == NULL || *err != NULL)
        return false;

    const uint8_t b[10] = {
        0x66,
        0xc2, 0x05,
        0xd0, 0x00, start,
        0xd1, 0x00, start + values_len,
        0x20,
    };
    if (10 != dg_serial_write(dw->fd, b, 10, err) || *err != NULL)
        return false;

    return values_len == dg_serial_write(dw->fd, values, values_len, err) && *err == NULL;
}


bool
dg_debugwire_read_registers(dg_debugwire_t *dw, uint8_t start,
    uint8_t *values, uint8_t values_len, dg_error_t **err)
{
    if (dw == NULL || err == NULL || *err != NULL)
        return false;

    const uint8_t b[10] = {
        0x66,
        0xc2, 0x01,
        0xd0, 0x00, start,
        0xd1, 0x00, start + values_len,
        0x20,
    };
    if (10 != dg_serial_write(dw->fd, b, 10, err) || *err != NULL)
        return false;

    return values_len == dg_serial_read(dw->fd, values, values_len, err) && *err == NULL;
}


bool
dg_debugwire_cache_pc(dg_debugwire_t *dw, dg_error_t **err)
{
    if (dw == NULL || err == NULL || *err != NULL)
        return false;

    tmp_pc = dg_debugwire_get_pc(dw, err);
    if (*err != NULL)
        return false;

    dg_debug_printf("PC = 0x%02x\n", tmp_pc);

    return true;
}


bool
dg_debugwire_restore_pc(dg_debugwire_t *dw, dg_error_t **err)
{
    if (dw == NULL || err == NULL || *err != NULL)
        return false;

    return dg_debugwire_set_pc(dw, tmp_pc, err) && *err == NULL;
}


bool
dg_debugwire_cache_yz(dg_debugwire_t *dw, dg_error_t **err)
{
    if (dw == NULL || err == NULL || *err != NULL)
        return false;

    if (!dg_debugwire_read_registers(dw, 28, tmp_reg, 4, err) || *err != NULL)
        return false;

    for (size_t i = 0; i < 4; i++)
        dg_debug_printf("R%d = 0x%02x\n", i + 28, tmp_reg[i]);

    return true;
}


bool
dg_debugwire_restore_yz(dg_debugwire_t *dw, dg_error_t **err)
{
    if (dw == NULL || err == NULL || *err != NULL)
        return false;

    return dg_debugwire_write_registers(dw, 28, tmp_reg, 4, err) && *err == NULL;
}


bool
dg_debugwire_read_sram(dg_debugwire_t *dw, uint16_t start, uint8_t *values,
    uint16_t values_len, dg_error_t **err)
{
    if (dw == NULL || err == NULL || *err != NULL)
        return false;

    uint8_t b[2] = {
        start, start >> 8,
    };

    if (!dg_debugwire_write_registers(dw, 30, b, 2, err) || *err != NULL)
        return false;

    uint8_t c[10] = {
        0x66,
        0xc2, 0x00,
        0xd0, 0x00, 0x00,
        0xd1, (values_len * 2) >> 8, values_len * 2,
        0x20,
    };
    if (10 != dg_serial_write(dw->fd, c, 10, err) || *err != NULL)
        return false;

    return values_len == dg_serial_read(dw->fd, values, values_len, err)
        && *err == NULL;
}


bool
dg_debugwire_write_instruction(dg_debugwire_t *dw, uint16_t inst,
    dg_error_t **err)
{
    if (dw == NULL || err == NULL || *err != NULL)
        return false;

    const uint8_t b[5] = {
        0x64,
        0xd2, inst >> 8, inst,
        0x23,
    };
    return 5 == dg_serial_write(dw->fd, b, 5, err) && *err == NULL;
}


bool
dg_debugwire_instruction_in(dg_debugwire_t *dw, uint8_t address, uint8_t reg,
    dg_error_t **err)
{
    if (dw == NULL || err == NULL || *err != NULL)
        return false;

    return dg_debugwire_write_instruction(dw,
        0xb000 | ((address & 0x30) << 5) | ((reg & 0x1f) << 4) | (address & 0x0f),
        err) && *err == NULL;
}


bool
dg_debugwire_instruction_out(dg_debugwire_t *dw, uint8_t address, uint8_t reg,
    dg_error_t **err)
{
    if (dw == NULL || err == NULL || *err != NULL)
        return false;

    return dg_debugwire_write_instruction(dw,
        0xb800 | ((address & 0x30) << 5) | ((reg & 0x1f) << 4) | (address & 0x0f),
        err) && *err == NULL;
}


char*
dg_debugwire_get_fuses(dg_debugwire_t *dw, dg_error_t **err)
{
    if (dw == NULL || err == NULL || *err != NULL)
        return NULL;

    uint8_t b[3] = {
        1 << 3 | 1 << 0,  // RFLB | SELFPRGEN
        0,
        0,
    };

    const uint8_t f[4] = {
        0,  // low fuse
        3,  // high fuse
        2,  // extended fuse
        1   // lockbit
    };

    dg_string_t *rv = dg_string_new();

    for (size_t i = 0; i < 4; i++) {
        b[1] = f[i];

        if (!dg_debugwire_write_registers(dw, 29, b, 3, err) || *err != NULL) {
            dg_string_free(rv, true);
            return NULL;
        }

        if (!dg_debugwire_instruction_out(dw, dw->dev->spmcsr, 29, err) || *err != NULL) {
            dg_string_free(rv, true);
            return NULL;
        }

        if (!dg_debugwire_write_instruction(dw, 0x95c8, err) || *err != NULL) {
            dg_string_free(rv, true);
            return NULL;
        }

        uint8_t r;
        if (!dg_debugwire_read_registers(dw, 0, &r, 1, err) || *err != NULL) {
            dg_string_free(rv, true);
            return NULL;
        }

        switch (f[i]) {
            case 0:
                dg_string_append(rv, "low=");
                break;
            case 1:
                dg_string_append(rv, "lockbit=");
                break;
            case 2:
                dg_string_append(rv, "extended=");
                break;
            case 3:
                dg_string_append(rv, "high=");
                break;
        }

        dg_string_append_printf(rv, "0x%02x", r);

        if (i < 3)
            dg_string_append(rv, ", ");
    }

    return dg_string_free(rv, false);
}
