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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "debugwire.h"
#include "error.h"
#include "gdbserver.h"
#include "serial.h"
#include "utils.h"


static void
print_help(const char *host, const char *port)
{
    printf(
        "usage:\n"
        "    dwire-gdb [-h|-v|-i|-f|-z] [-d] [-s SERIAL_PORT] [-b BAUDRATE]\n"
        "              [-t HOST] [-p PORT]\n"
        "              - A GDB server for AVR 8 bit microcontrollers, using\n"
        "                debugWire protocol through USB-to-TTL adapters.\n"
        "\n"
        "optional arguments:\n"
        "    -h              show this help message and exit\n"
        "    -v              show version and exit\n"
        "    -i              detect target mcu signature and exit\n"
        "    -f              detect target mcu fuses and exit\n"
        "    -z              disable debugWire and exit\n"
        "    -d              enable debug\n"
        "    -s SERIAL_PORT  set serial port to connect to (e.g. /dev/ttyUSB0,\n"
        "                    default: detect)\n"
        "    -b BAUDRATE     set serial port baud rate (default: detect)\n"
        "    -t HOST         set server listen address (default: %s)\n"
        "    -p PORT         set server listen port (default: %s)\n",
        host, port);
}


static void
print_usage(void)
{
    printf("usage: dwire-gdb [-h|-v|-i|-f|-z] [-d] [-s SERIAL_PORT] [-b BAUDRATE] "
        "[-t HOST] [-p PORT]\n");
}


int
main(int argc, char **argv)
{
    int rv = 0;
    dg_error_t *err = NULL;

    bool identify = false;
    bool fuses = false;
    bool disable = false;
    bool debug = false;

    char *serial_port = NULL;
    uint32_t baudrate = 0;
    char *host = NULL;
    char *port = NULL;

    const char *default_host = "127.0.0.1";
    const char *default_port = "4444";

    for (size_t i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
                case 'h':
                    print_help(default_host, default_port);
                    goto cleanup;
                case 'v':
                    printf("%s\n", PACKAGE_STRING);
                    goto cleanup;
                case 'i':
                    identify = true;
                    break;
                case 'f':
                    fuses = true;
                    break;
                case 'z':
                    disable = true;
                    break;
                case 'd':
                    debug = true;
                    break;
                case 's':
                    if (argv[i][2] != '\0')
                        serial_port = dg_strdup(argv[i] + 2);
                    else
                        serial_port = dg_strdup(argv[++i]);
                    break;
                case 'b':
                    if (argv[i][2] != '\0')
                        baudrate = strtoul(argv[i] + 2, NULL, 10);
                    else
                        baudrate = strtoul(argv[++i], NULL, 10);
                    break;
                case 't':
                    if (argv[i][2] != '\0')
                        host = dg_strdup(argv[i] + 2);
                    else
                        host = dg_strdup(argv[++i]);
                    break;
                case 'p':
                    if (argv[i][2] != '\0')
                        port = dg_strdup(argv[i] + 2);
                    else
                        port = dg_strdup(argv[++i]);
                    break;
                default:
                    print_usage();
                    fprintf(stderr, PACKAGE_NAME ": error: invalid argument: -%c\n",
                        argv[i][1]);
                    rv = 1;
                    goto cleanup;
            }
        }
    }

    dg_debug_set(debug);

    dg_serial_port_t *sp = dg_serial_port_new(serial_port, baudrate, &err);
    if (sp == NULL || err != NULL)
        goto cleanup;

    if (identify) {
        const dg_debugwire_device_t *dev = dg_debugwire_guess_device(sp, &err);
        if (dev != NULL && err == NULL)
            printf("Target device: %s\n", dev->name);
    }
    else if (fuses) {

    }
    else if (disable) {
        if (dg_debugwire_disable(sp, &err) && err == NULL)
            printf("Target device reseted. The device can be flashed using SPI now. "
                "This must be done WITHOUT removing power from the device.\n");
    }
    else {
        // run gdbserver here!
    }

cleanup2:
    dg_serial_port_free(sp);

cleanup:
    if (err != NULL) {
        dg_error_print(err);
        dg_error_free(err);
    }

    free(serial_port);
    free(host);
    free(port);

    return rv;
}
