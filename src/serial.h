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

int dg_serial_open(const char *device, uint32_t baudrate, dg_error_t **err);
int dg_serial_read(int fd, uint8_t *buf, size_t len, dg_error_t **err);
uint8_t dg_serial_read_byte(int fd, dg_error_t **err);
uint16_t dg_serial_read_word(int fd, dg_error_t **err);
int dg_serial_write(int fd, const uint8_t *buf, size_t len, dg_error_t **err);
bool dg_serial_write_byte(int fd, uint8_t b, dg_error_t **err);
int dg_serial_flush(int fd, dg_error_t **err);
uint8_t dg_serial_send_break(int fd, dg_error_t **err);
uint8_t dg_serial_recv_break(int fd, dg_error_t **err);
