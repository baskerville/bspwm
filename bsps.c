#define _BSD_SOURCE

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_ewmh.h>
#include <cairo/cairo.h>
#include "bsps.h"

#define MAX(A, B)         ((A) > (B) ? (A) : (B))

#define FIFO_PATH  "BSPWM_FIFO"
#define NO_VALUE   " "
#define NAME_SEP   "\0"
#define FONT_FAMILY "sans-serif"
#define FONT_SIZE   11

typedef enum {
    false,
    true
} bool;

char *desktop_name = NO_VALUE;
char *window_title = NO_VALUE;
char *font_family = FONT_FAMILY;
int font_size = FONT_SIZE;
char external_info[BUFSIZ] = NO_VALUE;
xcb_window_t curwin;

char *fifo_path;
int fifo_fd, dpy_fd, sel_fd;

xcb_connection_t *dpy;
xcb_ewmh_connection_t ewmh;
xcb_screen_t *screen;
int default_screen;

cairo_surface_t *surface;
cairo_t *cr;

bool running;

double text_width(char *s)
{
    cairo_text_extents_t te;
    cairo_select_font_face(cr, font_family, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, font_size);
    cairo_text_extents(cr, s, &te);
    return te.x_advance;
}

void setup(void)
{
    dpy = xcb_connect(NULL, &default_screen);
    xcb_intern_atom_cookie_t *ewmh_cookies;
    ewmh_cookies = xcb_ewmh_init_atoms(dpy, &ewmh);
    xcb_ewmh_init_atoms_replies(&ewmh, ewmh_cookies, NULL);
    surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, 0, 0);
    cr = cairo_create(surface);
    fifo_path = getenv(FIFO_PATH);
    mkfifo(fifo_path, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    /* http://www.outflux.net/blog/archives/2008/03/09/using-select-on-a-fifo/ */
    fifo_fd = open(fifo_path, O_RDWR | O_NONBLOCK);
    dpy_fd = xcb_get_file_descriptor(dpy);
    sel_fd = MAX(fifo_fd, dpy_fd) + 1;
    curwin = 0;
    running = true;
}

void register_events(void)
{
    xcb_generic_error_t *err;
    uint32_t values[] = {XCB_EVENT_MASK_PROPERTY_CHANGE};
    err = xcb_request_check(dpy, xcb_change_window_attributes_checked(dpy, screen->root, XCB_CW_EVENT_MASK, values));
    if (err != NULL)
        running = false;
}

void handle_signal(int sig)
{
    if (sig == SIGTERM || sig == SIGINT || sig == SIGHUP)
        running = false;
}

void update_window_title(void)
{
    xcb_window_t win;
    xcb_ewmh_get_utf8_strings_reply_t name;
    xcb_generic_error_t *err;
    uint32_t values[] = {XCB_EVENT_MASK_PROPERTY_CHANGE};
    uint32_t values_reset[] = {XCB_EVENT_MASK_NO_EVENT};

    if (xcb_ewmh_get_active_window_reply(&ewmh, xcb_ewmh_get_active_window(&ewmh, default_screen), &win, NULL) == 1)
    {
        if (xcb_ewmh_get_wm_name_reply(&ewmh, xcb_ewmh_get_wm_name(&ewmh, win), &name, NULL) == 1) {
            window_title = strdup(name.strings);
            if (win != curwin) {
                xcb_change_window_attributes(dpy, curwin, XCB_CW_EVENT_MASK, values_reset);
                curwin = win;
            }
            err = xcb_request_check(dpy, xcb_change_window_attributes_checked(dpy, win, XCB_CW_EVENT_MASK, values));
            if (err != NULL)
                running = false;

        } else {
            window_title = strdup(NO_VALUE);
        }
    }
}

void update_desktop_name(void)
{
    uint32_t cd;
    xcb_ewmh_get_utf8_strings_reply_t names;
    unsigned int pos = 0, cnt = 0;

    if (xcb_ewmh_get_current_desktop_reply(&ewmh, xcb_ewmh_get_current_desktop(&ewmh, default_screen), &cd, NULL) == 1  && xcb_ewmh_get_desktop_names_reply(&ewmh, xcb_ewmh_get_desktop_names(&ewmh, default_screen), &names, NULL) == 1) {
        while (cnt < cd && pos < names.strings_len) {
            pos += strlen(names.strings + pos) + 1;
            cnt++;
        }
        desktop_name = strdup(names.strings + pos);
        for (cnt = 0; cnt < strlen(desktop_name); cnt++)
            desktop_name[cnt] = toupper(desktop_name[cnt]);
    } else {
        desktop_name = strdup(NO_VALUE);
    }
}

void output_infos(void)
{
    /* printf("\\l %s\\c%s\\r%s \n", desktop_name, window_title, external_info); */
    printf("%s                %s                %s\n", desktop_name, window_title, external_info);
    fflush(stdout);
}

void handle_event(xcb_generic_event_t *evt)
{
    xcb_property_notify_event_t *pne;
    switch (XCB_EVENT_RESPONSE_TYPE(evt)) {
        case XCB_PROPERTY_NOTIFY:
            pne = (xcb_property_notify_event_t *) evt;
            if (pne->atom == ewmh._NET_CURRENT_DESKTOP) {
                update_desktop_name();
                output_infos();
            } else if (pne->atom == ewmh._NET_ACTIVE_WINDOW) {
                update_window_title();
                output_infos();
            } else if (pne->window != screen->root && (pne->atom == ewmh._NET_WM_NAME || pne->atom == XCB_ATOM_WM_NAME)) {
                update_window_title();
                output_infos();
            } else {
            }
        default:
            break;
    }
}

int main(int argc, char *argv[])
{
    fd_set descriptors;
    xcb_generic_event_t *evt;
    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);
    signal(SIGHUP, handle_signal);
    if (argc > 1) {
        font_family = strdup(argv[1]);
        if (argc > 2)
            font_size = atoi(argv[2]);
    }
    setup();
    screen = ewmh.screens[default_screen];
    register_events();
    update_desktop_name();
    update_window_title();
    output_infos();
    xcb_flush(dpy);

    while (running) {
        FD_ZERO(&descriptors);
        FD_SET(fifo_fd, &descriptors);
        FD_SET(dpy_fd, &descriptors);

        if (select(sel_fd, &descriptors, NULL, NULL, NULL)) {
            if (FD_ISSET(dpy_fd, &descriptors)) {
                while ((evt = xcb_poll_for_event(dpy)) != NULL) {
                    handle_event(evt);
                    free(evt);
                }
            }

            if (FD_ISSET(fifo_fd, &descriptors)) {
                int bytes = read(fifo_fd, external_info, sizeof(external_info));
                external_info[bytes] = '\0';
                output_infos();
            }
        }
    }

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    close(fifo_fd);
    xcb_disconnect(dpy);
    return 0;
}
