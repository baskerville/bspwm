#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_ewmh.h>
#include "bsps.h"

#define NO_VALUE  " "
#define NAME_SEP  "\0"

typedef enum {
    false,
    true
} bool;

char *deskname = NO_VALUE;
char *wintitle = NO_VALUE;
xcb_window_t curwin;

xcb_connection_t *dpy;
xcb_ewmh_connection_t ewmh;
xcb_screen_t *screen;
int default_screen;

bool running;

void setup(void)
{
    dpy = xcb_connect(NULL, &default_screen);
    xcb_intern_atom_cookie_t *ewmh_cookies;
    ewmh_cookies = xcb_ewmh_init_atoms(dpy, &ewmh);
    xcb_ewmh_init_atoms_replies(&ewmh, ewmh_cookies, NULL);
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

void update_wintitle(void)
{
    xcb_window_t win;
    xcb_ewmh_get_utf8_strings_reply_t name;
    xcb_generic_error_t *err;
    uint32_t values[] = {XCB_EVENT_MASK_PROPERTY_CHANGE};
    uint32_t values_reset[] = {XCB_EVENT_MASK_NO_EVENT};

    if (xcb_ewmh_get_active_window_reply(&ewmh, xcb_ewmh_get_active_window(&ewmh, default_screen), &win, NULL) == 1)
    {
        if (xcb_ewmh_get_wm_name_reply(&ewmh, xcb_ewmh_get_wm_name(&ewmh, win), &name, NULL) == 1) {
            wintitle = strdup(name.strings);
            if (win != curwin) {
                xcb_change_window_attributes(dpy, curwin, XCB_CW_EVENT_MASK, values_reset);
                curwin = win;
            }
            err = xcb_request_check(dpy, xcb_change_window_attributes_checked(dpy, win, XCB_CW_EVENT_MASK, values));
            if (err != NULL)
                running = false;

        } else {
            wintitle = strdup(NO_VALUE);
        }
    }
}

void update_deskname(void)
{
    uint32_t cd;
    xcb_ewmh_get_utf8_strings_reply_t names;
    unsigned int pos = 0, cnt = 0;

    if (xcb_ewmh_get_current_desktop_reply(&ewmh, xcb_ewmh_get_current_desktop(&ewmh, default_screen), &cd, NULL) == 1  && xcb_ewmh_get_desktop_names_reply(&ewmh, xcb_ewmh_get_desktop_names(&ewmh, default_screen), &names, NULL) == 1) {
        while (cnt < cd && pos < names.strings_len) {
            pos += strlen(names.strings + pos) + 1;
            cnt++;
        }
        deskname = strdup(names.strings + pos);
        for (cnt = 0; cnt < strlen(deskname); cnt++)
            deskname[cnt] = toupper(deskname[cnt]);
    } else {
        deskname = strdup(NO_VALUE);
    }
}

void output_infos(void)
{
    printf("\\l %s\\c%s\\r12:34 \n", deskname, wintitle);
    fflush(stdout);
}

/* int main(int argc, char *argv[]) */
int main(void)
{
    xcb_generic_event_t *evt;
    xcb_property_notify_event_t *pne;
    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);
    signal(SIGHUP, handle_signal);
    setup();
    screen = ewmh.screens[default_screen];
    register_events();
    update_deskname();
    update_wintitle();
    output_infos();
    xcb_flush(dpy);
    while (running) {
        evt = xcb_wait_for_event(dpy);
        switch (XCB_EVENT_RESPONSE_TYPE(evt)) {
            case XCB_PROPERTY_NOTIFY:
                pne = (xcb_property_notify_event_t *) evt;
                if (pne->atom == ewmh._NET_CURRENT_DESKTOP) {
                    update_deskname();
                    output_infos();
                } else if (pne->atom == ewmh._NET_ACTIVE_WINDOW) {
                    update_wintitle();
                    output_infos();
                } else if (pne->window != screen->root && (pne->atom == ewmh._NET_WM_NAME || pne->atom == XCB_ATOM_WM_NAME)) {
                    update_wintitle();
                    output_infos();
                } else {
                    /* printf("property %i\n", pne->atom); */
                }
            case XCB_CLIENT_MESSAGE:
                /* puts("client message"); */
                break;
            default:
                /* printf("event %i\n", XCB_EVENT_RESPONSE_TYPE(evt)); */
                break;
        }
        free(evt);
    }

    xcb_disconnect(dpy);
    return 0;
}
