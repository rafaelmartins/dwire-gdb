/*
 * dwire-gdb: A GDB server for AVR 8 bit microcontrollers, using debugWire
 *            protocol through USB-to-TTL adapters.
 * Copyright (C) 2019 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 *
 * This program can be distributed under the terms of the BSD License.
 * See the file LICENSE.
 */

#pragma once

#include <stdint.h>

#include "error.h"
#include "serial.h"

typedef struct {
    const char *name;
    uint16_t signature;
} dg_debugwire_device_t;

const dg_debugwire_device_t* dg_debugwire_guess_device(dg_serial_port_t *sp,
    dg_error_t **err);
uint16_t dg_debugwire_get_signature(dg_serial_port_t *sp, dg_error_t **err);
bool dg_debugwire_disable(dg_serial_port_t *sp, dg_error_t **err);
bool dg_debugwire_reset(dg_serial_port_t *sp, dg_error_t **err);
