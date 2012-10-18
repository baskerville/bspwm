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
#include <xcb/xinerama.h>
#include "types.h"
#include "settings.h"
#include "messages.h"
#include "rules.h"
#include "events.h"
#include "common.h"
#include "helpers.h"
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
    xcb_generic_error_t *e = xcb_request_check(dpy, xcb_change_window_attributes_checked(dpy, screen->root, XCB_CW_EVENT_MASK, values));
    if (e != NULL) {
        xcb_disconnect(dpy);
        err("another WM is already running\n");
    }

    xcb_grab_button(dpy, false, screen->root, XCB_EVENT_MASK_BUTTON_PRESS, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_BUTTON_INDEX_1, BUTTON_MODIFIER);
    xcb_grab_button(dpy, false, screen->root, XCB_EVENT_MASK_BUTTON_PRESS, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_BUTTON_INDEX_2, BUTTON_MODIFIER);
    xcb_grab_button(dpy, false, screen->root, XCB_EVENT_MASK_BUTTON_PRESS, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_BUTTON_INDEX_3, BUTTON_MODIFIER);
}

void setup(void)
{
    ewmh_init();
    screen = xcb_setup_roots_iterator(xcb_get_setup(dpy)).data;
    if (!screen)
        err("error: cannot aquire screen\n");

    screen_width = screen->width_in_pixels;
    screen_height = screen->height_in_pixels;
    root_depth = screen->root_depth;

    xcb_atom_t net_atoms[] = {ewmh->_NET_SUPPORTED,
                              ewmh->_NET_DESKTOP_NAMES,
                              ewmh->_NET_NUMBER_OF_DESKTOPS,
                              ewmh->_NET_CURRENT_DESKTOP,
                              ewmh->_NET_CLIENT_LIST,
                              ewmh->_NET_ACTIVE_WINDOW,
                              ewmh->_NET_WM_DESKTOP,
                              ewmh->_NET_WM_STATE,
                              ewmh->_NET_WM_STATE_FULLSCREEN,
                              ewmh->_NET_WM_WINDOW_TYPE,
                              ewmh->_NET_WM_WINDOW_TYPE_DOCK,
                              ewmh->_NET_WM_WINDOW_TYPE_NOTIFICATION,
                              ewmh->_NET_WM_WINDOW_TYPE_DIALOG,
                              ewmh->_NET_WM_WINDOW_TYPE_UTILITY,
                              ewmh->_NET_WM_WINDOW_TYPE_TOOLBAR};

    xcb_ewmh_set_supported(ewmh, default_screen, LENGTH(net_atoms), net_atoms);

    monitor_uid = desktop_uid = 0;
    mon = last_mon = mon_head = mon_tail = NULL;

    bool xinerama_is_active = false;

    if (xcb_get_extension_data(dpy, &xcb_xinerama_id)->present) {
        xcb_xinerama_is_active_reply_t *xia = xcb_xinerama_is_active_reply(dpy, xcb_xinerama_is_active(dpy), NULL);
        if (xia != NULL) {
            xinerama_is_active = xia->state;
            free(xia);
        }
    }

    if (xinerama_is_active) {
        xcb_xinerama_query_screens_reply_t *xsq = xcb_xinerama_query_screens_reply(dpy, xcb_xinerama_query_screens(dpy), NULL);
        xcb_xinerama_screen_info_t *xsi = xcb_xinerama_query_screens_screen_info(xsq);
        int n = xcb_xinerama_query_screens_screen_info_length(xsq);
        PRINTF("number of monitors: %d\n", n);
        for (int i = 0; i < n; i++) {
            xcb_xinerama_screen_info_t info = xsi[i];
            xcb_rectangle_t rect = (xcb_rectangle_t) {info.x_org, info.y_org, info.width, info.height};
            add_monitor(&rect);
        }
        free(xsq);
    } else {
        warn("Xinerama is inactive");
        xcb_rectangle_t rect = (xcb_rectangle_t) {0, 0, screen_width, screen_height};
        add_monitor(&rect);
    }

    for (monitor_t *m = mon_head; m != NULL; m = m->next)
        add_desktop(m, NULL);

    ewmh_update_number_of_desktops();
    ewmh_update_desktop_names();
    rule_head = make_rule();
    frozen_pointer = make_pointer_state();
    split_mode = MODE_AUTOMATIC;
}

int main(void)
{
    fd_set descriptors;
    char socket_path[MAXLEN];
    int sock_fd, ret_fd, dpy_fd, sel, n;
    struct sockaddr_un sock_address;
    size_t rsplen = 0;
    char msg[BUFSIZ] = {0};
    char rsp[BUFSIZ] = {0};

    xcb_generic_event_t *event;

    running = true;

    dpy = xcb_connect(NULL, &default_screen);

    if (xcb_connection_has_error(dpy))
        err("error: cannot open display\n");

    setup();
    register_events();

    dpy_fd = xcb_get_file_descriptor(dpy);

    char *sp = getenv(SOCKET_ENV_VAR);

    strncpy(socket_path, (sp == NULL ? DEFAULT_SOCKET_PATH : sp), sizeof(socket_path));

    sock_address.sun_family = AF_UNIX;
    strncpy(sock_address.sun_path, socket_path, sizeof(sock_address.sun_path));
    unlink(socket_path);

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sock_fd == -1)
        err("error: could not create socket\n");

    bind(sock_fd, (struct sockaddr *) &sock_address, sizeof(sock_address));
    listen(sock_fd, SOMAXCONN);

    sel = MAX(sock_fd, dpy_fd) + 1;

    load_settings();
    run_autostart();
    ewmh_update_wm_name();

    while (running) {

        xcb_flush(dpy);

        FD_ZERO(&descriptors);
        FD_SET(sock_fd, &descriptors);
        FD_SET(dpy_fd, &descriptors);

        if (select(sel, &descriptors, NULL, NULL, NULL)) {

            if (FD_ISSET(sock_fd, &descriptors)) {
                ret_fd = accept(sock_fd, NULL, 0);
                if (ret_fd > 0 && (n = recv(ret_fd, msg, sizeof(msg), 0)) > 0) {
                    msg[n] = '\0';
                    process_message(msg, rsp);
                    rsplen = strlen(rsp);
                    if (rsp[rsplen - 1] == '\n')
                        rsp[--rsplen] = '\0';
                    send(ret_fd, rsp, rsplen, 0);
                    close(ret_fd);
                    rsp[0] = '\0';
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
            err("connection has errors\n");
        }
    }

    close(sock_fd);
    xcb_ewmh_connection_wipe(ewmh);
    free(ewmh);
    xcb_flush(dpy);
    xcb_disconnect(dpy);
    return 0;
}
