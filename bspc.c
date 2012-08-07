#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "common.h"

int main(int argc, char *argv[])
{
    int sock_fd, nbr;
    struct sockaddr_un sock_address;
    char *sock_path;
    char rsp[BUFSIZ];
    char *msg;

    if (argc < 2)
        return -1;

    sock_path = getenv(SOCK_PATH);

    if (sock_path == NULL)
        return -1;

    msg = *(argv + 1);
    sock_address.sun_family = AF_UNIX;
    strcpy(sock_address.sun_path, sock_path);

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    connect(sock_fd, (struct sockaddr *) &sock_address, sizeof(sock_address));

    send(sock_fd, msg, strlen(msg), 0);
    if ((nbr = recv(sock_fd, rsp, sizeof(rsp), 0)) > 0) {
        rsp[nbr] = '\0';
        if (strcmp(rsp, EMPTY_RESPONSE) != 0)
            printf("%s", rsp);
    }

    return 0;
}
