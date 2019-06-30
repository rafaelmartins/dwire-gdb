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


uint16_t
dg_debugwire_get_signature(dg_serial_port_t *sp, dg_error_t **err)
{
    if (sp == NULL || err == NULL || *err != NULL)
        return 0;

    if (!dg_serial_port_write_byte(sp, 0xf3, err))
        return 0;

    uint16_t rv = dg_serial_port_read_word(sp, err);
    if (rv == 0 || *err != NULL)
        return 0;

    return rv;
}
