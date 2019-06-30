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
#include <stdbool.h>

#include "error.h"

typedef struct {
    char *device;
    uint32_t baudrate;
    int fd;
} dg_serial_port_t;

dg_serial_port_t* dg_serial_port_new(const char *device, uint32_t baudrate,
    dg_error_t **err);
void dg_serial_port_free(dg_serial_port_t *sp);
int dg_serial_port_flush(dg_serial_port_t *sp, dg_error_t **err);
bool dg_serial_port_break(dg_serial_port_t *sp, dg_error_t **err);
int dg_serial_port_read(dg_serial_port_t *sp, uint8_t *buf, size_t len,
    dg_error_t **err);
uint8_t dg_serial_port_read_byte(dg_serial_port_t *sp, dg_error_t **err);
uint16_t dg_serial_port_read_word(dg_serial_port_t *sp, dg_error_t **err);
int dg_serial_port_write(dg_serial_port_t *sp, const uint8_t *buf, size_t len,
    dg_error_t **err);
bool dg_serial_port_write_byte(dg_serial_port_t *sp, uint8_t b, dg_error_t **err);
