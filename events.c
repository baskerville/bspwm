#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_icccm.h>
#include "types.h"
#include "bspwm.h"
#include "settings.h"
#include "helpers.h"
#include "window.h"
#include "events.h"
#include "tree.h"
#include "query.h"
#include "rules.h"
#include "ewmh.h"

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

    coordinates_t loc;
    if (locate_window(e->window, &loc)) {
        remove_node(loc.desktop, loc.node);
        arrange(loc.monitor, loc.desktop);
    }
}

void unmap_notify(xcb_generic_event_t *evt)
{
    xcb_unmap_notify_event_t *e = (xcb_unmap_notify_event_t *) evt;

    PRINTF("unmap notify %X\n", e->window);

    coordinates_t loc;
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
        if (loc.node == mon->desk->focus)
            return;
        if (loc.desktop->focus->client->fullscreen && loc.desktop->focus != loc.node) {
            set_fullscreen(loc.desktop, loc.desktop->focus, false);
            arrange(loc.monitor, loc.desktop);
        }
        focus_node(loc.monitor, loc.desktop, loc.node);
    } else if (e->type == ewmh->_NET_WM_DESKTOP) {
        coordinates_t dloc;
        if (ewmh_locate_desktop(e->data.data32[0], &dloc))
            transfer_node(loc.monitor, loc.desktop, dloc.monitor, dloc.desktop, loc.node);
    }
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
    query_pointer(&win, NULL);
    if (win != XCB_NONE) {
        bool backup = pointer_follows_monitor;
        auto_raise = false;
        pointer_follows_monitor = false;
        window_focus(win);
        pointer_follows_monitor = backup;
        auto_raise = true;
    }
}

void handle_state(monitor_t *m, desktop_t *d, node_t *n, xcb_atom_t state, unsigned int action)
{
    if (state == ewmh->_NET_WM_STATE_FULLSCREEN) {
        if (action == XCB_EWMH_WM_STATE_ADD)
            set_fullscreen(d, n, true);
        else if (action == XCB_EWMH_WM_STATE_REMOVE)
            set_fullscreen(d, n, false);
        else if (action == XCB_EWMH_WM_STATE_TOGGLE)
            set_fullscreen(d, n, !n->client->fullscreen);
        arrange(m, d);
    } else if (state == ewmh->_NET_WM_STATE_DEMANDS_ATTENTION) {
        if (action == XCB_EWMH_WM_STATE_ADD)
            set_urgency(m, d, n, true);
        else if (action == XCB_EWMH_WM_STATE_REMOVE)
            set_urgency(m, d, n, false);
        else if (action == XCB_EWMH_WM_STATE_TOGGLE)
            set_urgency(m, d, n, !n->client->urgent);
    }
}

