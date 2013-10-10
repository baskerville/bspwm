#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_icccm.h>

#define FRAME_CLASS_NAME     "BSPWM_FRAME"
#define FRAME_INSTANCE_NAME  "bspwm_frame"

xcb_connection_t *dpy;

bool get_atom(char *name, xcb_atom_t *atom)
{
    bool ack = true;
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(dpy, xcb_intern_atom(dpy, 0, strlen(name), name), NULL);
    if (reply != NULL)
        *atom = reply->atom;
    else
        ack = false;
    free(reply);
    return ack;
}

int main(int argc, char *argv[])
{
    dpy = xcb_connect(NULL, NULL);
    if (dpy == NULL)
        return EXIT_FAILURE;
    xcb_atom_t WM_PROTOCOLS, WM_DELETE_WINDOW;
    if (!get_atom("WM_PROTOCOLS", &WM_PROTOCOLS)
            || !get_atom("WM_DELETE_WINDOW", &WM_DELETE_WINDOW)) {
        xcb_disconnect(dpy);
        return EXIT_FAILURE;
    }
    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(dpy)).data;
    if (screen == NULL)
        return EXIT_FAILURE;
    xcb_window_t root = screen->root;
    xcb_window_t win = xcb_generate_id(dpy);
    xcb_create_window(dpy, XCB_COPY_FROM_PARENT, win, root, 0, 0, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, XCB_COPY_FROM_PARENT, 0, NULL);
    xcb_icccm_set_wm_class(dpy, win, strlen(FRAME_CLASS_NAME) + strlen(FRAME_INSTANCE_NAME) + 1, FRAME_INSTANCE_NAME "\0" FRAME_CLASS_NAME);
    xcb_map_window(dpy, win);
    xcb_flush(dpy);
    xcb_generic_event_t *evt;
    bool running = true;
    while (running && (evt = xcb_wait_for_event(dpy)) != NULL) {
        uint8_t rt = XCB_EVENT_RESPONSE_TYPE(evt);
        if (rt == XCB_CLIENT_MESSAGE)  {
            xcb_client_message_event_t *cme = (xcb_client_message_event_t *) evt;
            if (cme->type == WM_PROTOCOLS && cme->data.data32[0] == WM_DELETE_WINDOW)
                running = false;
        }
        free(evt);
    }
    xcb_destroy_window(dpy, win);
    xcb_disconnect(dpy);
    return EXIT_SUCCESS;
}
