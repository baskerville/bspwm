#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_icccm.h>
#include "types.h"
#include "bspwm.h"
#include "settings.h"
#include "buttons.h"
#include "helpers.h"
#include "window.h"
#include "events.h"
#include "tree.h"
#include "rules.h"
#include "ewmh.h"

void handle_event(xcb_generic_event_t *evt)
{
    switch (XCB_EVENT_RESPONSE_TYPE(evt)) {
        case XCB_MAP_REQUEST:
            map_request(evt);
            break;
        case XCB_DESTROY_NOTIFY:
            destroy_notify(evt);
            break;
        case XCB_UNMAP_NOTIFY:
            unmap_notify(evt);
            break;
        case XCB_CLIENT_MESSAGE:
            client_message(evt);
            break;
        case XCB_CONFIGURE_REQUEST:
            configure_request(evt);
            break;
        case XCB_PROPERTY_NOTIFY:
            property_notify(evt);
            break;
        case XCB_MAPPING_NOTIFY:
            mapping_notify(evt);
            break;
        case XCB_ENTER_NOTIFY:
            enter_notify(evt);
            break;
        case XCB_BUTTON_PRESS:
            button_press(evt);
            break;
        case XCB_MOTION_NOTIFY:
            motion_notify(evt);
            break;
        case XCB_BUTTON_RELEASE:
            button_release();
            break;
        default:
            break;
    }
}

void map_request(xcb_generic_event_t *evt)
{
    xcb_map_request_event_t *e = (xcb_map_request_event_t *) evt;

    PRINTF("map request %X\n", e->window);

    manage_window(mon, mon->desk, e->window);
}

void configure_request(xcb_generic_event_t *evt)
{
    xcb_configure_request_event_t *e = (xcb_configure_request_event_t *) evt;

    PRINTF("configure request %X\n", e->window);

    window_location_t loc;
    bool is_managed = locate_window(e->window, &loc);

    if (!is_managed || is_floating(loc.node->client)) {
        uint16_t mask = 0;
        uint32_t values[7];
        unsigned short i = 0;

        if (e->value_mask & XCB_CONFIG_WINDOW_X) {
            mask |= XCB_CONFIG_WINDOW_X;
            values[i++] = e->x;
            if (is_managed)
                loc.node->client->floating_rectangle.x = e->x;
        }

        if (e->value_mask & XCB_CONFIG_WINDOW_Y) {
            mask |= XCB_CONFIG_WINDOW_Y;
            values[i++] = e->y;
            if (is_managed)
                loc.node->client->floating_rectangle.y = e->y;
        }

        if (e->value_mask & XCB_CONFIG_WINDOW_WIDTH) {
            mask |= XCB_CONFIG_WINDOW_WIDTH;
            values[i++] = e->width;
            if (is_managed)
                loc.node->client->floating_rectangle.width = e->width;
        }

        if (e->value_mask & XCB_CONFIG_WINDOW_HEIGHT) {
            mask |= XCB_CONFIG_WINDOW_HEIGHT;
            values[i++] = e->height;
            if (is_managed)
                loc.node->client->floating_rectangle.height = e->height;
        }

        if (!is_managed && e->value_mask & XCB_CONFIG_WINDOW_BORDER_WIDTH) {
            mask |= XCB_CONFIG_WINDOW_BORDER_WIDTH;
            values[i++] = e->border_width;
        }

        if (e->value_mask & XCB_CONFIG_WINDOW_SIBLING) {
            mask |= XCB_CONFIG_WINDOW_SIBLING;
            values[i++] = e->sibling;
        }

        if (e->value_mask & XCB_CONFIG_WINDOW_STACK_MODE) {
            mask |= XCB_CONFIG_WINDOW_STACK_MODE;
            values[i++] = e->stack_mode;
        }

        xcb_configure_window(dpy, e->window, mask, values);
        if (is_managed)
            window_draw_border(loc.node, loc.node == loc.desktop->focus, loc.monitor == mon);
    } else {
        xcb_configure_notify_event_t evt;
        xcb_rectangle_t rect;
        unsigned int bw;
        xcb_window_t win = loc.node->client->window;

        if (is_tiled(loc.node->client)) {
            rect = loc.node->client->tiled_rectangle;
            bw = border_width;
        } else {
            rect = loc.monitor->rectangle;
            bw = 0;
        }

        evt.response_type = XCB_CONFIGURE_NOTIFY;
        evt.event = win;
        evt.window = win;
        evt.above_sibling = XCB_NONE;
        evt.x = rect.x;
        evt.y = rect.y;
        evt.width = rect.width;
        evt.height = rect.height;
        evt.border_width = bw;
        evt.override_redirect = false;

        xcb_send_event(dpy, false, win, XCB_EVENT_MASK_STRUCTURE_NOTIFY, (const char *) &evt);
    }
}

