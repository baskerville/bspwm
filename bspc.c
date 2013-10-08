/* * Copyright (c) 2012-2013 Bastien Dejean
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#ifdef __OpenBSD__
#include <sys/types.h>
#endif
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "helpers.h"
#include "common.h"

int main(int argc, char *argv[])
{
    int sock_fd;
    struct sockaddr_un sock_address;
    char msg[BUFSIZ];
    char rsp[BUFSIZ];

    if (argc < 2)
        err("No arguments given.\n");

    char *sp = getenv(SOCKET_ENV_VAR);

    sock_address.sun_family = AF_UNIX;
    snprintf(sock_address.sun_path, sizeof(sock_address.sun_path), "%s", (sp == NULL ? DEFAULT_SOCKET_PATH : sp));

    argc--, argv++;
    int msg_len = 0;

    for (int offset = 0, rem = sizeof(msg), n = 0; argc > 0 && rem > 0; offset += n, rem -= n, argc--, argv++) {
        n = snprintf(msg + offset, rem, "%s%c", *argv, 0);
        msg_len += n;
    }

    if ((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
        err("Failed to create the socket.\n");

    if (connect(sock_fd, (struct sockaddr *) &sock_address, sizeof(sock_address)) == -1)
        err("Failed to connect to the socket.\n");

    if (send(sock_fd, msg, msg_len, 0) == -1)
        err("Failed to send the data.\n");

    int ret = EXIT_SUCCESS;

    int n = recv(sock_fd, rsp, sizeof(rsp), 0);
    if (n == -1) {
        err("Failed to get the response.\n");
    } else if (n > 0) {
        if (n == 1 && rsp[0] == MESSAGE_FAILURE) {
            ret = EXIT_FAILURE;
        } else {
            rsp[n] = '\0';
            printf("%s\n", rsp);
        }
    }

    if (sock_fd)
        close(sock_fd);

    return ret;
}
