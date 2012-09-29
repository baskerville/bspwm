#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "common.h"

int main(int argc, char *argv[])
{
    int sock_fd, nbr, i;
    struct sockaddr_un sock_address;
    char socket_path[BUFSIZ];
    char msg[BUFSIZ];
    char rsp[BUFSIZ];

    if (argc < 2)
        return -1;

    char *sp = getenv(SOCKET_ENV_VAR);

    strncpy(socket_path, (sp == NULL ? DEFAULT_SOCKET_PATH : sp), sizeof(socket_path));

    msg[0] = '\0';

    int max = sizeof(msg);
    for (i = 1; max > 0 && i < argc; i++) {
        strncat(msg, argv[i], max);
        max -= strlen(argv[i]);
        if (i < (argc - 1)) {
            strncat(msg, TOKEN_SEP, max);
            max -= strlen(TOKEN_SEP);
        }
    }

    sock_address.sun_family = AF_UNIX;
    strncpy(sock_address.sun_path, socket_path, sizeof(sock_address.sun_path));

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    connect(sock_fd, (struct sockaddr *) &sock_address, sizeof(sock_address));

    send(sock_fd, msg, strlen(msg), 0);

    if ((nbr = recv(sock_fd, rsp, sizeof(rsp), 0)) > 0) {
        rsp[nbr] = '\0';
        if (strcmp(rsp, EMPTY_RESPONSE) != 0)
            printf("%s", rsp);
    }

    close(sock_fd);
    return 0;
}
