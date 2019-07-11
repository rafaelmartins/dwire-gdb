/*
 * dwire-gdb: A GDB server for AVR 8 bit microcontrollers, using debugWire
 *            protocol through USB-to-TTL adapters.
 * Copyright (C) 2019 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 *
 * This program can be distributed under the terms of the BSD License.
 * See the file LICENSE.
 */

#define DG_STRING_CHUNK_SIZE 128

#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
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


char**
dg_str_split(const char *str, char c, size_t max_pieces)
{
    if (str == NULL)
        return NULL;
    char **rv = dg_malloc(sizeof(char*));
    size_t i, start = 0, count = 0;
    for (i = 0; i < strlen(str) + 1; i++) {
        if (str[0] == '\0')
            break;
        if ((str[i] == c && (!max_pieces || count + 1 < max_pieces)) || str[i] == '\0') {
            rv = dg_realloc(rv, (count + 1) * sizeof(char*));
            rv[count] = dg_malloc(i - start + 1);
            memcpy(rv[count], str + start, i - start);
            rv[count++][i - start] = '\0';
            start = i + 1;
        }
    }
    rv = dg_realloc(rv, (count + 1) * sizeof(char*));
    rv[count] = NULL;
    return rv;
}


void
dg_strv_free(char **strv)
{
    if (strv == NULL)
        return;
    for (size_t i = 0; strv[i] != NULL; i++)
        free(strv[i]);
    free(strv);
}


size_t
dg_strv_length(char **strv)
{
    if (strv == NULL)
        return 0;
    size_t i;
    for (i = 0; strv[i] != NULL; i++);
    return i;
}


dg_string_t*
dg_string_new(void)
{
    dg_string_t* rv = dg_malloc(sizeof(dg_string_t));
    rv->str = NULL;
    rv->len = 0;
    rv->allocated_len = 0;

    // initialize with empty string
    rv = dg_string_append(rv, "");

    return rv;
}


char*
dg_string_free(dg_string_t *str, bool free_str)
{
    if (str == NULL)
        return NULL;
    char *rv = NULL;
    if (free_str)
        free(str->str);
    else
        rv = str->str;
    free(str);
    return rv;
}


dg_string_t*
dg_string_append_len(dg_string_t *str, const char *suffix, size_t len)
{
    if (str == NULL)
        return NULL;
    if (suffix == NULL)
        return str;
    size_t old_len = str->len;
    str->len += len;
    if (str->len + 1 > str->allocated_len) {
        str->allocated_len = (((str->len + 1) / DG_STRING_CHUNK_SIZE) + 1) *
            DG_STRING_CHUNK_SIZE;
        str->str = dg_realloc(str->str, str->allocated_len);
    }
    memcpy(str->str + old_len, suffix, len);
    str->str[str->len] = '\0';
    return str;
}


dg_string_t*
dg_string_append(dg_string_t *str, const char *suffix)
{
    if (str == NULL)
        return NULL;
    const char *my_suffix = suffix == NULL ? "" : suffix;
    return dg_string_append_len(str, my_suffix, strlen(my_suffix));
}


dg_string_t*
dg_string_append_c(dg_string_t *str, char c)
{
    if (str == NULL)
        return NULL;
    size_t old_len = str->len;
    str->len += 1;
    if (str->len + 1 > str->allocated_len) {
        str->allocated_len = (((str->len + 1) / DG_STRING_CHUNK_SIZE) + 1) *
            DG_STRING_CHUNK_SIZE;
        str->str = dg_realloc(str->str, str->allocated_len);
    }
    str->str[old_len] = c;
    str->str[str->len] = '\0';
    return str;
}


dg_string_t*
dg_string_append_printf(dg_string_t *str, const char *format, ...)
{
    if (str == NULL)
        return NULL;
    va_list ap;
    va_start(ap, format);
    char *tmp = dg_strdup_vprintf(format, ap);
    va_end(ap);
    str = dg_string_append(str, tmp);
    free(tmp);
    return str;
}
