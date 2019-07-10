/*
 * dwire-gdb: A GDB server for AVR 8 bit microcontrollers, using debugWire
 *            protocol through USB-to-TTL adapters.
 * Copyright (C) 2019 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 *
 * This program can be distributed under the terms of the BSD License.
 * See the file LICENSE.
 */

#pragma once

#include "debugwire.h"
#include "error.h"

int dg_gdbserver_run(dg_debugwire_t *dw, const char *host, const char *port,
    dg_error_t **err);
