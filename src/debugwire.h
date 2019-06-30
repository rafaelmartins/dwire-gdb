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

const dg_debugwire_device_t* dg_debugwire_guess_device(dg_serial_port_t *sp,
    dg_error_t **err);
uint16_t dg_debugwire_get_signature(dg_serial_port_t *sp, dg_error_t **err);
bool dg_debugwire_disable(dg_serial_port_t *sp, dg_error_t **err);
bool dg_debugwire_reset(dg_serial_port_t *sp, dg_error_t **err);
bool dg_debugwire_write_registers(dg_serial_port_t *sp, uint8_t start,
    const uint8_t *values, uint8_t values_len, dg_error_t **err);
bool dg_debugwire_read_registers(dg_serial_port_t *sp, uint8_t start,
    uint8_t *values, uint8_t values_len, dg_error_t **err);
bool dg_debugwire_write_instruction(dg_serial_port_t *sp, uint16_t inst,
    dg_error_t **err);
bool dg_debugwire_instruction_in(dg_serial_port_t *sp, uint8_t address,
    uint8_t reg, dg_error_t **err);
bool dg_debugwire_instruction_out(dg_serial_port_t *sp, uint8_t address,
    uint8_t reg, dg_error_t **err);
char* dg_debugwire_get_fuses(dg_serial_port_t *sp, dg_error_t **err);
