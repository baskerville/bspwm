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
#include "events.h"

int main(void)
{
    fd_set descriptors;
    int fifo, xfd, sel, nbr;
    char msg[BUFSIZ];
    xcb_generic_event_t *event;

    running = true;

    dpy = xcb_connect(NULL, &default_screen);

    if (xcb_connection_has_error(dpy))
        die("error: cannot open display\n");

    xfd = xcb_get_file_descriptor(dpy);

    /* O_RDWR is needed, see http://bit.ly/T0C5Mh */
    fifo = open(INPUT_FIFO, O_RDWR | O_NONBLOCK);

    if (fifo == -1) 
        die("error: could not open fifo\n");

    FD_ZERO(&descriptors);
    FD_SET(fifo, &descriptors);
    FD_SET(xfd, &descriptors);

    /* printf("fifo: %i\n", fifo); */
    /* printf("xfd: %i\n", xfd); */

    sel = MAX(fifo, xfd) + 1;

    load_settings();

    while (running) {
        if (select(sel, &descriptors, NULL, NULL, NULL)) {

            printf("in select\n");

            if (FD_ISSET(fifo, &descriptors)) {
                if ((nbr = read(fifo, msg, sizeof(msg))) > 0) {
                    msg[nbr] = '\0';
                    process_message(msg);
                }
            }

            if (FD_ISSET(xfd, &descriptors)) {
                while ((event = xcb_poll_for_event(dpy)) != NULL)
                    handle_event(event);
            }
        }

        FD_ZERO(&descriptors);
        FD_SET(fifo, &descriptors);
        FD_SET(xfd, &descriptors);
    }

    close(fifo);
    xcb_disconnect(dpy);
    return 0;
}
