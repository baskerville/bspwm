#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_ewmh.h>
#include "helpers.h"
#include "types.h"
#include "settings.h"
#include "messages.h"
#include "events.h"
#include "common.h"
#include "utils.h"
#include "bspwm.h"
#include "ewmh.h"
 
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
void get_atoms(char **names, xcb_atom_t *atoms, int count)
{
    int i;
    xcb_intern_atom_cookie_t cookies[count];
    xcb_intern_atom_reply_t  *reply;

    for (i = 0; i < count; i++)
        cookies[i] = xcb_intern_atom(dpy, 0, strlen(names[i]), names[i]);
    for (i = 0; i < count; i++) {
        reply = xcb_intern_atom_reply(dpy, cookies[i], NULL);
        if (reply) {
            /* PRINTF("%s : %d\n", names[i], reply->atom); */
            atoms[i] = reply->atom;
            free(reply);
        } else {
            PUTS("warning: failed to register atoms");
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
    sig = sig; /* to prevent an "ununsed parameter" warning */
    if (signal(SIGCHLD, sigchld) == SIG_ERR)
        die("cannot install SIGCHLD handler\n");
    while (0 < waitpid(-1, NULL, WNOHANG))
        ;
}

void setup(int default_screen)
{
    sigchld(0);
    ewmh_init();
    screen = ewmh.screens[default_screen];
    /* screen = screen_of_display(dpy, default_screen); */
    if (!screen)
        die("error: cannot aquire screen\n");
    screen_width = screen->width_in_pixels;
    screen_height = screen->height_in_pixels;

    char *WM_ATOM_NAME[] = { "WM_PROTOCOLS", "WM_DELETE_WINDOW" };
    char *NET_ATOM_NAME[] = { "_NET_SUPPORTED", "_NET_WM_STATE_FULLSCREEN", "_NET_WM_STATE", "_NET_ACTIVE_WINDOW" };

    /* set up atoms for dialog/notification windows */
    get_atoms(WM_ATOM_NAME, wmatoms, WM_COUNT);
    get_atoms(NET_ATOM_NAME, netatoms, NET_COUNT);

    xcb_change_property(dpy, XCB_PROP_MODE_REPLACE, screen->root, netatoms[NET_SUPPORTED], XCB_ATOM_ATOM, 32, NET_COUNT, netatoms);
}

int main(void)
{
    fd_set descriptors;
    int sock_fd, ret_fd, xfd, sel, nbr;
    struct sockaddr_un sock_address;
    char *sock_path;
    char msg[BUFSIZ], rsp[BUFSIZ];

    xcb_generic_event_t *event;

    running = true;

    dpy = xcb_connect(NULL, &default_screen);

    if (xcb_connection_has_error(dpy))
        die("error: cannot open display\n");

    setup(default_screen);

    /* if (register_events() == 1) { */
    /*     xcb_disconnect(dpy); */
    /*     die("another WM is already running\n"); */
    /* } */

    xfd = xcb_get_file_descriptor(dpy);

    sock_path = getenv(SOCK_PATH);

    if (sock_path == NULL)
        die("BSPWM_SOCKET environment variable is not set\n");

    sock_address.sun_family = AF_UNIX;
    strcpy(sock_address.sun_path, sock_path);
    unlink(sock_path);

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sock_fd == -1) 
        die("error: could not create socket\n");

    bind(sock_fd, (struct sockaddr *) &sock_address, sizeof(sock_address));
    listen(sock_fd, SOMAXCONN);

    sel = MAX(sock_fd, xfd) + 1;

    load_settings();

    while (running) {

        FD_ZERO(&descriptors);
        FD_SET(sock_fd, &descriptors);
        FD_SET(xfd, &descriptors);

        xcb_flush(dpy);

        if (select(sel, &descriptors, NULL, NULL, NULL)) {

            if (FD_ISSET(xfd, &descriptors)) {
                while ((event = xcb_poll_for_event(dpy)) != NULL) {
                    handle_event(event);
                    free(event);
                }
            }

            if (FD_ISSET(sock_fd, &descriptors)) {
                ret_fd = accept(sock_fd, NULL, 0);
                if (ret_fd > 0 && (nbr = recv(ret_fd, msg, sizeof(msg), 0)) > 0) {
                    msg[nbr] = '\0';
                    strcpy(rsp, EMPTY_RESPONSE);
                    process_message(msg, rsp);
                    send(ret_fd, rsp, strlen(rsp), 0);
                    close(ret_fd);
                }
            }
        }
    }

    close(sock_fd);
    xcb_disconnect(dpy);
    return 0;
}
