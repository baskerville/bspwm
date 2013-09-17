#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_icccm.h>
#include "types.h"
#include "tree.h"
#include "bspwm.h"
#include "settings.h"
#include "ewmh.h"
#include "rules.h"
#include "query.h"
#include "window.h"

void center(xcb_rectangle_t a, xcb_rectangle_t *b)
{
    if (b->width < a.width)
        b->x = a.x + (a.width - b->width) / 2;
    if (b->height < a.height)
        b->y = a.y + (a.height - b->height) / 2;
}

bool contains(xcb_rectangle_t a, xcb_rectangle_t b)
{
    return (a.x <= b.x && (a.x + a.width) >= (b.x + b.width)
            && a.y <= b.y && (a.y + a.height) >= (b.y + b.height));
}

bool is_inside(monitor_t *m, xcb_point_t pt)
{
    xcb_rectangle_t r = m->rectangle;
    return (r.x <= pt.x && pt.x < (r.x + r.width)
            && r.y <= pt.y && pt.y < (r.y + r.height));
}

xcb_rectangle_t get_rectangle(client_t *c)
{
    if (is_tiled(c))
        return c->tiled_rectangle;
    else
        return c->floating_rectangle;
}

void get_side_handle(client_t *c, direction_t dir, xcb_point_t *pt)
{
    xcb_rectangle_t rect = get_rectangle(c);
    switch (dir) {
        case DIR_RIGHT:
            pt->x = rect.x + rect.width;
            pt->y = rect.y + (rect.height / 2);
            break;
        case DIR_DOWN:
            pt->x = rect.x + (rect.width / 2);
            pt->y = rect.y + rect.height;
            break;
        case DIR_LEFT:
            pt->x = rect.x;
            pt->y = rect.y + (rect.height / 2);
            break;
        case DIR_UP:
            pt->x = rect.x + (rect.width / 2);
            pt->y = rect.y;
            break;
    }
}

monitor_t *monitor_from_point(xcb_point_t pt)
{
    for (monitor_t *m = mon_head; m != NULL; m = m->next)
        if (is_inside(m, pt))
            return m;
    return NULL;
}

monitor_t *underlying_monitor(client_t *c)
{
    xcb_point_t pt = (xcb_point_t) {c->floating_rectangle.x, c->floating_rectangle.y};
    return monitor_from_point(pt);
}

void manage_window(monitor_t *m, desktop_t *d, xcb_window_t win)
{
    coordinates_t loc;
    xcb_get_window_attributes_reply_t *wa = xcb_get_window_attributes_reply(dpy, xcb_get_window_attributes(dpy, win), NULL);
    uint8_t override_redirect = 0;

    if (wa != NULL) {
        override_redirect = wa->override_redirect;
        free(wa);
    }

    if (override_redirect || locate_window(win, &loc))
        return;

    bool floating = false, follow = false, transient = false, fullscreen = false, takes_focus = true, manage = true;

    handle_rules(win, &m, &d, &floating, &follow, &transient, &fullscreen, &takes_focus, &manage);

    if (!manage) {
        disable_floating_atom(win);
        window_show(win);
        return;
    }

    client_t *c = make_client(win);
    update_floating_rectangle(c);

    xcb_icccm_get_wm_class_reply_t reply;
    if (xcb_icccm_get_wm_class_reply(dpy, xcb_icccm_get_wm_class(dpy, win), &reply, NULL) == 1) {
        strncpy(c->class_name, reply.class_name, sizeof(c->class_name));
        xcb_icccm_get_wm_class_reply_wipe(&reply);
    }

    if (c->transient)
        floating = true;

    node_t *n = make_node();
    n->client = c;

    insert_node(m, d, n, d->focus);

    disable_floating_atom(c->window);

    if (floating)
        set_floating(d, n, true);

    if (d->focus != NULL && d->focus->client->fullscreen)
        set_fullscreen(d, d->focus, false);

    if (fullscreen)
        set_fullscreen(d, n, true);

    c->transient = transient;

    bool give_focus = (takes_focus && (d == mon->desk || follow));

    if (give_focus) {
        focus_node(m, d, n);
    } else if (takes_focus) {
        pseudo_focus(d, n);
    } else {
        node_t *f = d->focus;
        pseudo_focus(d, n);
        if (f != NULL)
            pseudo_focus(d, f);
    }

    xcb_rectangle_t *frect = &n->client->floating_rectangle;
    if (frect->x == 0 && frect->y == 0)
        center(m->rectangle, frect);

    fit_monitor(m, n->client);

    arrange(m, d);

    if (d == m->desk && visible)
        window_show(c->window);

    /* the same function is already called in `focus_node` but has no effects on unmapped windows */
    if (give_focus)
        xcb_set_input_focus(dpy, XCB_INPUT_FOCUS_POINTER_ROOT, win, XCB_CURRENT_TIME);

    uint32_t values[] = {(focus_follows_pointer ? CLIENT_EVENT_MASK_FFP : CLIENT_EVENT_MASK)};
    xcb_change_window_attributes(dpy, c->window, XCB_CW_EVENT_MASK, values);

    num_clients++;
    ewmh_set_wm_desktop(n, d);
    ewmh_update_client_list();
}

