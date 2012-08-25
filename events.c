#include <stdio.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "helpers.h"
#include "types.h"
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
    /* if (e->override_redirect) */
    /*     return; */
    xcb_window_t win = e->window;
    Client *c = make_client();
    c->window = win;
}
