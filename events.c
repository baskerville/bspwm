#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "helpers.h"
#include "types.h"
#include "bspwm.h"
#include "utils.h"
#include "events.h"

void handle_event(xcb_generic_event_t *evt)
{
    switch (XCB_EVENT_RESPONSE_TYPE(evt)) {
        case XCB_MAP_REQUEST:
            PUTS("received a map request\n");
            map_request(evt);
            break;
        case XCB_CONFIGURE_REQUEST:
            PUTS("received a map request\n");
            break;
        case XCB_UNGRAB_KEY:
            /* PUTS("ungrab key received"); */
            break;
        case XCB_KEY_PRESS:
            PUTS("keypress received");
            break;
        case XCB_KEY_RELEASE:
            PUTS("keyrelease received");
            break;
        case XCB_BUTTON_PRESS:
        case XCB_BUTTON_RELEASE:
            PUTS("button event");
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
    if ((wa != NULL && wa->override_redirect) || win_to_node(win) != NULL)
        return;
    free(wa);
    client_t *c = make_client();
    c->window = win;
    num_clients++;
}