void adopt_orphans(void)
{
    xcb_query_tree_reply_t *qtr = xcb_query_tree_reply(dpy, xcb_query_tree(dpy, root), NULL);
    if (qtr == NULL)
        return;

    PUTS("adopt orphans");

    int len = xcb_query_tree_children_length(qtr);
    xcb_window_t *wins = xcb_query_tree_children(qtr);
    for (int i = 0; i < len; i++) {
        uint32_t idx;
        xcb_window_t win = wins[i];
        window_hide(win);
        if (xcb_ewmh_get_wm_desktop_reply(ewmh, xcb_ewmh_get_wm_desktop(ewmh, win), &idx, NULL) == 1) {
            coordinates_t loc;
            if (ewmh_locate_desktop(idx, &loc))
                manage_window(loc.monitor, loc.desktop, win);
            else
                manage_window(mon, mon->desk, win);
        }
    }
    free(qtr);
}

void window_draw_border(node_t *n, bool focused_window, bool focused_monitor)
{
    if (n == NULL || border_width < 1 || n->client->border_width < 1)
        return;

    xcb_window_t win = n->client->window;
    uint32_t border_color_pxl = get_border_color(n->client, focused_window, focused_monitor);

    if (n->split_mode == MODE_AUTOMATIC) {
        xcb_change_window_attributes(dpy, win, XCB_CW_BORDER_PIXEL, &border_color_pxl);
    } else {
        uint32_t presel_border_color_pxl;
        get_color(presel_border_color, win, &presel_border_color_pxl);

        xcb_rectangle_t actual_rectangle = get_rectangle(n->client);

        uint16_t width = actual_rectangle.width;
        uint16_t height = actual_rectangle.height;

        uint16_t full_width = width + 2 * border_width;
        uint16_t full_height = height + 2 * border_width;

        xcb_rectangle_t border_rectangles[] =
        {
            { width, 0, 2 * border_width, height + 2 * border_width },
            { 0, height, width + 2 * border_width, 2 * border_width }
        };

        xcb_rectangle_t *presel_rectangles;

        uint8_t win_depth = root_depth;
        xcb_get_geometry_reply_t *geo = xcb_get_geometry_reply(dpy, xcb_get_geometry(dpy, win), NULL);
        if (geo != NULL)
            win_depth = geo->depth;
        free(geo);

        xcb_pixmap_t pixmap = xcb_generate_id(dpy);
        xcb_create_pixmap(dpy, win_depth, pixmap, win, full_width, full_height);

        xcb_gcontext_t gc = xcb_generate_id(dpy);
        xcb_create_gc(dpy, gc, pixmap, 0, NULL);

        xcb_change_gc(dpy, gc, XCB_GC_FOREGROUND, &border_color_pxl);
        xcb_poly_fill_rectangle(dpy, pixmap, gc, LENGTH(border_rectangles), border_rectangles);

        uint16_t fence = (int16_t) (n->split_ratio * ((n->split_dir == DIR_UP || n->split_dir == DIR_DOWN) ? height : width));
        presel_rectangles = malloc(2 * sizeof(xcb_rectangle_t));
        switch (n->split_dir) {
            case DIR_UP:
                presel_rectangles[0] = (xcb_rectangle_t) {width, 0, 2 * border_width, fence};
                presel_rectangles[1] = (xcb_rectangle_t) {0, height + border_width, full_width, border_width};
                break;
            case DIR_DOWN:
                presel_rectangles[0] = (xcb_rectangle_t) {width, fence + 1, 2 * border_width, height + border_width - (fence + 1)};
                presel_rectangles[1] = (xcb_rectangle_t) {0, height, full_width, border_width};
                break;
            case DIR_LEFT:
                presel_rectangles[0] = (xcb_rectangle_t) {0, height, fence, 2 * border_width};
                presel_rectangles[1] = (xcb_rectangle_t) {width + border_width, 0, border_width, full_height};
                break;
            case DIR_RIGHT:
                presel_rectangles[0] = (xcb_rectangle_t) {fence + 1, height, width + border_width - (fence + 1), 2 * border_width};
                presel_rectangles[1] = (xcb_rectangle_t) {width, 0, border_width, full_height};
                break;
        }
        xcb_change_gc(dpy, gc, XCB_GC_FOREGROUND, &presel_border_color_pxl);
        xcb_poly_fill_rectangle(dpy, pixmap, gc, 2, presel_rectangles);
        xcb_change_window_attributes(dpy, win, XCB_CW_BORDER_PIXMAP, &pixmap);
        free(presel_rectangles);
        xcb_free_gc(dpy, gc);
        xcb_free_pixmap(dpy, pixmap);
    }
}

