#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/randr.h>
#include "types.h"
#include "settings.h"
#include "messages.h"
#include "rules.h"
#include "events.h"
#include "common.h"
#include "helpers.h"
#include "window.h"
#include "bspwm.h"
#include "tree.h"
#include "ewmh.h"

int main(int argc, char *argv[])
{
    fd_set descriptors;
    char socket_path[MAXLEN];
    char *fifo_path = NULL;
    config_path[0] = '\0';
    status_prefix = NULL;
    int sock_fd, ret_fd, dpy_fd, sel, n;
    struct sockaddr_un sock_address;
    size_t rsp_len = 0;
    char msg[BUFSIZ] = {0};
    char rsp[BUFSIZ] = {0};
    xcb_generic_event_t *event;
    char opt;

    while ((opt = getopt(argc, argv, "hvc:s:p:")) != -1) {
        switch (opt) {
            case 'h':
                printf(WM_NAME " [-h|-v|-c CONFIG_PATH|-s PANEL_FIFO|-p PANEL_PREFIX]\n");
                exit(EXIT_SUCCESS);
                break;
            case 'v':
                printf("%s\n", VERSION);
                exit(EXIT_SUCCESS);
                break;
            case 'c':
                strncpy(config_path, optarg, sizeof(config_path));
                break;
            case 's':
                fifo_path = optarg;
                break;
            case 'p':
                status_prefix = optarg;
                break;
        }
    }

    if (config_path[0] == '\0') {
        char *config_home = getenv(CONFIG_HOME_ENV);
        if (config_home != NULL)
            snprintf(config_path, sizeof(config_path), "%s/%s/%s", config_home, WM_NAME, CONFIG_NAME);
        else
            snprintf(config_path, sizeof(config_path), "%s/%s/%s/%s", getenv("HOME"), ".config", WM_NAME, CONFIG_NAME);
    }

    dpy = xcb_connect(NULL, &default_screen);

    if (xcb_connection_has_error(dpy))
        err("Can't open the default display.\n");

    setup();

    dpy_fd = xcb_get_file_descriptor(dpy);

    char *sp = getenv(SOCKET_ENV_VAR);
    strncpy(socket_path, (sp == NULL ? DEFAULT_SOCKET_PATH : sp), sizeof(socket_path));

    sock_address.sun_family = AF_UNIX;
    strncpy(sock_address.sun_path, socket_path, sizeof(sock_address.sun_path));
    unlink(socket_path);

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sock_fd == -1)
        err("Couldn't create the socket.\n");

    if (bind(sock_fd, (struct sockaddr *) &sock_address, sizeof(sock_address)) == -1)
        err("Couldn't bind a name to the socket.\n");

    if (listen(sock_fd, SOMAXCONN) == -1)
        err("Couldn't listen to the socket.\n");

    sel = MAX(sock_fd, dpy_fd) + 1;

    if (fifo_path != NULL) {
        int fifo_fd = open(fifo_path, O_RDWR | O_NONBLOCK);
        if (fifo_fd != -1)
            status_fifo = fdopen(fifo_fd, "w");
        else
            warn("Couldn't open status fifo.\n");
    }

    load_settings();
    run_config();
    running = true;

    while (running) {

        xcb_flush(dpy);

        FD_ZERO(&descriptors);
        FD_SET(sock_fd, &descriptors);
        FD_SET(dpy_fd, &descriptors);

        if (select(sel, &descriptors, NULL, NULL, NULL) > 0) {

            if (FD_ISSET(sock_fd, &descriptors)) {
                ret_fd = accept(sock_fd, NULL, 0);
                if (ret_fd > 0 && (n = recv(ret_fd, msg, sizeof(msg), 0)) > 0) {
                    msg[n] = '\0';
                    if (handle_message(msg, n, rsp)) {
                        rsp_len = strlen(rsp);
                        if (rsp[rsp_len - 1] == '\n')
                            rsp[--rsp_len] = '\0';
                    } else {
                        rsp[0] = MESSAGE_FAILURE;
                        rsp_len = 1;
                    }
                    send(ret_fd, rsp, rsp_len, 0);
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

        if (xcb_connection_has_error(dpy))
            err("The server has closed the connection.\n");
    }

    cleanup();
    close(sock_fd);
    if (status_fifo != NULL)
        fclose(status_fifo);
    xcb_ewmh_connection_wipe(ewmh);
    xcb_destroy_window(dpy, motion_recorder);
    free(ewmh);
    xcb_flush(dpy);
    xcb_disconnect(dpy);
    return exit_status;
}

void init(void)
{
    num_monitors = num_desktops = num_clients = 0;
    monitor_uid = desktop_uid = rule_uid = 0;
    mon = last_mon = mon_head = mon_tail = pri_mon = NULL;
    rule_head = rule_tail = NULL;
    status_fifo = NULL;
    last_motion_time = last_motion_x = last_motion_y = 0;
    randr_base = 0;
    visible = auto_raise = true;
    exit_status = 0;
}

void setup(void)
{
    init();
    ewmh_init();
    screen = xcb_setup_roots_iterator(xcb_get_setup(dpy)).data;
    if (screen == NULL)
        err("Can't acquire the default screen.\n");
    root = screen->root;
    register_events();

    screen_width = screen->width_in_pixels;
    screen_height = screen->height_in_pixels;
    root_depth = screen->root_depth;

    uint32_t mask = XCB_CW_EVENT_MASK;
    uint32_t values[] = {XCB_EVENT_MASK_POINTER_MOTION};
    motion_recorder = xcb_generate_id(dpy);
    xcb_create_window(dpy, XCB_COPY_FROM_PARENT, motion_recorder, root, 0, 0, screen_width, screen_height, 0, XCB_WINDOW_CLASS_INPUT_ONLY, XCB_COPY_FROM_PARENT, mask, values);

    xcb_atom_t net_atoms[] = {ewmh->_NET_SUPPORTED,
                              ewmh->_NET_SUPPORTING_WM_CHECK,
                              ewmh->_NET_DESKTOP_NAMES,
                              ewmh->_NET_NUMBER_OF_DESKTOPS,
                              ewmh->_NET_CURRENT_DESKTOP,
                              ewmh->_NET_CLIENT_LIST,
                              ewmh->_NET_ACTIVE_WINDOW,
                              ewmh->_NET_WM_DESKTOP,
                              ewmh->_NET_WM_STATE,
                              ewmh->_NET_WM_STATE_FULLSCREEN,
                              ewmh->_NET_WM_STATE_DEMANDS_ATTENTION,
                              ewmh->_NET_WM_WINDOW_TYPE,
                              ewmh->_NET_WM_WINDOW_TYPE_DOCK,
                              ewmh->_NET_WM_WINDOW_TYPE_DESKTOP,
                              ewmh->_NET_WM_WINDOW_TYPE_NOTIFICATION,
                              ewmh->_NET_WM_WINDOW_TYPE_DIALOG,
                              ewmh->_NET_WM_WINDOW_TYPE_UTILITY,
                              ewmh->_NET_WM_WINDOW_TYPE_TOOLBAR};

    xcb_ewmh_set_wm_name(ewmh, root, strlen(WM_NAME), WM_NAME);
    xcb_ewmh_set_supported(ewmh, default_screen, LENGTH(net_atoms), net_atoms);
    ewmh_set_supporting(motion_recorder);

#define GETATOM(a) \
    get_atom(#a, &a);
    GETATOM(WM_STATE)
    GETATOM(WM_DELETE_WINDOW)
    GETATOM(WM_TAKE_FOCUS)
    GETATOM(_BSPWM_FLOATING_WINDOW)
#undef GETATOM

    const xcb_query_extension_reply_t *qep = xcb_get_extension_data(dpy, &xcb_randr_id);
    if (qep->present && import_monitors()) {
        randr = true;
        randr_base = qep->first_event;
        xcb_randr_select_input(dpy, root, XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);
    } else {
        randr = false;
        warn("Couldn't retrieve monitors via RandR.\n");
        xcb_rectangle_t rect = (xcb_rectangle_t) {0, 0, screen_width, screen_height};
        monitor_t *m = add_monitor(rect);
        add_desktop(m, make_desktop(NULL));
    }

    ewmh_update_number_of_desktops();
    ewmh_update_desktop_names();
    ewmh_update_current_desktop();
    frozen_pointer = make_pointer_state();
    xcb_get_input_focus_reply_t *ifo = xcb_get_input_focus_reply(dpy, xcb_get_input_focus(dpy), NULL);
    if (ifo != NULL && (ifo->focus == XCB_INPUT_FOCUS_POINTER_ROOT || ifo->focus == XCB_NONE))
        clear_input_focus();
    free(ifo);
}

void register_events(void)
{
    uint32_t values[] = {ROOT_EVENT_MASK};
    xcb_generic_error_t *e = xcb_request_check(dpy, xcb_change_window_attributes_checked(dpy, root, XCB_CW_EVENT_MASK, values));
    if (e != NULL) {
        xcb_disconnect(dpy);
        err("Another window manager is already running.\n");
    }
}

bool import_monitors(void)
{
    PUTS("import monitors");
    xcb_randr_get_screen_resources_current_reply_t *sres = xcb_randr_get_screen_resources_current_reply(dpy, xcb_randr_get_screen_resources_current(dpy, root), NULL);
    if (sres == NULL)
        return false;

    int len = xcb_randr_get_screen_resources_current_outputs_length(sres);
    xcb_randr_output_t *outputs = xcb_randr_get_screen_resources_current_outputs(sres);

    xcb_randr_get_output_info_cookie_t cookies[len];
    for (int i = 0; i < len; i++)
        cookies[i] = xcb_randr_get_output_info(dpy, outputs[i], XCB_CURRENT_TIME);

    for (monitor_t *m = mon_head; m != NULL; m = m->next)
        m->wired = false;

    monitor_t *mm = NULL;
    unsigned int num = 0;

    for (int i = 0; i < len; i++) {
        xcb_randr_get_output_info_reply_t *info = xcb_randr_get_output_info_reply(dpy, cookies[i], NULL);
        if (info != NULL && info->crtc != XCB_NONE) {

            xcb_randr_get_crtc_info_reply_t *cir = xcb_randr_get_crtc_info_reply(dpy, xcb_randr_get_crtc_info(dpy, info->crtc, XCB_CURRENT_TIME), NULL);
            if (cir != NULL) {
                xcb_rectangle_t rect = (xcb_rectangle_t) {cir->x, cir->y, cir->width, cir->height};
                mm = get_monitor_by_id(outputs[i]);
                if (mm != NULL) {
                    mm->rectangle = rect;
                    for (desktop_t *d = mm->desk_head; d != NULL; d = d->next)
                        for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root))
                            fit_monitor(mm, n->client);
                    arrange(mm, mm->desk);
                    mm->wired = true;
                    PRINTF("update monitor %s (0x%X)\n", mm->name, mm->id);
                } else {
                    mm = add_monitor(rect);
                    char *name = (char *)xcb_randr_get_output_info_name(info);
                    size_t name_len = MIN(sizeof(mm->name), (size_t)xcb_randr_get_output_info_name_length(info));
                    strncpy(mm->name, name, name_len);
                    mm->name[name_len] = '\0';
                    mm->id = outputs[i];
                    PRINTF("add monitor %s (0x%X)\n", mm->name, mm->id);
                }
                num++;
            }
            free(cir);
        }
        free(info);
    }

    /* initially focus the primary monitor and add the first desktop to it */
    xcb_randr_get_output_primary_reply_t *gpo = xcb_randr_get_output_primary_reply(dpy, xcb_randr_get_output_primary(dpy, root), NULL);
    if (gpo != NULL) {
        pri_mon = get_monitor_by_id(gpo->output);
        if (!running && pri_mon != NULL) {
            if (mon != pri_mon)
                mon = pri_mon;
            add_desktop(pri_mon, make_desktop(NULL));
            ewmh_update_current_desktop();
        }
    }
    free(gpo);

    /* add one desktop to each new monitor */
    for (monitor_t *m = mon_head; m != NULL; m = m->next)
        if (m->desk == NULL && (running || pri_mon == NULL || m != pri_mon))
            add_desktop(m, make_desktop(NULL));

    /* merge and remove disconnected monitors */
    monitor_t *m = mon_head;
    while (m != NULL) {
        monitor_t *next = m->next;
        if (!m->wired) {
            PRINTF("remove monitor %s (0x%X)\n", m->name, m->id);
            merge_monitors(m, mm);
            remove_monitor(m);
        }
        m = next;
    }

    free(sres);
    update_motion_recorder();
    return (num_monitors > 0);
}

void quit(void)
{
    running = false;
}

void cleanup(void)
{
    while (mon_head != NULL)
        remove_monitor(mon_head);
    while (rule_head != NULL)
        remove_rule(rule_head);
    free(frozen_pointer);
}

void put_status(void)
{
    if (status_fifo == NULL)
        return;
    if (status_prefix != NULL)
        fprintf(status_fifo, "%s", status_prefix);
    bool urgent = false;
    for (monitor_t *m = mon_head; m != NULL; m = m->next) {
        fprintf(status_fifo, "%c%s:", (mon == m ? 'M' : 'm'), m->name);
        for (desktop_t *d = m->desk_head; d != NULL; d = d->next, urgent = false) {
            for (node_t *n = first_extrema(d->root); n != NULL && !urgent; n = next_leaf(n, d->root))
                urgent |= n->client->urgent;
            char c = (urgent ? 'u' : (d->root == NULL ? 'f' : 'o'));
            if (m->desk == d)
                c = toupper(c);
            fprintf(status_fifo, "%c%s:", c, d->name);
        }
    }
    if (mon != NULL && mon->desk != NULL)
        fprintf(status_fifo, "L%s", (mon->desk->layout == LAYOUT_TILED ? "tiled" : "monocle"));
    fprintf(status_fifo, "\n");
    fflush(status_fifo);
}
