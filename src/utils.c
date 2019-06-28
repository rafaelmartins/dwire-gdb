/*
 * dwire-gdb: A GDB server for AVR 8 bit microcontrollers, using debugWire
 *            protocol through USB-to-TTL adapters.
 * Copyright (C) 2019 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 *
 * This program can be distributed under the terms of the BSD License.
 * See the file LICENSE.
 */

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "utils.h"


void*
dg_malloc(size_t size)
{
    // simple things simple!
    void *rv = malloc(size);
    if (rv == NULL) {
        fprintf(stderr, "fatal: Failed to allocate memory!\n");
        abort();
    }
    return rv;
}


void*
dg_realloc(void *ptr, size_t size)
{
    // simple things even simpler :P
    void *rv = realloc(ptr, size);
    if (rv == NULL && size != 0) {
        fprintf(stderr, "fatal: Failed to reallocate memory!\n");
        free(ptr);
        abort();
    }
    return rv;
}


char*
dg_strdup(const char *s)
{
    if (s == NULL)
        return NULL;
    size_t l = strlen(s);
    char *tmp = malloc(l + 1);
    if (tmp == NULL)
        return NULL;
    memcpy(tmp, s, l + 1);
    return tmp;
}


char*
dg_strndup(const char *s, size_t n)
{
    if (s == NULL)
        return NULL;
    size_t l = strnlen(s, n);
    char *tmp = malloc(l + 1);
    if (tmp == NULL)
        return NULL;
    memcpy(tmp, s, l);
    tmp[l] = '\0';
    return tmp;
}


char*
dg_strdup_vprintf(const char *format, va_list ap)
{
    va_list ap2;
    va_copy(ap2, ap);
    int l = vsnprintf(NULL, 0, format, ap2);
    va_end(ap2);
    if (l < 0)
        return NULL;
    char *tmp = malloc(l + 1);
    if (!tmp)
        return NULL;
    int l2 = vsnprintf(tmp, l + 1, format, ap);
    if (l2 < 0) {
        free(tmp);
        return NULL;
    }
    return tmp;
}


char*
dg_strdup_printf(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    char *tmp = dg_strdup_vprintf(format, ap);
    va_end(ap);
    return tmp;
}
