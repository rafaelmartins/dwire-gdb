/*
 * dwire-gdb: A GDB server for AVR 8 bit microcontrollers, using debugWire
 *            protocol through USB-to-TTL adapters.
 * Copyright (C) 2019 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 *
 * This program can be distributed under the terms of the BSD License.
 * See the file LICENSE.
 */

#pragma once

#include <stddef.h>

typedef enum {
    DG_ERROR_UTILS = 1,
    DG_ERROR_SERIAL,
    DG_ERROR_GDBSERVER,
    DG_ERROR_DEBUGWIRE,
} dg_error_type_t;

typedef struct {
    char *msg;
    dg_error_type_t type;
} dg_error_t;

dg_error_t* dg_error_new(dg_error_type_t type, const char *msg);
dg_error_t* dg_error_new_printf(dg_error_type_t type, const char *format, ...);
dg_error_t* dg_error_new_errno(dg_error_type_t type, int errno_,
    const char *prefix);
dg_error_t* dg_error_new_errno_printf(dg_error_type_t type, int errno_,
    const char *prefix_format, ...);
void dg_error_print(dg_error_t *err);
void dg_error_free(dg_error_t *err);
