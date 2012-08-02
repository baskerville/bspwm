#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCK_PATH  "BSPWM_SOCKET"

int main(int argc, char *argv[])
{
    int sock_fd, i;
    struct sockaddr_un sock_address;
    char *sock_path;
    char response[BUFSIZ];
    int num_args = argc - 1;
    char **args = (argv + 1);

    if (num_args < 1)
        return;

    sock_path = getenv(SOCK_PATH);

    if (sock_path == NULL)
        return;

    sock_address.sun_family = AF_UNIX;
    strcpy(sock_address.sun_path, sock_path);

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    connect(sock_fd, (struct sockaddr *) &sock_address, sizeof(sock_address));

    for (i = 0; i < num_args; i++) {
        send(sock_fd, args[i], strlen(args[i]), 0);
        
        if (recv(sock_fd, response, sizeof(response), 0) > 0)
            printf("%s\n", response);
    }
                
}
