#include <stdio.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "utils.h"
#include "events.h"

void handle_event(xcb_generic_event_t *evt)
{
    switch (XCB_EVENT_RESPONSE_TYPE(evt)) {
        case XCB_MAP_REQUEST:
            PUTS("received a map request\n");
            break;
        case XCB_CONFIGURE_REQUEST:
            PUTS("received a map request\n");
            break;
        case XCB_UNGRAB_KEY:
            PUTS("ungrab key received");
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