void destroy_notify(xcb_generic_event_t *evt)
{
    xcb_destroy_notify_event_t *e = (xcb_destroy_notify_event_t *) evt;

    PRINTF("destroy notify %X\n", e->window);

    window_location_t loc;
    if (locate_window(e->window, &loc)) {
        remove_node(loc.desktop, loc.node);
        arrange(loc.monitor, loc.desktop);
    }
}

void unmap_notify(xcb_generic_event_t *evt)
{
    xcb_unmap_notify_event_t *e = (xcb_unmap_notify_event_t *) evt;

    PRINTF("unmap notify %X\n", e->window);

    window_location_t loc;
    if (locate_window(e->window, &loc)) {
        remove_node(loc.desktop, loc.node);
        arrange(loc.monitor, loc.desktop);
    }
}

void property_notify(xcb_generic_event_t *evt)
{
    xcb_property_notify_event_t *e = (xcb_property_notify_event_t *) evt;
    xcb_icccm_wm_hints_t hints;

    /* PRINTF("property notify %X\n", e->window); */

    if (e->atom != XCB_ATOM_WM_HINTS)
        return;

    window_location_t loc;
    if (locate_window(e->window, &loc)) {
        if (xcb_icccm_get_wm_hints_reply(dpy, xcb_icccm_get_wm_hints(dpy, e->window), &hints, NULL) == 1) {
            uint32_t urgent = xcb_icccm_wm_hints_get_urgency(&hints);
            if (urgent != 0 && loc.node != mon->desk->focus) {
                loc.node->client->urgent = urgent;
                put_status();
                if (loc.monitor->desk == loc.desktop)
                    arrange(loc.monitor, loc.desktop);
            }
        }
    }
}

void mapping_notify(xcb_generic_event_t *evt)
{
    xcb_mapping_notify_event_t *e = (xcb_mapping_notify_event_t *) evt;
    if (e->count > 0) {
        ungrab_buttons();
        xcb_refresh_keyboard_mapping(symbols, e);
        get_lock_fields();
        grab_buttons();
    }
}

void enter_notify(xcb_generic_event_t *evt)
{
    xcb_enter_notify_event_t *e = (xcb_enter_notify_event_t *) evt;

    PRINTF("enter notify %X %d %d\n", e->event, e->mode, e->detail);

    if (!focus_follows_mouse 
            || (e->mode != XCB_NOTIFY_MODE_NORMAL && e->detail == XCB_NOTIFY_DETAIL_INFERIOR)
            || (pointer_position.x == e->root_x && pointer_position.y == e->root_y)
            || last_entered == e->event)
        return;

    window_location_t loc;
    if (locate_window(e->event, &loc)) {
        select_monitor(loc.monitor);
        focus_node(loc.monitor, loc.desktop, loc.node, true);
        pointer_position = (xcb_point_t) {e->root_x, e->root_y};
        last_entered = e->event;
    }
}

void client_message(xcb_generic_event_t *evt)
{
    xcb_client_message_event_t *e = (xcb_client_message_event_t *) evt;

    PRINTF("client message %X %u\n", e->window, e->type);

    if (e->type == ewmh->_NET_CURRENT_DESKTOP) {
        desktop_location_t loc;
        if (ewmh_locate_desktop(e->data.data32[0], &loc)) {
            select_monitor(loc.monitor);
            select_desktop(loc.desktop);
        }
        return;
    }

    window_location_t loc;
    if (!locate_window(e->window, &loc))
        return;

    if (e->type == ewmh->_NET_WM_STATE) {
        handle_state(loc.monitor, loc.desktop, loc.node, e->data.data32[1], e->data.data32[0]);
        handle_state(loc.monitor, loc.desktop, loc.node, e->data.data32[2], e->data.data32[0]);
    } else if (e->type == ewmh->_NET_ACTIVE_WINDOW) {
        if (loc.desktop->focus->client->fullscreen && loc.desktop->focus != loc.node)
            toggle_fullscreen(loc.monitor, loc.desktop->focus->client);
        select_monitor(loc.monitor);
        select_desktop(loc.desktop);
        focus_node(loc.monitor, loc.desktop, loc.node, true);
        arrange(loc.monitor, loc.desktop);
    }
}

