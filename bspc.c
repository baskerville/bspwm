#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "helpers.h"
#include "common.h"

int main(int argc, char *argv[])
{
    int sock_fd;
    struct sockaddr_un sock_address;
    size_t msglen = 0;
    char msg[BUFSIZ];
    char rsp[BUFSIZ];

    if (argc < 2)
        err("No arguments given.\n");

    char *sock_path = getenv(SOCKET_ENV_VAR);
    if (sock_path == NULL || strlen(sock_path) == 0)
        warn("The environment variable '%s' is not set or empty, we will use: '%s'.\n", SOCKET_ENV_VAR, DEFAULT_SOCKET_PATH);
    else if (sizeof(sock_address.sun_path) <= strlen(sock_path))
        err("The string can't fit in the socket address: '%s'.\n", sock_path);

    sock_address.sun_family = AF_UNIX;
    strncpy(sock_address.sun_path, (sock_path == NULL ? DEFAULT_SOCKET_PATH : sock_path), sizeof(sock_address.sun_path));
    sock_address.sun_path[sizeof(sock_address.sun_path) - 1] = 0;

    for (int offset = 0, len = sizeof(msg), n = 0; --argc && ++argv && len > 0; offset += n, len -= n)
        n = snprintf(msg + offset, len, "%s ", *argv);

    msglen = strlen(msg);
    if (msg[msglen - 1] == ' ')
        msg[--msglen] = '\0';

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1)
        err("Failed to create the socket.\n");

    if (connect(sock_fd, (struct sockaddr *) &sock_address, sizeof(sock_address)) == -1)
        err("Failed to connect to the socket.\n");

    if (send(sock_fd, msg, msglen, 0) == -1)
        err("Failed to send the data.\n");

    int n = recv(sock_fd, rsp, sizeof(rsp), 0);
    if (n == -1) {
        err("Failed to get the response.\n");
    } else if (n > 0) {
        rsp[n] = '\0';
        printf("%s\n", rsp);
    }

    if (sock_fd)
        close(sock_fd);

    return EXIT_SUCCESS;
}
