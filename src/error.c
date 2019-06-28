/*
 * dwire-gdb: A GDB server for AVR 8 bit microcontrollers, using debugWire
 *            protocol through USB-to-TTL adapters.
 * Copyright (C) 2019 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 *
 * This program can be distributed under the terms of the BSD License.
 * See the file LICENSE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "error.h"
#include "utils.h"


dg_error_t*
dg_error_new(dg_error_type_t type, const char *msg)
{
    dg_error_t *err = dg_malloc(sizeof(dg_error_t));
    err->type = type;
    err->msg = dg_strdup(msg);
    return err;
}


dg_error_t*
dg_error_new_printf(dg_error_type_t type, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    char *tmp = dg_strdup_vprintf(format, ap);
    va_end(ap);
    dg_error_t *rv = dg_error_new(type, tmp);
    free(tmp);
    return rv;
}


dg_error_t*
dg_error_new_errno(dg_error_type_t type, int errno_, const char *prefix)
{
    if (errno == 0)
        return NULL;

    return dg_error_new_printf(type, "%s: %s.", prefix, strerror(errno_));
}


dg_error_t*
dg_error_new_errno_printf(dg_error_type_t type, int errno_,
    const char *prefix_format, ...)
{
    va_list ap;
    va_start(ap, prefix_format);
    char *tmp = dg_strdup_vprintf(prefix_format, ap);
    va_end(ap);
    dg_error_t *rv = dg_error_new_errno(type, errno_, tmp);
    free(tmp);
    return rv;
}


void
dg_error_print(dg_error_t *err)
{
    if (err == NULL)
        return;

    fprintf(stderr, PACKAGE_NAME ": ");

    switch(err->type) {
        case DG_ERROR_UTILS:
            fprintf(stderr, "error: utils: %s\n", err->msg);
            break;
        case DG_ERROR_SERIAL:
            fprintf(stderr, "error: serial: %s\n", err->msg);
            break;
        case DG_ERROR_GDBSERVER:
            fprintf(stderr, "error: gdbserver: %s\n", err->msg);
            break;
        case DG_ERROR_DEBUGWIRE:
            fprintf(stderr, "error: debugwire: %s\n", err->msg);
            break;
        default:
            fprintf(stderr, "error: %s\n", err->msg);
    }
}


void
dg_error_free(dg_error_t *err)
{
    if (err == NULL)
        return;
    free(err->msg);
    free(err);
}