void button_press(xcb_generic_event_t *evt)
{
    xcb_button_press_event_t *e = (xcb_button_press_event_t *) evt;
    xcb_window_t win = e->child;

    PRINTF("button press %u %u %X\n", e->detail, e->state, win);

    window_location_t loc;
    if (locate_window(win, &loc)) {
        client_t *c = loc.node->client;
        switch (e->detail)  {
            case XCB_BUTTON_INDEX_2:
                focus_node(loc.monitor, loc.desktop, loc.node, true);
                break;
            case XCB_BUTTON_INDEX_1:
            case XCB_BUTTON_INDEX_3:
                if (is_tiled(loc.node->client)) {
                    loc.node->client->floating_rectangle = loc.node->client->tiled_rectangle;
                    toggle_floating(loc.node);
                    arrange(loc.monitor, loc.desktop);
                } else if (loc.node->client->fullscreen) {
                    return;
                }
                frozen_pointer->monitor = loc.monitor;
                frozen_pointer->desktop = loc.desktop;
                frozen_pointer->node = loc.node;
                frozen_pointer->rectangle = c->floating_rectangle;
                frozen_pointer->position = (xcb_point_t) {e->root_x, e->root_y};
                frozen_pointer->button = e->detail;
                if (e->detail == XCB_BUTTON_INDEX_3) {
                    int16_t mid_x, mid_y;
                    mid_x = c->floating_rectangle.x + (c->floating_rectangle.width / 2);
                    mid_y = c->floating_rectangle.y + (c->floating_rectangle.height / 2);
                    if (e->root_x > mid_x) {
                        if (e->root_y > mid_y)
                            frozen_pointer->corner = BOTTOM_RIGHT;
                        else
                            frozen_pointer->corner = TOP_RIGHT;
                    } else {
                        if (e->root_y > mid_y)
                            frozen_pointer->corner = BOTTOM_LEFT;
                        else
                            frozen_pointer->corner = TOP_LEFT;
                    }
                }
                xcb_grab_pointer(dpy, false, root, XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_CURRENT_TIME);
                break;
        }
    }
}

void motion_notify(xcb_generic_event_t *evt)
{
    xcb_motion_notify_event_t *e = (xcb_motion_notify_event_t *) evt;

    int16_t delta_x, delta_y, x, y, w, h;
    uint16_t width, height;

    monitor_t *m = frozen_pointer->monitor;
    desktop_t *d = frozen_pointer->desktop;
    node_t *n = frozen_pointer->node;
    client_t *c = n->client;
    xcb_rectangle_t rect = frozen_pointer->rectangle;
    xcb_window_t win = c->window;

    x = y = 0;
    w = h = 1;

    /* PRINTF("motion notify %X\n", win); */

    delta_x = e->root_x - frozen_pointer->position.x;
    delta_y = e->root_y - frozen_pointer->position.y;

    switch (frozen_pointer->button) {
        case XCB_BUTTON_INDEX_1:
            x = rect.x + delta_x;
            y = rect.y + delta_y;
            window_move(win, x, y);
            break;
        case XCB_BUTTON_INDEX_3:
            switch (frozen_pointer->corner) {
                case TOP_LEFT:
                    x = rect.x + delta_x;
                    y = rect.y + delta_y;
                    w = rect.width - delta_x;
                    h = rect.height - delta_y;
                    break;
                case TOP_RIGHT:
                    x = rect.x;
                    y = rect.y + delta_y;
                    w = rect.width + delta_x;
                    h = rect.height - delta_y;
                    break;
                case BOTTOM_LEFT:
                    x = rect.x + delta_x;
                    y = rect.y;
                    w = rect.width - delta_x;
                    h = rect.height + delta_y;
                    break;
                case BOTTOM_RIGHT:
                    x = rect.x;
                    y = rect.y;
                    w = rect.width + delta_x;
                    h = rect.height + delta_y;
                    break;
            }
            width = MAX(1, w);
            height = MAX(1, h);
            window_move_resize(win, x, y, width, height);
            c->floating_rectangle = (xcb_rectangle_t) {x, y, width, height};
            window_draw_border(n, d->focus == n, mon == m);
            break;
    }
}

void button_release(void)
{
    PUTS("button release");

    xcb_ungrab_pointer(dpy, XCB_CURRENT_TIME);
    update_floating_rectangle(frozen_pointer->node->client);
    monitor_t *m = underlying_monitor(frozen_pointer->node->client);
    if (m != NULL && m != frozen_pointer->monitor) {
        transfer_node(frozen_pointer->monitor, frozen_pointer->desktop, m, m->desk, frozen_pointer->node);
        select_monitor(m);
    }
}

void handle_state(monitor_t *m, desktop_t *d, node_t *n, xcb_atom_t state, unsigned int action)
{
    if (state == ewmh->_NET_WM_STATE_FULLSCREEN) {
        bool fs = n->client->fullscreen;
        if (action == XCB_EWMH_WM_STATE_TOGGLE
                || (fs && action == XCB_EWMH_WM_STATE_REMOVE)
                || (!fs && action == XCB_EWMH_WM_STATE_ADD)) {
            toggle_fullscreen(m, n->client);
            arrange(m, d);
        }
    }
}
