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

#include <stdbool.h>
#include <stdint.h>

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


const dg_debugwire_device_t*
dg_debugwire_guess_device(dg_serial_port_t *sp, dg_error_t **err)
{
    if (sp == NULL || err == NULL || *err != NULL)
        return NULL;

    uint16_t sign = dg_debugwire_get_signature(sp, err);
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


uint16_t
dg_debugwire_get_signature(dg_serial_port_t *sp, dg_error_t **err)
{
    if (sp == NULL || err == NULL || *err != NULL)
        return 0;

    if (!dg_serial_port_write_byte(sp, 0xf3, err))
        return 0;

    uint16_t rv = dg_serial_port_read_word(sp, err);
    if (*err != NULL)
        return 0;

    return rv;
}


bool
dg_debugwire_disable(dg_serial_port_t *sp, dg_error_t **err)
{
    if (sp == NULL || err == NULL || *err != NULL)
        return false;

    return dg_serial_port_write_byte(sp, 0x06, err);
}


bool
dg_debugwire_reset(dg_serial_port_t *sp, dg_error_t **err)
{
    if (sp == NULL || err == NULL || *err != NULL)
        return false;

    if (!dg_serial_port_break(sp, err))
        return false;

    if (!dg_serial_port_write_byte(sp, 0x07, err))
        return false;

    uint8_t b;

    do {
        b = dg_serial_port_read_byte(sp, err);
        if (*err != NULL)
            return false;
    }
    while (b == 0x00 || b == 0xff);

    if (b != 0x55) {
        *err = dg_error_new_printf(DG_ERROR_SERIAL,
            "Bad break sent from MCU. Expected 0x55, got 0x%02x", b);
        return false;
    }

    return true;
}


bool
dg_debugwire_write_registers(dg_serial_port_t *sp, uint8_t start,
    const uint8_t *values, uint8_t values_len, dg_error_t **err)
{
    if (sp == NULL || err == NULL || *err != NULL)
        return false;

    const uint8_t b[10] = {
        0x66,
        0xc2, 0x05,
        0xd0, 0x00, start,
        0xd1, 0x00, start + values_len,
        0x20,
    };
    if (10 != dg_serial_port_write(sp, b, 10, err) || *err != NULL)
        return false;

    return values_len == dg_serial_port_write(sp, values, values_len, err)
        && *err == NULL;
}


bool
dg_debugwire_read_registers(dg_serial_port_t *sp, uint8_t start,
    uint8_t *values, uint8_t values_len, dg_error_t **err)
{
    if (sp == NULL || err == NULL || *err != NULL)
        return false;

    const uint8_t b[10] = {
        0x66,
        0xc2, 0x01,
        0xd0, 0x00, start,
        0xd1, 0x00, start + values_len,
        0x20,
    };
    if (10 != dg_serial_port_write(sp, b, 10, err) || *err != NULL)
        return false;

    return values_len == dg_serial_port_read(sp, values, values_len, err)
        && *err == NULL;
}


bool
dg_debugwire_write_instruction(dg_serial_port_t *sp, uint16_t inst,
    dg_error_t **err)
{
    if (sp == NULL || err == NULL || *err != NULL)
        return false;

    const uint8_t b[5] = {
        0x64,
        0xd2,
        inst >> 8,
        inst,
        0x23,
    };
    return 5 == dg_serial_port_write(sp, b, 5, err) && *err == NULL;
}


bool
dg_debugwire_instruction_in(dg_serial_port_t *sp, uint8_t address, uint8_t reg,
    dg_error_t **err)
{
    if (sp == NULL || err == NULL || *err != NULL)
        return false;

    return dg_debugwire_write_instruction(sp,
        0xb000 | ((address & 0x30) << 5) | ((reg & 0x1f) << 4) | (address & 0x0f),
        err) && *err == NULL;
}


bool
dg_debugwire_instruction_out(dg_serial_port_t *sp, uint8_t address, uint8_t reg,
    dg_error_t **err)
{
    if (sp == NULL || err == NULL || *err != NULL)
        return false;

    return dg_debugwire_write_instruction(sp,
        0xb800 | ((address & 0x30) << 5) | ((reg & 0x1f) << 4) | (address & 0x0f),
        err) && *err == NULL;
}


char*
dg_debugwire_get_fuses(dg_serial_port_t *sp, dg_error_t **err)
{
    if (sp == NULL || err == NULL || *err != NULL)
        return NULL;

    const dg_debugwire_device_t *dev = dg_debugwire_guess_device(sp, err);
    if (dev == NULL || *err != NULL)
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

        if (!dg_debugwire_write_registers(sp, 29, b, 3, err) || *err != NULL) {
            dg_string_free(rv, true);
            return NULL;
        }

        if (!dg_debugwire_instruction_out(sp, dev->spmcsr, 29, err) || *err != NULL) {
            dg_string_free(rv, true);
            return NULL;
        }

        if (!dg_debugwire_write_instruction(sp, 0x95c8, err) || *err != NULL) {
            dg_string_free(rv, true);
            return NULL;
        }

        uint8_t r;
        if (!dg_debugwire_read_registers(sp, 0, &r, 1, err) || *err != NULL) {
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