void grab_pointer(pointer_action_t pac)
{
    PRINTF("grab pointer %u\n", pac);

    xcb_window_t win = XCB_NONE;
    xcb_point_t pos;

    query_pointer(&win, &pos);

    if (win == XCB_NONE)
        return;

    coordinates_t loc;
    if (locate_window(win, &loc)) {
        client_t *c = NULL;
        frozen_pointer->position = pos;
        frozen_pointer->action = pac;
        c = loc.node->client;
        frozen_pointer->monitor = loc.monitor;
        frozen_pointer->desktop = loc.desktop;
        frozen_pointer->node = loc.node;
        frozen_pointer->client = c;
        frozen_pointer->window = c->window;
        frozen_pointer->horizontal_fence = NULL;
        frozen_pointer->vertical_fence = NULL;

        switch (pac)  {
            case ACTION_FOCUS:
                if (loc.node != mon->desk->focus) {
                    bool backup = pointer_follows_monitor;
                    pointer_follows_monitor = false;
                    focus_node(loc.monitor, loc.desktop, loc.node);
                    pointer_follows_monitor = backup;
                } else if (focus_follows_pointer && is_floating(loc.node->client)) {
                    stack(loc.desktop, loc.node);
                }
                break;
            case ACTION_MOVE:
            case ACTION_RESIZE_SIDE:
            case ACTION_RESIZE_CORNER:
                if (is_tiled(c)) {
                    frozen_pointer->rectangle = c->tiled_rectangle;
                    frozen_pointer->is_tiled = true;
                } else if (is_floating(c)) {
                    frozen_pointer->rectangle = c->floating_rectangle;
                    frozen_pointer->is_tiled = false;
                } else {
                    frozen_pointer->action = ACTION_NONE;
                    return;
                }
                if (pac == ACTION_RESIZE_SIDE) {
                    float W = frozen_pointer->rectangle.width;
                    float H = frozen_pointer->rectangle.height;
                    float ratio = W / H;
                    float x = pos.x - frozen_pointer->rectangle.x;
                    float y = pos.y - frozen_pointer->rectangle.y;
                    float diag_a = ratio * y;
                    float diag_b = W - diag_a;
                    if (x < diag_a) {
                        if (x < diag_b)
                            frozen_pointer->side = SIDE_LEFT;
                        else
                            frozen_pointer->side = SIDE_BOTTOM;
                    } else {
                        if (x < diag_b)
                            frozen_pointer->side = SIDE_TOP;
                        else
                            frozen_pointer->side = SIDE_RIGHT;
                    }
                } else if (pac == ACTION_RESIZE_CORNER) {
                    int16_t mid_x = frozen_pointer->rectangle.x + (frozen_pointer->rectangle.width / 2);
                    int16_t mid_y = frozen_pointer->rectangle.y + (frozen_pointer->rectangle.height / 2);
                    if (pos.x > mid_x) {
                        if (pos.y > mid_y)
                            frozen_pointer->corner = CORNER_BOTTOM_RIGHT;
                        else
                            frozen_pointer->corner = CORNER_TOP_RIGHT;
                    } else {
                        if (pos.y > mid_y)
                            frozen_pointer->corner = CORNER_BOTTOM_LEFT;
                        else
                            frozen_pointer->corner = CORNER_TOP_LEFT;
                    }
                }
                if (frozen_pointer->is_tiled) {
                    if (pac == ACTION_RESIZE_SIDE) {
                        switch (frozen_pointer->side) {
                            case SIDE_TOP:
                                frozen_pointer->horizontal_fence = find_fence(loc.node, DIR_UP);
                                break;
                            case SIDE_RIGHT:
                                frozen_pointer->vertical_fence = find_fence(loc.node, DIR_RIGHT);
                                break;
                            case SIDE_BOTTOM:
                                frozen_pointer->horizontal_fence = find_fence(loc.node, DIR_DOWN);
                                break;
                            case SIDE_LEFT:
                                frozen_pointer->vertical_fence = find_fence(loc.node, DIR_LEFT);
                                break;
                        }
                    } else if (pac == ACTION_RESIZE_CORNER) {
                        switch (frozen_pointer->corner) {
                            case CORNER_TOP_LEFT:
                                frozen_pointer->horizontal_fence = find_fence(loc.node, DIR_UP);
                                frozen_pointer->vertical_fence = find_fence(loc.node, DIR_LEFT);
                                break;
                            case CORNER_TOP_RIGHT:
                                frozen_pointer->horizontal_fence = find_fence(loc.node, DIR_UP);
                                frozen_pointer->vertical_fence = find_fence(loc.node, DIR_RIGHT);
                                break;
                            case CORNER_BOTTOM_RIGHT:
                                frozen_pointer->horizontal_fence = find_fence(loc.node, DIR_DOWN);
                                frozen_pointer->vertical_fence = find_fence(loc.node, DIR_RIGHT);
                                break;
                            case CORNER_BOTTOM_LEFT:
                                frozen_pointer->horizontal_fence = find_fence(loc.node, DIR_DOWN);
                                frozen_pointer->vertical_fence = find_fence(loc.node, DIR_LEFT);
                                break;
                        }
                    }
                    if (frozen_pointer->horizontal_fence != NULL)
                        frozen_pointer->horizontal_ratio = frozen_pointer->horizontal_fence->split_ratio;
                    if (frozen_pointer->vertical_fence != NULL)
                        frozen_pointer->vertical_ratio = frozen_pointer->vertical_fence->split_ratio;
                }
                break;
            case ACTION_NONE:
                break;
        }
    } else {
        frozen_pointer->action = ACTION_NONE;
    }
}

