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
#include "debugwire.h"

// FIXME: I'm only listing here the devices I own.
static const dg_debugwire_device_t devices[] = {
    {"ATtiny84", 0x930c},
    {"ATtiny85", 0x930b},
    {NULL, 0},
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
        return 0;

    return dg_serial_port_write_byte(sp, 0x06, err);
}


bool
dg_debugwire_reset(dg_serial_port_t *sp, dg_error_t **err)
{
    if (sp == NULL || err == NULL || *err != NULL)
        return 0;

    if (!dg_serial_port_break(sp, err))
        return 0;

    if (!dg_serial_port_write_byte(sp, 0x07, err))
        return 0;

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
