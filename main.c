#include <sys/select.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "utils.h"
#include "main.h"
#include "types.h"
#include "settings.h"
#include "messages.h"
#include "events.h"
 
void quit(void)
{
    running = false;
}

// check for other WM and initiate events capture
int register_events(void)
{
    xcb_generic_error_t *error;
    unsigned int values[] = {XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_BUTTON_PRESS};
    error = xcb_request_check(dpy, xcb_change_window_attributes_checked(dpy, screen->root, XCB_CW_EVENT_MASK, values));
    xcb_flush(dpy);
    if (error) return 1;
    return 0;
}
 
/* wrapper to get atoms using xcb */
void get_atoms(char **names, xcb_atom_t *atoms, unsigned int count)
{
    xcb_intern_atom_cookie_t cookies[count];
    xcb_intern_atom_reply_t  *reply;

    for (unsigned int i = 0; i < count; i++)
        cookies[i] = xcb_intern_atom(dpy, 0, strlen(names[i]), names[i]);
    for (unsigned int i = 0; i < count; i++) {
        reply = xcb_intern_atom_reply(dpy, cookies[i], NULL);
        if (reply) {
            /* PRINTF("%s : %d\n", names[i], reply->atom); */
            atoms[i] = reply->atom; free(reply);
        } else {
            PUTS("warning: monsterwm failed to register atoms");
        }
    }
}

xcb_screen_t *screen_of_display(xcb_connection_t *dpy, int default_screen)
{
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(dpy));
    for (; iter.rem; --default_screen, xcb_screen_next(&iter))
        if (default_screen == 0) return iter.data;
    return NULL;
}

void sigchld(int sig)
{
    if (signal(SIGCHLD, sigchld) == SIG_ERR)
        die("cannot install SIGCHLD handler\n");
    while (0 < waitpid(-1, NULL, WNOHANG))
        ;
}

void setup(int default_screen)
{
    sigchld(0);
    screen = screen_of_display(dpy, default_screen);
    if (!screen)
        die("error: cannot aquire screen\n");
    screen_width = screen->width_in_pixels;
    screen_height = screen->height_in_pixels;

    /* set up atoms for dialog/notification windows */
    get_atoms(WM_ATOM_NAME, wmatoms, WM_COUNT);
    get_atoms(NET_ATOM_NAME, netatoms, NET_COUNT);

    xcb_change_property(dpy, XCB_PROP_MODE_REPLACE, screen->root, netatoms[NET_SUPPORTED], XCB_ATOM_ATOM, 32, NET_COUNT, netatoms);
}

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

    setup(default_screen);

    if (register_events() == 1) {
        xcb_disconnect(dpy);
        die("another WM is already running\n");
    }

    xfd = xcb_get_file_descriptor(dpy);

    /* O_RDWR is needed, see http://bit.ly/T0C5Mh */
    fifo = open(INPUT_FIFO, O_RDWR | O_NONBLOCK);

    if (fifo == -1) 
        die("error: could not open fifo\n");

    sel = MAX(fifo, xfd) + 1;

    load_settings();

    while (running) {

        FD_ZERO(&descriptors);
        FD_SET(fifo, &descriptors);
        FD_SET(xfd, &descriptors);

        xcb_flush(dpy);

        if (select(sel, &descriptors, NULL, NULL, NULL)) {

            if (FD_ISSET(xfd, &descriptors)) {
                while ((event = xcb_poll_for_event(dpy)) != NULL) {
                    handle_event(event);
                    free(event);
                }
            }

            if (FD_ISSET(fifo, &descriptors)) {
                if ((nbr = read(fifo, msg, sizeof(msg))) > 0) {
                    msg[nbr] = '\0';
                    process_message(msg);
                }
            }

        }

    }

    close(fifo);
    xcb_disconnect(dpy);
    return 0;
}