void track_pointer(int root_x, int root_y)
{
    if (frozen_pointer->action == ACTION_NONE)
        return;

    int16_t delta_x, delta_y, x, y, w, h;
    uint16_t width, height;

    pointer_action_t pac = frozen_pointer->action;
    monitor_t *m = frozen_pointer->monitor;
    desktop_t *d = frozen_pointer->desktop;
    node_t *n = frozen_pointer->node;
    client_t *c = frozen_pointer->client;
    xcb_window_t win = frozen_pointer->window;
    xcb_rectangle_t rect = frozen_pointer->rectangle;
    node_t *vertical_fence = frozen_pointer->vertical_fence;
    node_t *horizontal_fence = frozen_pointer->horizontal_fence;

    x = y = 0;
    w = h = 1;

    delta_x = root_x - frozen_pointer->position.x;
    delta_y = root_y - frozen_pointer->position.y;

    switch (pac) {
        case ACTION_MOVE:
            if (frozen_pointer->is_tiled) {
                xcb_window_t pwin = XCB_NONE;
                query_pointer(&pwin, NULL);
                if (pwin == win)
                    return;
                coordinates_t loc;
                bool is_managed = (pwin == XCB_NONE ? false : locate_window(pwin, &loc));
                if (is_managed && is_tiled(loc.node->client) && loc.monitor == m) {
                    swap_nodes(n, loc.node);
                    arrange(m, d);
                } else {
                    if (is_managed && loc.monitor == m) {
                        return;
                    } else if (!is_managed) {
                        xcb_point_t pt = (xcb_point_t) {root_x, root_y};
                        monitor_t *pmon = monitor_from_point(pt);
                        if (pmon == NULL || pmon == m) {
                            return;
                        } else {
                            loc.monitor = pmon;
                            loc.desktop = pmon->desk;
                        }
                    }
                    transfer_node(m, d, loc.monitor, loc.desktop, n);
                    frozen_pointer->monitor = loc.monitor;
                    frozen_pointer->desktop = loc.desktop;
                }
            } else {
                x = rect.x + delta_x;
                y = rect.y + delta_y;
                window_move(win, x, y);
                c->floating_rectangle.x = x;
                c->floating_rectangle.y = y;
                xcb_point_t pt = (xcb_point_t) {root_x, root_y};
                monitor_t *pmon = monitor_from_point(pt);
                if (pmon == NULL || pmon == m)
                    return;
                transfer_node(m, d, pmon, pmon->desk, n);
                frozen_pointer->monitor = pmon;
                frozen_pointer->desktop = pmon->desk;
            }
            break;
        case ACTION_RESIZE_SIDE:
        case ACTION_RESIZE_CORNER:
            if (frozen_pointer->is_tiled) {
                if (vertical_fence != NULL) {
                    double sr = frozen_pointer->vertical_ratio + (double) delta_x / vertical_fence->rectangle.width;
                    sr = MAX(0, sr);
                    sr = MIN(1, sr);
                    vertical_fence->split_ratio = sr;
                }
                if (horizontal_fence != NULL) {
                    double sr = frozen_pointer->horizontal_ratio + (double) delta_y / horizontal_fence->rectangle.height;
                    sr = MAX(0, sr);
                    sr = MIN(1, sr);
                    horizontal_fence->split_ratio = sr;
                }
                arrange(mon, mon->desk);
            } else {
                if (pac == ACTION_RESIZE_SIDE) {
                    switch (frozen_pointer->side) {
                        case SIDE_TOP:
                            x = rect.x;
                            y = rect.y + delta_y;
                            w = rect.width;
                            h = rect.height - delta_y;
                            break;
                        case SIDE_RIGHT:
                            x = rect.x;
                            y = rect.y;
                            w = rect.width + delta_x;
                            h = rect.height;
                            break;
                        case SIDE_BOTTOM:
                            x = rect.x;
                            y = rect.y;
                            w = rect.width;
                            h = rect.height + delta_y;
                            break;
                        case SIDE_LEFT:
                            x = rect.x + delta_x;
                            y = rect.y;
                            w = rect.width - delta_x;
                            h = rect.height;
                            break;
                    }
                    width = MAX(1, w);
                    height = MAX(1, h);
                    window_move_resize(win, x, y, width, height);
                    c->floating_rectangle = (xcb_rectangle_t) {x, y, width, height};
                    window_draw_border(n, d->focus == n, mon == m);
                } else if (pac == ACTION_RESIZE_CORNER) {
                    switch (frozen_pointer->corner) {
                        case CORNER_TOP_LEFT:
                            x = rect.x + delta_x;
                            y = rect.y + delta_y;
                            w = rect.width - delta_x;
                            h = rect.height - delta_y;
                            break;
                        case CORNER_TOP_RIGHT:
                            x = rect.x;
                            y = rect.y + delta_y;
                            w = rect.width + delta_x;
                            h = rect.height - delta_y;
                            break;
                        case CORNER_BOTTOM_LEFT:
                            x = rect.x + delta_x;
                            y = rect.y;
                            w = rect.width - delta_x;
                            h = rect.height + delta_y;
                            break;
                        case CORNER_BOTTOM_RIGHT:
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
                }
            }
            break;
        case ACTION_FOCUS:
        case ACTION_NONE:
            break;
    }
}
