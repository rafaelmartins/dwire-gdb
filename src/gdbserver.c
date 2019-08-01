/*
 * dwire-gdb: A GDB server for AVR 8 bit microcontrollers, using debugWire
 *            protocol through USB-to-TTL adapters.
 * Copyright (C) 2019 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 *
 * This program can be distributed under the terms of the BSD License.
 * See the file LICENSE.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "debug.h"
#include "debugwire.h"
#include "error.h"
#include "utils.h"
#include "gdbserver.h"


static char*
get_ip(int af, const struct sockaddr *addr)
{
    char host[INET6_ADDRSTRLEN];
    if (af == AF_INET6) {
        struct sockaddr_in6 *a = (struct sockaddr_in6*) addr;
        inet_ntop(af, &(a->sin6_addr), host, INET6_ADDRSTRLEN);
    }
    else {
        struct sockaddr_in *a = (struct sockaddr_in*) addr;
        inet_ntop(af, &(a->sin_addr), host, INET6_ADDRSTRLEN);
    }
    return dg_strdup(host);
}


static u_int16_t
get_port(int af, const struct sockaddr *addr)
{
    in_port_t port = 0;
    if (af == AF_INET6) {
        struct sockaddr_in6 *a = (struct sockaddr_in6*) addr;
        port = a->sin6_port;
    }
    else {
        struct sockaddr_in *a = (struct sockaddr_in*) addr;
        port = a->sin_port;
    }
    return ntohs(port);
}


static void
write_response(int fd, const char *resp)
{
    uint8_t c = 0;
    for (size_t i = 0; resp[i] != 0; i++)
        c += resp[i];

    dg_debug_printf("$> command: %s\n", resp);

    char *r = dg_strdup_printf("$%s#%02x", resp, c);
    size_t len = strlen(r);

    int n = 0;
    while (n < len)
        n += write(fd, r + n, len - n);

    free(r);
}


static bool
wait(dg_debugwire_t *dw, int fd, dg_error_t **err)
{
    if (dw == NULL || err == NULL || *err != NULL)
        return false;

    fd_set fds;
    FD_ZERO(&fds);

    int nfds = 0;
    if (dw->fd >= 0) {
        FD_SET(dw->fd, &fds);
        nfds = dw->fd;
    }
    if (fd >= 0) {
        FD_SET(fd, &fds);
        if (fd > nfds)
            nfds = fd;
    }

    int rv = select(nfds + 1, &fds, NULL, NULL, NULL);
    if (rv == -1) {
        *err = dg_error_new_errno(DG_ERROR_GDBSERVER, errno, "Failed select");
        return false;
    }
    if (rv == 0) {
        *err = dg_error_new(DG_ERROR_GDBSERVER, "Failed select, no data");
        return false;
    }
    if (FD_ISSET(dw->fd, &fds) && dw->hw_breakpoint_set) {
        uint8_t b = dg_serial_recv_break(dw->fd, err);
        if (*err != NULL)
            return false;

        if (b != 0x55) {
            *err = dg_error_new_printf(DG_ERROR_GDBSERVER,
                "Bad break received from MCU. Expected 0x55, got 0x%02x", b);
            return false;
        }
    }

    return true;
}


static int
handle_command(dg_debugwire_t *dw, int fd, const char *cmd, dg_error_t **err)
{
    size_t len = strlen(cmd);
    if (len == 0) {
        *err = dg_error_new(DG_ERROR_GDBSERVER, "Empty command");
        return 1;
    }

    bool add = false;

    switch (cmd[0]) {
        case 0x03:
            {
                uint8_t b = dg_serial_send_break(dw->fd, err);
                if (*err != NULL)
                    return 1;

                if (b != 0x55) {
                    *err = dg_error_new_printf(DG_ERROR_GDBSERVER,
                        "Bad break response from MCU. Expected 0x55, got 0x%02x", b);
                    return 1;
                }
            }
            return 0;

        case 'q':
            if (0 == strcmp(cmd, "qAttached")) {
                write_response(fd, "1");
                return 0;
            }
            break;

        case 'g':
            {
                uint8_t buf[39] = {0};

                uint16_t pc = dg_debugwire_get_pc(dw, err);
                if (*err != NULL)
                    return 1;

                buf[35] = pc;
                buf[36] = pc >> 8;

                if (!dg_debugwire_read_registers(dw, 0, buf, 32, err) || *err != NULL)
                    return 1;

                // sreg
                if (!dg_debugwire_read_sram(dw, 0x5f, buf + 32, 1, err) || *err != NULL)
                    return 1;

                // spl sph
                if (!dg_debugwire_read_sram(dw, 0x5d, buf + 33, 2, err) || *err != NULL)
                    return 1;

                if (!dg_debugwire_write_registers(dw, 28, buf + 28, 4, err) || *err != NULL)
                    return 1;

                if (!dg_debugwire_set_pc(dw, pc, err) || *err != NULL)
                    return 1;

                dg_string_t *s = dg_string_new();
                for (size_t i = 0; i < 39; i++)
                    dg_string_append_printf(s, "%02x", buf[i]);
                write_response(fd, s->str);
                dg_string_free(s, true);

                return 0;
            }
            break;

        case 'm':
            {
                char **pieces = dg_str_split(cmd + 1, ',', 2);
                if (2 != dg_strv_length(pieces)) {
                    *err = dg_error_new_printf(DG_ERROR_GDBSERVER,
                        "Malformed memory read request: %s", cmd);
                    dg_strv_free(pieces);
                    return 1;
                }

                uint32_t addr = strtoul(pieces[0], NULL, 16);
                uint16_t count = strtoul(pieces[1], NULL, 16);
                dg_strv_free(pieces);

                if (!dg_debugwire_cache_pc(dw, err) || *err != NULL)
                    return 1;

                if (!dg_debugwire_cache_yz(dw, err) || *err != NULL)
                    return 1;

                uint8_t buf[count];
                if (addr < 0x800000) {
                    if (!dg_debugwire_read_flash(dw, (uint16_t) addr, buf, count, err) || *err != NULL)
                        return 1;
                }
                else if (addr < 0x810000) {
                    if (!dg_debugwire_read_sram(dw, (uint16_t) addr, buf, count, err) || *err != NULL)
                        return 1;
                }
                else {
                    write_response(fd, "E01");
                    return 1;
                }

                if (!dg_debugwire_restore_yz(dw, err) || *err != NULL)
                    return 1;

                if (!dg_debugwire_restore_pc(dw, err) || *err != NULL)
                    return 1;

                dg_string_t *s = dg_string_new();
                for (size_t i = 0; i < count; i++)
                    dg_string_append_printf(s, "%02x", buf[i]);
                write_response(fd, s->str);
                dg_string_free(s, true);

                return 0;
            }
            break;

        case 's':
            if (!dg_debugwire_step(dw, err) || *err != NULL)
                return 1;
            write_response(fd, "S00");
            return 0;

        case 'c':
            if (!dg_debugwire_continue(dw, err) || *err != NULL)
                return 1;
            if (!wait(dw, fd, err) || *err != NULL)
                return 1;
            write_response(fd, "S00");
            return 0;

        case 'Z':
            add = true;
        case 'z':
            {
                char **pieces = dg_str_split(cmd + 1, ',', 0);
                if (3 > dg_strv_length(pieces)) {
                    *err = dg_error_new_printf(DG_ERROR_GDBSERVER,
                        "Malformed breakpoint request: %s", cmd);
                    dg_strv_free(pieces);
                    return 1;
                }

                switch (pieces[0][0]) {
                    case '1':
                        if (add) {
                            if (dw->hw_breakpoint_set) {
                                write_response(fd, "E01");
                                dg_strv_free(pieces);
                                return 0;
                            }
                            dw->hw_breakpoint = strtoul(pieces[1], NULL, 16) / strtoul(pieces[2], NULL, 16);
                            dw->hw_breakpoint_set = true;
                        }
                        else {
                            dw->hw_breakpoint = 0;
                            dw->hw_breakpoint_set = false;
                        }
                        write_response(fd, "OK");
                        dg_strv_free(pieces);
                        return 0;
                    default:
                        write_response(fd, "E01");
                        dg_strv_free(pieces);
                        return 1;
                }

                dg_strv_free(pieces);
            }
            break;

        case '?':
            write_response(fd, "S00");
            return 0;
    }

    write_response(fd, "");
    return 0;
}


typedef enum {
    COMMAND_ACK = 1,
    COMMAND_START,
    COMMAND,
    COMMAND_CHECKSUM1,
    COMMAND_CHECKSUM2,
} command_state_t;


static int
handle_client(dg_debugwire_t *dw, int fd, dg_error_t **err)
{
    command_state_t s = COMMAND_ACK;
    dg_string_t *cmd = NULL;
    char b;
    uint8_t c, cd;
    char d[3];
    d[2] = 0;

    while (true) {
        if (1 != read(fd, &b, 1)) {
            *err = dg_error_new_errno(DG_ERROR_GDBSERVER, errno,
                "Failed to read from client socket");
            return 1;
        }

        if (b == 0x03) {
            dg_debug_printf("$< ctrl-c\n");
            char cmd[2] = {0x03, 0};
            int rv = handle_command(dw, fd, cmd, err);
            if (rv != 0 || *err != NULL)
                return rv;
            continue;
        }

        switch (s) {
            case COMMAND_ACK:
                if (b == '+') {
                    dg_debug_printf("$< ack\n");
                    break;
                }
                if (b == '-') {
                    dg_debug_printf("$< nack\n");
                    *err = dg_error_new(DG_ERROR_GDBSERVER,
                        "GDB requested retransmission");  // FIXME: retransmit
                    return 1;
                }
                if (b == '$') {
                    s = COMMAND_START;
                    break;
                }
                *err = dg_error_new_printf(DG_ERROR_GDBSERVER,
                    "ACK failed, expected '+', got '%c'", b);
                return 1;

            case COMMAND_START:
                dg_string_free(cmd, true);
                cmd = dg_string_new();
                dg_string_append_c(cmd, b);
                c = b;
                s = COMMAND;
                break;

            case COMMAND:
                if (b == '#') {
                    s = COMMAND_CHECKSUM1;
                    break;
                }
                dg_string_append_c(cmd, b);
                c += b;
                break;

            case COMMAND_CHECKSUM1:
                s = COMMAND_CHECKSUM2;
                d[0] = b;
                break;

            case COMMAND_CHECKSUM2:
                s = COMMAND_ACK;
                d[1] = b;
                cd = strtoul(d, NULL, 16);
                if (c != cd) {
                    *err = dg_error_new_printf(DG_ERROR_GDBSERVER,
                        "Bad checksum, expected '%x', got '%x'", cd, c);
                    return 1;
                }
                dg_debug_printf("$< command: %s\n", cmd->str);

                {
                    dg_debug_printf("$> ack\n");
                    c = '+';
                    if (1 != write(fd, &c, 1)) {
                        *err = dg_error_new(DG_ERROR_GDBSERVER,
                            "Failed to send ack to GDB");
                        return 1;
                    }

                    int rv = handle_command(dw, fd, cmd->str, err);
                    if (rv != 0 || *err != NULL)
                        return rv;
                }
                dg_string_free(cmd, true);
                cmd = NULL;
                break;
        }
    }

    dg_string_free(cmd, true);

    return 0;
}


int
dg_gdbserver_run(dg_debugwire_t *dw, const char *host, const char *port,
    dg_error_t **err)
{
    int rv = 0;
    struct addrinfo *result;
    struct addrinfo hints = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_flags = AI_PASSIVE,
        .ai_protocol = 0,
        .ai_canonname = NULL,
        .ai_addr = NULL,
        .ai_next = NULL,
    };
    if (0 != (rv = getaddrinfo(host, port, &hints, &result))) {
        *err = dg_error_new_printf(DG_ERROR_GDBSERVER,
            "Failed to get host:port info: %s", gai_strerror(rv));
        return rv;
    }

    int server_socket = 0;

    int ai_family = 0;
    char *final_host = NULL;
    uint16_t final_port = 0;

    for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
        final_host = get_ip(rp->ai_family, rp->ai_addr);
        final_port = get_port(rp->ai_family, rp->ai_addr);
        server_socket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (server_socket == -1) {
            if (rp->ai_next == NULL) {
                *err = dg_error_new_errno_printf(DG_ERROR_GDBSERVER, errno,
                    "Failed to open server socket (%s:%d)", final_host, final_port);
                rv = 1;
                goto cleanup0;
            }
            continue;
        }
        int value = 1;
        if (0 > setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &value,
            sizeof(int)))
        {
            if (rp->ai_next == NULL) {
                *err = dg_error_new_errno_printf(DG_ERROR_GDBSERVER, errno,
                    "Failed to set socket option (%s:%d)", final_host, final_port);
                rv = 1;
                goto cleanup;
            }
            close(server_socket);
            continue;
        }
        if (0 == bind(server_socket, rp->ai_addr, rp->ai_addrlen)) {
            ai_family = rp->ai_family;
            break;
        }
        else {
            if (rp->ai_next == NULL) {
                *err = dg_error_new_errno_printf(DG_ERROR_GDBSERVER, errno,
                    "Failed to bind to server socket (%s:%d)",
                    final_host, final_port);
                rv = 1;
                goto cleanup;
            }
        }
        free(final_host);
        close(server_socket);
    }

    // we can only accept the first connection, no parallel debugging allowed
    if (-1 == listen(server_socket, 0)) {
        *err = dg_error_new_errno_printf(DG_ERROR_GDBSERVER, errno,
            "Failed to listen to server socket (%s:%d)", final_host, final_port);
        rv = 1;
        goto cleanup;
    }

    fprintf(stderr, " * GDB server running on ");
    if (ai_family == AF_INET6)
        fprintf(stderr, "[%s]", final_host);
    else
        fprintf(stderr, "%s", final_host);
    fprintf(stderr, ":%d\n", final_port);

    size_t current_thread = 0;

    struct sockaddr_in6 addr6;
    struct sockaddr_in addr;

    socklen_t addrlen;
    struct sockaddr *client_addr = NULL;

    if (ai_family == AF_INET6) {
        addrlen = sizeof(addr6);
        client_addr = (struct sockaddr*) &addr6;
    }
    else {
        addrlen = sizeof(addr);
        client_addr = (struct sockaddr*) &addr;
    }

    int client_socket = accept(server_socket, client_addr, &addrlen);
    if (client_socket == -1) {
        *err = dg_error_new_errno_printf(DG_ERROR_GDBSERVER, errno,
            "Failed to accept connection");
        rv = 1;
        goto cleanup;
    }

    char *ip = get_ip(ai_family, client_addr);
    fprintf(stderr, " * Connection accepted from %s\n", ip);
    free(ip);

    if (dg_debugwire_reset(dw, err) && *err == NULL)
        rv = handle_client(dw, client_socket, err);

    close(client_socket);
    fprintf(stderr, " * Connection closed\n");

cleanup:
    close(server_socket);

cleanup0:
    free(final_host);
    freeaddrinfo(result);

    return rv;
}
