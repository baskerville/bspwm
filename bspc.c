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
    char msg[BUFSIZ];
    char rsp[BUFSIZ];

    if (argc < 2)
        err("No arguments given.\n");

    char *sock_path = getenv(SOCKET_ENV_VAR);
    if (sock_path != NULL && sizeof(sock_address.sun_path) <= strlen(sock_path))
        err("The socket path can't fit into the socket address.\n");

    sock_address.sun_family = AF_UNIX;
    strncpy(sock_address.sun_path, (sock_path == NULL ? DEFAULT_SOCKET_PATH : sock_path), sizeof(sock_address.sun_path));
    sock_address.sun_path[sizeof(sock_address.sun_path) - 1] = 0;

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
