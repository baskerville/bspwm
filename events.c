#include <stdio.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "events.h"

void handle_event(xcb_generic_event_t *evt)
{
    switch (XCB_EVENT_RESPONSE_TYPE(evt)) {
        case XCB_MAP_REQUEST:
            printf("received a map request\n");
            break;
        default:
            printf("received some event\n");
    }
}
