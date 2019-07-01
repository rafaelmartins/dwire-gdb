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
    uint8_t spmcsr;
} dg_debugwire_device_t;

typedef struct {
    dg_serial_port_t *sp;
    const dg_debugwire_device_t *dev;
} dg_debugwire_t;

dg_debugwire_t* dg_debugwire_new(const char *device, uint32_t baudrate,
    dg_error_t **err);
void dg_debugwire_free(dg_debugwire_t *dw);
uint16_t dg_debugwire_get_signature(dg_debugwire_t *dw, dg_error_t **err);
bool dg_debugwire_disable(dg_debugwire_t *dw, dg_error_t **err);
bool dg_debugwire_reset(dg_debugwire_t *dw, dg_error_t **err);
bool dg_debugwire_write_registers(dg_debugwire_t *dw, uint8_t start,
    const uint8_t *values, uint8_t values_len, dg_error_t **err);
bool dg_debugwire_read_registers(dg_debugwire_t *dw, uint8_t start,
    uint8_t *values, uint8_t values_len, dg_error_t **err);
bool dg_debugwire_write_instruction(dg_debugwire_t *dw, uint16_t inst,
    dg_error_t **err);
bool dg_debugwire_instruction_in(dg_debugwire_t *dw, uint8_t address,
    uint8_t reg, dg_error_t **err);
bool dg_debugwire_instruction_out(dg_debugwire_t *dw, uint8_t address,
    uint8_t reg, dg_error_t **err);
char* dg_debugwire_get_fuses(dg_debugwire_t *dw, dg_error_t **err);
