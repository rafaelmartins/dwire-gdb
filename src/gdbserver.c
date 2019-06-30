/*
 * dwire-gdb: A GDB server for AVR 8 bit microcontrollers, using debugWire
 *            protocol through USB-to-TTL adapters.
 * Copyright (C) 2019 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 *
 * This program can be distributed under the terms of the BSD License.
 * See the file LICENSE.
 */

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

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


int
dg_gdbserver_run(const char *host, const char *port, dg_error_t **err)
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

    rv = dg_gdbserver_handle_client(client_socket);

    fprintf(stderr, " * Connection closed\n");
    close(client_socket);

cleanup:
    close(server_socket);

cleanup0:
    free(final_host);
    freeaddrinfo(result);

    return rv;
}


int
dg_gdbserver_handle_client(int fd)
{
    return 0;
}
