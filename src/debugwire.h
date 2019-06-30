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

uint16_t dg_debugwire_get_signature(dg_serial_port_t *sp, dg_error_t **err);
