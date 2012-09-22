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
#include "rules.h"
#include "events.h"
#include "common.h"
#include "utils.h"
#include "bspwm.h"
#include "tree.h"
#include "ewmh.h"

void quit(void)
{
    running = false;
}

void register_events(void)
{
    uint32_t values[] = {ROOT_EVENT_MASK};
    xcb_generic_error_t *err = xcb_request_check(dpy, xcb_change_window_attributes_checked(dpy, screen->root, XCB_CW_EVENT_MASK, values));
    if (err != NULL) {
        xcb_disconnect(dpy);
        die("another WM is already running\n");
    }

    xcb_grab_button(dpy, false, screen->root, XCB_EVENT_MASK_BUTTON_PRESS, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_BUTTON_INDEX_1, MOUSE_MODIFIER);
    xcb_grab_button(dpy, false, screen->root, XCB_EVENT_MASK_BUTTON_PRESS, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_BUTTON_INDEX_2, MOUSE_MODIFIER);
    xcb_grab_button(dpy, false, screen->root, XCB_EVENT_MASK_BUTTON_PRESS, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_BUTTON_INDEX_3, MOUSE_MODIFIER);
}

void setup(void)
{
    ewmh_init();
    screen = xcb_setup_roots_iterator(xcb_get_setup(dpy)).data;
    if (!screen)
        die("error: cannot aquire screen\n");

    screen_width = screen->width_in_pixels;
    screen_height = screen->height_in_pixels;
    root_depth = screen->root_depth;

    xcb_atom_t net_atoms[] = {ewmh->_NET_SUPPORTED, ewmh->_NET_WM_STATE_FULLSCREEN, ewmh->_NET_WM_STATE, ewmh->_NET_ACTIVE_WINDOW};

    xcb_ewmh_set_supported(ewmh, default_screen, LENGTH(net_atoms), net_atoms);

    desk = make_desktop(DEFAULT_DESK_NAME);
    last_desk = NULL;
    desk_head = desk;
    desk_tail = desk;
    num_desktops++;

    ewmh_update_number_of_desktops();
    ewmh_update_desktop_names();
    rule_head = make_rule();
    frozen_pointer = make_pointer_state();
    split_mode = MODE_AUTOMATIC;
}

int main(void)
{
    fd_set descriptors;
    char *socket_path;
    int sock_fd, ret_fd, dpy_fd, sel, nbr;
    struct sockaddr_un sock_address;
    char msg[BUFSIZ], rsp[BUFSIZ];

    xcb_generic_event_t *event;

    running = true;

    dpy = xcb_connect(NULL, &default_screen);

    if (xcb_connection_has_error(dpy))
        die("error: cannot open display\n");

    setup();
    register_events();

    dpy_fd = xcb_get_file_descriptor(dpy);

    socket_path = getenv(SOCKET_ENV_VAR);

    if (socket_path == NULL) {
        xcb_disconnect(dpy);
        die("the socket path environment variable is not defined\n");
    }

    sock_address.sun_family = AF_UNIX;
    strcpy(sock_address.sun_path, socket_path);
    unlink(socket_path);

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sock_fd == -1) 
        die("error: could not create socket\n");

    bind(sock_fd, (struct sockaddr *) &sock_address, sizeof(sock_address));
    listen(sock_fd, SOMAXCONN);

    sel = MAX(sock_fd, dpy_fd) + 1;

    load_settings();
    run_autostart();
    ewmh_update_wm_name();
    update_root_dimensions();

    while (running) {

        xcb_flush(dpy);

        FD_ZERO(&descriptors);
        FD_SET(sock_fd, &descriptors);
        FD_SET(dpy_fd, &descriptors);

        if (select(sel, &descriptors, NULL, NULL, NULL)) {

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

            if (FD_ISSET(dpy_fd, &descriptors)) {
                while ((event = xcb_poll_for_event(dpy)) != NULL) {
                    handle_event(event);
                    free(event);
                }
            }

        }

        if (xcb_connection_has_error(dpy)) {
            die("connection has errors\n");
        }
    }

    close(sock_fd);
    xcb_ewmh_connection_wipe(ewmh);
    free(ewmh);
    xcb_flush(dpy);
    xcb_disconnect(dpy);
    return 0;
}
