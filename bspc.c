#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "common.h"

static void logmsg(FILE *stream, char *fmt, va_list ap) {
    vfprintf(stream, fmt, ap);
}

static void warn(char *warnfmt, ...) {
    va_list ap;
    va_start(ap, warnfmt);
    logmsg(stderr, warnfmt, ap);
    va_end(ap);
}

__attribute__((noreturn))
static void err(char *errfmt, ...) {
    va_list ap;
    va_start(ap, errfmt);
    logmsg(stderr, errfmt, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    int sock_fd;
    struct sockaddr_un sock_address;
    char msg[BUFSIZ] = {0};
    char rsp[BUFSIZ] = {0};

    if (argc < 2)
        err("invalid number of arguments: %d\n", argc);

    char *sock_path = getenv(SOCKET_ENV_VAR);
    if (sock_path == NULL || strlen(sock_path) == 0)
        warn("environmental variable '%s' is not set or empty - using default value: %s\n", SOCKET_ENV_VAR, DEFAULT_SOCKET_PATH);
    else if (sizeof(sock_address.sun_path) <= strlen(sock_path))
        err("value too long for environmental variable '%s'\n", SOCKET_ENV_VAR);

    sock_address.sun_family = AF_UNIX;
    strncpy(sock_address.sun_path, (sock_path == NULL ? DEFAULT_SOCKET_PATH: sock_path), sizeof(sock_address.sun_path));
    sock_address.sun_path[sizeof(sock_address.sun_path) - 1] = 0;

    for (int offset = 0, len = BUFSIZ, n = 0; --argc && ++argv && len > 0; offset += n, len -= n)
        n = snprintf(msg + offset, len, "%s ", *argv);

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1)
        err("failed to create socket\n");

    if (connect(sock_fd, (struct sockaddr *) &sock_address, sizeof(sock_address)) == -1)
        err("failed to connect to socket\n");

    if (send(sock_fd, msg, strlen(msg), 0) == -1)
        err("failed to send data\n");

    int n = recv(sock_fd, rsp, sizeof(rsp), 0);
    if (n == -1)
        err("failed to get response\n");
    else if (n > 0) {
        rsp[n] = '\0';
        printf("%s\n", rsp);
    }

    if (sock_fd)
        close(sock_fd);

    return EXIT_SUCCESS;
}
