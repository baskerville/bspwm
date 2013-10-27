/* * Copyright (c) 2012-2013 Bastien Dejean
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include "bspwm.h"
#include "ewmh.h"
#include "monitor.h"
#include "query.h"
#include "settings.h"
#include "tree.h"
#include "window.h"
#include "events.h"

void handle_event(xcb_generic_event_t *evt)
{
    uint8_t resp_type = XCB_EVENT_RESPONSE_TYPE(evt);
    switch (resp_type) {
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
        case XCB_ENTER_NOTIFY:
            enter_notify(evt);
            break;
        case XCB_MOTION_NOTIFY:
            motion_notify(evt);
            break;
        case XCB_FOCUS_IN:
            focus_in(evt);
            break;
        case XCB_EXPOSE:
            expose(evt);
            break;
        default:
            if (randr && resp_type == randr_base + XCB_RANDR_SCREEN_CHANGE_NOTIFY)
                import_monitors();
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

    coordinates_t loc;
    bool is_managed = locate_window(e->window, &loc);

    if (is_managed && !is_floating(loc.node->client)) {
        if (e->value_mask & XCB_CONFIG_WINDOW_X)
            loc.node->client->floating_rectangle.x = e->x;
        if (e->value_mask & XCB_CONFIG_WINDOW_Y)
            loc.node->client->floating_rectangle.y = e->y;
        if (e->value_mask & XCB_CONFIG_WINDOW_WIDTH)
            loc.node->client->floating_rectangle.width = e->width;
        if (e->value_mask & XCB_CONFIG_WINDOW_HEIGHT)
            loc.node->client->floating_rectangle.height = e->height;

        xcb_configure_notify_event_t evt;
        xcb_rectangle_t rect;
        xcb_window_t win = loc.node->client->window;
        unsigned int bw = loc.node->client->border_width;

        if (loc.node->client->fullscreen)
            rect = loc.monitor->rectangle;
        else
            rect = loc.node->client->tiled_rectangle;

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
    } else {
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
    }
    if (is_managed)
        translate_client(monitor_from_client(loc.node->client), loc.monitor, loc.node->client);
}

void destroy_notify(xcb_generic_event_t *evt)
{
    xcb_destroy_notify_event_t *e = (xcb_destroy_notify_event_t *) evt;

    PRINTF("destroy notify %X\n", e->window);

    coordinates_t loc;
    if (locate_window(e->window, &loc)) {
        remove_node(loc.monitor, loc.desktop, loc.node);
        arrange(loc.monitor, loc.desktop);
    }
}

void unmap_notify(xcb_generic_event_t *evt)
{
    xcb_unmap_notify_event_t *e = (xcb_unmap_notify_event_t *) evt;

    PRINTF("unmap notify %X\n", e->window);

    coordinates_t loc;
    if (locate_window(e->window, &loc)) {
        remove_node(loc.monitor, loc.desktop, loc.node);
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

    coordinates_t loc;
    if (locate_window(e->window, &loc)
            && xcb_icccm_get_wm_hints_reply(dpy, xcb_icccm_get_wm_hints(dpy, e->window), &hints, NULL) == 1)
        set_urgency(loc.monitor, loc.desktop, loc.node, xcb_icccm_wm_hints_get_urgency(&hints));
}

void client_message(xcb_generic_event_t *evt)
{
    xcb_client_message_event_t *e = (xcb_client_message_event_t *) evt;

    PRINTF("client message %X %u\n", e->window, e->type);

    if (e->type == ewmh->_NET_CURRENT_DESKTOP) {
        coordinates_t loc;
        if (ewmh_locate_desktop(e->data.data32[0], &loc))
            focus_node(loc.monitor, loc.desktop, loc.desktop->focus);
        return;
    }

    coordinates_t loc;
    if (!locate_window(e->window, &loc))
        return;

    if (e->type == ewmh->_NET_WM_STATE) {
        handle_state(loc.monitor, loc.desktop, loc.node, e->data.data32[1], e->data.data32[0]);
        handle_state(loc.monitor, loc.desktop, loc.node, e->data.data32[2], e->data.data32[0]);
    } else if (e->type == ewmh->_NET_ACTIVE_WINDOW) {
        if (ignore_ewmh_focus || loc.node == mon->desk->focus)
            return;
        if (loc.desktop->focus->client->fullscreen && loc.desktop->focus != loc.node) {
            set_fullscreen(loc.desktop->focus, false);
            arrange(loc.monitor, loc.desktop);
        }
        focus_node(loc.monitor, loc.desktop, loc.node);
    } else if (e->type == ewmh->_NET_WM_DESKTOP) {
        coordinates_t dloc;
        if (ewmh_locate_desktop(e->data.data32[0], &dloc))
            transfer_node(loc.monitor, loc.desktop, loc.node, dloc.monitor, dloc.desktop, dloc.desktop->focus);
    } else if (e->type == ewmh->_NET_CLOSE_WINDOW) {
        window_close(loc.node);
    }
}

void focus_in(xcb_generic_event_t *evt)
{
    xcb_focus_in_event_t *e = (xcb_focus_in_event_t *) evt;

    /* PRINTF("focus in %X %d %d\n", e->event, e->mode, e->detail); */

    if (e->mode == XCB_NOTIFY_MODE_GRAB
            || e->mode == XCB_NOTIFY_MODE_UNGRAB)
        return;
    /* prevent focus stealing */
    if ((e->detail == XCB_NOTIFY_DETAIL_ANCESTOR ||
                e->detail == XCB_NOTIFY_DETAIL_INFERIOR ||
                e->detail == XCB_NOTIFY_DETAIL_NONLINEAR_VIRTUAL ||
                e->detail == XCB_NOTIFY_DETAIL_NONLINEAR) &&
            (mon->desk->focus == NULL
             || mon->desk->focus->client->window != e->event))
        update_input_focus();
}

