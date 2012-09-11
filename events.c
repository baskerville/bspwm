#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "helpers.h"
#include "types.h"
#include "bspwm.h"
#include "utils.h"
#include "events.h"
#include "tree.h"
#include "ewmh.h"

void handle_event(xcb_generic_event_t *evt)
{
    switch (XCB_EVENT_RESPONSE_TYPE(evt)) {
        case XCB_MAP_REQUEST:
            PUTS("received a map request\n");
            map_request(evt);
            break;
        case XCB_CLIENT_MESSAGE:
            PUTS("received a map request\n");
            client_message(evt);
            break;
        case XCB_CONFIGURE_REQUEST:
            PUTS("received a configure request\n");
            break;
        case XCB_BUTTON_PRESS:
            PUTS("button press event");
            break;
        default:
            PRINTF("received event %i\n", XCB_EVENT_RESPONSE_TYPE(evt));
    }
}

void map_request(xcb_generic_event_t *evt)
{
    xcb_map_request_event_t *e = (xcb_map_request_event_t *) evt;
    xcb_get_window_attributes_reply_t  *wa;
    xcb_window_t win = e->window;
    wa = xcb_get_window_attributes_reply(dpy, xcb_get_window_attributes(dpy, win), NULL);

    if ((wa != NULL && wa->override_redirect) || is_managed(win))
        return;

    free(wa);
    bool takes_focus = true;
    client_t *c = make_client(win);
    num_clients++;
    node_t *birth = make_node();
    birth->client = c;
    insert_node(desk, birth);
    if (takes_focus)
        focus_node(desk, birth);
    xcb_map_window(dpy, c->window);
}

void client_message(xcb_generic_event_t *evt)
{
    xcb_client_message_event_t *e = (xcb_client_message_event_t *) evt;
    window_location_t wl = locate_window(e->window);
    if (wl.desktop == NULL || wl.node == NULL)
        return;

    if (e->type == ewmh._NET_WM_STATE) {
        handle_state(wl.node, e->data.data32[1], e->data.data32[0]);
        handle_state(wl.node, e->data.data32[2], e->data.data32[0]);
    } else if (e->type == ewmh._NET_ACTIVE_WINDOW) {
        focus_node(wl.desktop, wl.node);
    }
}

void handle_state(node_t *n, xcb_atom_t state, unsigned int action)
{
    if (state == ewmh._NET_WM_STATE_FULLSCREEN) {
        bool fs = n->client->fullscreen;
        if (action == XCB_EWMH_WM_STATE_TOGGLE
                || (fs && action == XCB_EWMH_WM_STATE_REMOVE)
                || (!fs && action == XCB_EWMH_WM_STATE_ADD))
            toggle_fullscreen(n);
    }
}
