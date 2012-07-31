#include <sys/select.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "utils.h"
#include "main.h"
#include "types.h"
#include "settings.h"
#include "messages.h"
#include "commands.h"

int main(void)
{
    fd_set descriptors;
    int fifo, xfd, nbr;
    char msg[BUFSIZ];
    struct timeval timeout;
    xcb_generic_event_t *event;

    running = true;

    /* O_RDWR is needed, see http://bit.ly/T0C5Mh */
    fifo = open(INPUT_FIFO, O_RDWR | O_NONBLOCK);

    if (fifo == -1) 
        die("error: could not open fifo\n");

    dpy = xcb_connect(NULL, &default_screen);

    if (xcb_connection_has_error(dpy))
        die("error: cannot open display\n");

    xfd = xcb_get_file_descriptor(dpy);
    timeout.tv_sec = SELECT_TIMEOUT;

    FD_ZERO(&descriptors);
    FD_SET(xfd, &descriptors);
    FD_SET(fifo, &descriptors);

    load_settings();

    while (running) {
        if (select(fifo + 1, &descriptors, 0, 0, &timeout)) {

            nbr = read(fifo, msg, sizeof(msg));
            if (nbr > 0) {
                msg[nbr] = '\0';
                process_message(msg);
            }

            event = xcb_poll_for_event(dpy);
            if (event != NULL) {
                /* handle_event(event); */
            }
        }
    }

    close(fifo);
    xcb_disconnect(dpy);
    return 0;
}