void window_close(node_t *n)
{
    if (n == NULL || n->client->locked)
        return;

    PRINTF("close window %X\n", n->client->window);

    send_client_message(n->client->window, ewmh->WM_PROTOCOLS, WM_DELETE_WINDOW);
}

void window_kill(desktop_t *d, node_t *n)
{
    if (n == NULL)
        return;

    xcb_window_t win = n->client->window;
    PRINTF("kill window %X\n", win);

    xcb_kill_client(dpy, win);
    remove_node(d, n);
}

void set_fullscreen(desktop_t *d, node_t *n, bool value)
{
    if (n == NULL || n->client->fullscreen == value)
        return;

    client_t *c = n->client;

    PRINTF("fullscreen %X: %s\n", c->window, BOOLSTR(value));

    if (value) {
        c->fullscreen = true;
        xcb_atom_t values[] = {ewmh->_NET_WM_STATE_FULLSCREEN};
        xcb_ewmh_set_wm_state(ewmh, c->window, LENGTH(values), values);
    } else {
        c->fullscreen = false;
        xcb_atom_t values[] = {XCB_NONE};
        xcb_ewmh_set_wm_state(ewmh, c->window, LENGTH(values), values);
    }

    stack(d, n);
}

void set_floating(desktop_t *d, node_t *n, bool value)
{
    if (n == NULL || n->client->transient || n->client->fullscreen || n->client->floating == value)
        return;

    PRINTF("floating %X: %s\n", n->client->window, BOOLSTR(value));

    n->split_mode = MODE_AUTOMATIC;
    client_t *c = n->client;
    c->floating = n->vacant = value;
    update_vacant_state(n->parent);

    if (value) {
        enable_floating_atom(c->window);
        unrotate_brother(n);
    } else {
        disable_floating_atom(c->window);
        rotate_brother(n);
    }

    stack(d, n);
}

void set_locked(monitor_t *m, desktop_t *d, node_t *n, bool value)
{
    if (n == NULL || n->client->locked == value)
        return;

    client_t *c = n->client;

    PRINTF("set locked %X: %s\n", c->window, BOOLSTR(value));

    c->locked = value;
    window_draw_border(n, d->focus == n, m == mon);
}

void set_urgency(monitor_t *m, desktop_t *d, node_t *n, bool value)
{
    if (value && mon->desk->focus == n)
        return;
    n->client->urgent = value;
    window_draw_border(n, d->focus == n, m == mon);
    put_status();
}

void set_floating_atom(xcb_window_t win, uint32_t value)
{
    if (!apply_floating_atom)
        return;
    set_atom(win, _BSPWM_FLOATING_WINDOW, value);
}

void enable_floating_atom(xcb_window_t win)
{
    set_floating_atom(win, 1);
}

void disable_floating_atom(xcb_window_t win)
{
    set_floating_atom(win, 0);
}

