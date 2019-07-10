/*
 * dwire-gdb: A GDB server for AVR 8 bit microcontrollers, using debugWire
 *            protocol through USB-to-TTL adapters.
 * Copyright (C) 2019 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 *
 * This program can be distributed under the terms of the BSD License.
 * See the file LICENSE.
 */

#pragma once

#include <stdbool.h>
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
char** dg_str_split(const char *str, char c, size_t max_pieces);
void dg_strv_free(char **strv);


// string

typedef struct {
    char *str;
    size_t len;
    size_t allocated_len;
} dg_string_t;

dg_string_t* dg_string_new(void);
char* dg_string_free(dg_string_t *str, bool free_str);
dg_string_t* dg_string_append_len(dg_string_t *str, const char *suffix, size_t len);
dg_string_t* dg_string_append(dg_string_t *str, const char *suffix);
dg_string_t* dg_string_append_c(dg_string_t *str, char c);
dg_string_t* dg_string_append_printf(dg_string_t *str, const char *format, ...);
