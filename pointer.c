#include "bspwm.h"
#include "settings.h"
#include "tree.h"
#include "window.h"
#include "query.h"
#include "pointer.h"

void grab_pointer(pointer_action_t pac)
{
    PRINTF("grab pointer %u\n", pac);

    xcb_window_t win = XCB_NONE;
    xcb_point_t pos;

    query_pointer(&win, &pos);

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
        if (pac == ACTION_FOCUS) {
            monitor_t *m = monitor_from_point(pos);
            if (m != NULL && m != mon)
                focus_node(m, m->desk, m->desk->focus);
        }
        frozen_pointer->action = ACTION_NONE;
    }
}

void track_pointer(int root_x, int root_y)
{
    if (frozen_pointer->action == ACTION_NONE)
        return;

    int16_t delta_x, delta_y, x = 0, y = 0, w = 1, h = 1;
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