uint32_t get_border_color(client_t *c, bool focused_window, bool focused_monitor)
{
    if (c == NULL)
        return 0;

    uint32_t pxl = 0;

    if (focused_monitor && focused_window) {
        if (c->locked)
            get_color(focused_locked_border_color, c->window, &pxl);
        else
            get_color(focused_border_color, c->window, &pxl);
    } else if (focused_window) {
        if (c->urgent)
            get_color(urgent_border_color, c->window, &pxl);
        else if (c->locked)
            get_color(active_locked_border_color, c->window, &pxl);
        else
            get_color(active_border_color, c->window, &pxl);
    } else {
        if (c->urgent)
            get_color(urgent_border_color, c->window, &pxl);
        else if (c->locked)
            get_color(normal_locked_border_color, c->window, &pxl);
        else
            get_color(normal_border_color, c->window, &pxl);
    }

    return pxl;
}

void update_floating_rectangle(client_t *c)
{
    xcb_get_geometry_reply_t *geo = xcb_get_geometry_reply(dpy, xcb_get_geometry(dpy, c->window), NULL);

    if (geo != NULL)
        c->floating_rectangle = (xcb_rectangle_t) {geo->x, geo->y, geo->width, geo->height};
    else
        c->floating_rectangle = (xcb_rectangle_t) {0, 0, 32, 24};

    free(geo);
}


void query_pointer(xcb_window_t *win, xcb_point_t *pt)
{
    window_lower(motion_recorder);

    xcb_query_pointer_reply_t *qpr = xcb_query_pointer_reply(dpy, xcb_query_pointer(dpy, root), NULL);

    if (qpr != NULL) {
        if (win != NULL)
            *win = qpr->child;
        if (pt != NULL)
            *pt = (xcb_point_t) {qpr->root_x, qpr->root_y};
        free(qpr);
    }

    window_raise(motion_recorder);
}

void window_focus(xcb_window_t win)
{
    coordinates_t loc;
    if (locate_window(win, &loc)) {
        if (loc.node == mon->desk->focus)
            return;
        focus_node(loc.monitor, loc.desktop, loc.node);
    }
}

void window_border_width(xcb_window_t win, uint32_t bw)
{
    uint32_t values[] = {bw};
    xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_BORDER_WIDTH, values);
}

void window_move(xcb_window_t win, int16_t x, int16_t y)
{
    uint32_t values[] = {x, y};
    xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_X_Y, values);
}

void window_resize(xcb_window_t win, uint16_t w, uint16_t h)
{
    uint32_t values[] = {w, h};
    xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_WIDTH_HEIGHT, values);
}

void window_move_resize(xcb_window_t win, int16_t x, int16_t y, uint16_t w, uint16_t h)
{
    uint32_t values[] = {x, y, w, h};
    xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_X_Y_WIDTH_HEIGHT, values);
}

void window_raise(xcb_window_t win)
{
    uint32_t values[] = {XCB_STACK_MODE_ABOVE};
    xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_STACK_MODE, values);
}

void window_stack(xcb_window_t w1, xcb_window_t w2, uint32_t mode)
{
    if (w2 == XCB_NONE)
        return;
    uint16_t mask = XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE;
    uint32_t values[] = {w2, mode};
    xcb_configure_window(dpy, w1, mask, values);
}

void window_above(xcb_window_t w1, xcb_window_t w2)
{
    window_stack(w1, w2, XCB_STACK_MODE_ABOVE);
}

void window_below(xcb_window_t w1, xcb_window_t w2)
{
    window_stack(w1, w2, XCB_STACK_MODE_BELOW);
}

void stack(desktop_t *d, node_t *n)
{
    if (is_leaf(d->root))
        return;
    if (n->client->fullscreen) {
        window_raise(n->client->window);
    } else {
        if (n->client->floating && !auto_raise)
            return;
        xcb_window_t latest_tiled = XCB_NONE;
        xcb_window_t oldest_floating = XCB_NONE;
        for (node_list_t *a = d->history->head; a != NULL; a = a->next) {
            if (a->latest && a->node != n) {
                if (a->node->client->floating == n->client->floating) {
                    window_above(n->client->window, a->node->client->window);
                    return;
                } else if (latest_tiled == XCB_NONE && !a->node->client->floating) {
                    latest_tiled = a->node->client->window;
                } else if (a->node->client->floating) {
                    oldest_floating = a->node->client->window;
                }
            }
        }
        if (n->client->floating)
            window_above(n->client->window, latest_tiled);
        else
            window_below(n->client->window, oldest_floating);
    }
}

void window_lower(xcb_window_t win)
{
    uint32_t values[] = {XCB_STACK_MODE_BELOW};
    xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_STACK_MODE, values);
}