void expose(xcb_generic_event_t *evt)
{
    xcb_expose_event_t *e = (xcb_expose_event_t *) evt;

    PRINTF("expose %X\n", e->window);

    coordinates_t loc;
    if (locate_window(e->window, &loc) && loc.node->client->frame)
        draw_frame_background(loc.node, loc.desktop->focus == loc.node, loc.monitor == mon);
}

void enter_notify(xcb_generic_event_t *evt)
{
    xcb_enter_notify_event_t *e = (xcb_enter_notify_event_t *) evt;
    xcb_window_t win = e->event;

    PRINTF("enter notify %X %d %d\n", win, e->mode, e->detail);

    if (e->mode != XCB_NOTIFY_MODE_NORMAL
            || (mon->desk->focus != NULL && mon->desk->focus->client->window == win))
        return;

    enable_motion_recorder();
}

void motion_notify(xcb_generic_event_t *evt)
{
    PUTS("motion notify");

    xcb_motion_notify_event_t *e = (xcb_motion_notify_event_t *) evt;

    int dtime = e->time - last_motion_time;
    if (dtime > 1000) {
        last_motion_time = e->time;
        last_motion_x = e->event_x;
        last_motion_y = e->event_y;
        return;
    }

    int mdist = abs(e->event_x - last_motion_x) + abs(e->event_y - last_motion_y);
    if (mdist < 10)
        return;

    disable_motion_recorder();

    xcb_window_t win = XCB_NONE;
    xcb_point_t pt = {e->root_x, e->root_y};
    query_pointer(&win, NULL);

    bool backup = pointer_follows_monitor;
    auto_raise = false;
    pointer_follows_monitor = false;
    if (!window_focus(win)) {
        monitor_t *m = monitor_from_point(pt);
        if (m != NULL && m != mon)
            focus_node(m, m->desk, m->desk->focus);
    }
    pointer_follows_monitor = backup;
    auto_raise = true;
}

void handle_state(monitor_t *m, desktop_t *d, node_t *n, xcb_atom_t state, unsigned int action)
{
    if (state == ewmh->_NET_WM_STATE_FULLSCREEN) {
        if (action == XCB_EWMH_WM_STATE_ADD)
            set_fullscreen(n, true);
        else if (action == XCB_EWMH_WM_STATE_REMOVE)
            set_fullscreen(n, false);
        else if (action == XCB_EWMH_WM_STATE_TOGGLE)
            set_fullscreen(n, !n->client->fullscreen);
        arrange(m, d);
    } else if (state == ewmh->_NET_WM_STATE_STICKY) {
        if (action == XCB_EWMH_WM_STATE_ADD)
            set_sticky(m, d, n, true);
        else if (action == XCB_EWMH_WM_STATE_REMOVE)
            set_sticky(m, d, n, false);
        else if (action == XCB_EWMH_WM_STATE_TOGGLE)
            set_sticky(m, d, n, !n->client->sticky);
    } else if (state == ewmh->_NET_WM_STATE_DEMANDS_ATTENTION) {
        if (action == XCB_EWMH_WM_STATE_ADD)
            set_urgency(m, d, n, true);
        else if (action == XCB_EWMH_WM_STATE_REMOVE)
            set_urgency(m, d, n, false);
        else if (action == XCB_EWMH_WM_STATE_TOGGLE)
            set_urgency(m, d, n, !n->client->urgent);
    }
}
