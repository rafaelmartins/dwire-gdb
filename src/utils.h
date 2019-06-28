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
#include <stdarg.h>


// memory

void* dg_malloc(size_t size);
void* dg_realloc(void *ptr, size_t size);


// strfuncs

char* dg_strdup(const char *s);
char* dg_strndup(const char *s, size_t n);
char* dg_strdup_vprintf(const char *format, va_list ap);
char* dg_strdup_printf(const char *format, ...);