void window_set_visibility(xcb_window_t win, bool visible)
{
    uint32_t values_off[] = {ROOT_EVENT_MASK & ~XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY};
    uint32_t values_on[] = {ROOT_EVENT_MASK};
    xcb_change_window_attributes(dpy, root, XCB_CW_EVENT_MASK, values_off);
    if (visible)
        xcb_map_window(dpy, win);
    else
        xcb_unmap_window(dpy, win);
    xcb_change_window_attributes(dpy, root, XCB_CW_EVENT_MASK, values_on);
}

void window_hide(xcb_window_t win)
{
    window_set_visibility(win, false);
}

void window_show(xcb_window_t win)
{
    window_set_visibility(win, true);
}

void toggle_visibility(void)
{
    visible = !visible;
    if (!visible)
        clear_input_focus();
    for (monitor_t *m = mon_head; m != NULL; m = m->next)
        for (node_t *n = first_extrema(m->desk->root); n != NULL; n = next_leaf(n, m->desk->root))
            window_set_visibility(n->client->window, visible);
    if (visible)
        update_input_focus();
}

void desktop_show(desktop_t *d)
{
    if (!visible)
        return;
    for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root))
        window_show(n->client->window);
}

void desktop_hide(desktop_t *d)
{
    if (!visible)
        return;
    for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root))
        window_hide(n->client->window);
}

void enable_motion_recorder(void)
{
    PUTS("enable motion recorder");
    window_raise(motion_recorder);
    window_show(motion_recorder);
}

void disable_motion_recorder(void)
{
    PUTS("disable motion recorder");
    window_hide(motion_recorder);
}

void update_motion_recorder(void)
{
    xcb_get_geometry_reply_t *geo = xcb_get_geometry_reply(dpy, xcb_get_geometry(dpy, root), NULL);

    if (geo != NULL) {
        window_resize(motion_recorder, geo->width, geo->height);
        PRINTF("update motion recorder size: %ux%u\n", geo->width, geo->height);
    }

    free(geo);
}

void update_input_focus(void)
{
    set_input_focus(mon->desk->focus);
}

void set_input_focus(node_t *n)
{
    if (n == NULL) {
        clear_input_focus();
    } else {
        if (n->client->icccm_focus)
            send_client_message(n->client->window, ewmh->WM_PROTOCOLS, WM_TAKE_FOCUS);
        xcb_set_input_focus(dpy, XCB_INPUT_FOCUS_POINTER_ROOT, n->client->window, XCB_CURRENT_TIME);
    }
}

void clear_input_focus(void)
{
    xcb_set_input_focus(dpy, XCB_INPUT_FOCUS_POINTER_ROOT, root, XCB_CURRENT_TIME);
}

void center_pointer(monitor_t *m)
{
    int16_t cx = m->rectangle.x + m->rectangle.width / 2;
    int16_t cy = m->rectangle.y + m->rectangle.height / 2;
    window_lower(motion_recorder);
    xcb_warp_pointer(dpy, XCB_NONE, root, 0, 0, 0, 0, cx, cy);
    window_raise(motion_recorder);
}

void get_atom(char *name, xcb_atom_t *atom)
{
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(dpy, xcb_intern_atom(dpy, 0, strlen(name), name), NULL);
    if (reply != NULL)
        *atom = reply->atom;
    else
        *atom = XCB_NONE;
    free(reply);
}

void set_atom(xcb_window_t win, xcb_atom_t atom, uint32_t value)
{
    xcb_change_property(dpy, XCB_PROP_MODE_REPLACE, win, atom, XCB_ATOM_CARDINAL, 32, 1, &value);
}

bool has_proto(xcb_atom_t atom, xcb_icccm_get_wm_protocols_reply_t *protocols)
{
    for (uint32_t i = 0; i < protocols->atoms_len; i++)
        if (protocols->atoms[i] == atom)
            return true;
    return false;
}

void send_client_message(xcb_window_t win, xcb_atom_t property, xcb_atom_t value)
{
    xcb_client_message_event_t e;

    e.response_type = XCB_CLIENT_MESSAGE;
    e.window = win;
    e.format = 32;
    e.sequence = 0;
    e.type = property;
    e.data.data32[0] = value;
    e.data.data32[1] = XCB_CURRENT_TIME;

    xcb_send_event(dpy, false, win, XCB_EVENT_MASK_NO_EVENT, (char *) &e);
}
