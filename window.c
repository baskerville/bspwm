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
#include <string.h>
#include "bspwm.h"
#include "ewmh.h"
#include "monitor.h"
#include "query.h"
#include "rule.h"
#include "settings.h"
#include "stack.h"
#include "tree.h"
#include "window.h"

void schedule_window(xcb_window_t win)
{
    coordinates_t loc;
    uint8_t override_redirect = 0;
    xcb_get_window_attributes_reply_t *wa = xcb_get_window_attributes_reply(dpy, xcb_get_window_attributes(dpy, win), NULL);

    if (wa != NULL) {
        override_redirect = wa->override_redirect;
        free(wa);
    }

    if (override_redirect || locate_window(win, &loc))
        return;

    /* Ignore pending windows */
    for (pending_rule_t *pr = pending_rule_head; pr != NULL; pr = pr->next) {
        if (pr->win == win)
            return;
    }

    rule_consequence_t *csq = make_rule_conquence();
    apply_rules(win, csq);
    if (!schedule_rules(win, csq)) {
        manage_window(win, csq, -1);
        free(csq);
    }
}

void manage_window(xcb_window_t win, rule_consequence_t *csq, int fd)
{
    monitor_t *m = mon;
    desktop_t *d = mon->desk;

    parse_rule_consequence(fd, csq, &m, &d);

    if (csq->lower)
        window_lower(win);

    if (!csq->manage) {
        disable_floating_atom(win);
        window_show(win);
        return;
    }

    PRINTF("manage %X\n", win);

    client_t *c = make_client(win);
    update_floating_rectangle(c);
    translate_client(monitor_from_client(c), m, c);
    if (csq->center)
        window_center(m, c);
    c->frame = csq->frame;

    xcb_icccm_get_wm_class_reply_t reply;
    if (xcb_icccm_get_wm_class_reply(dpy, xcb_icccm_get_wm_class(dpy, win), &reply, NULL) == 1) {
        snprintf(c->class_name, sizeof(c->class_name), "%s", reply.class_name);
        xcb_icccm_get_wm_class_reply_wipe(&reply);
    }

    csq->floating = csq->floating || d->floating;

    node_t *n = make_node();
    n->client = c;

    insert_node(m, d, n, d->focus);

    disable_floating_atom(c->window);
    set_floating(n, csq->floating);
    set_locked(m, d, n, csq->locked);
    set_sticky(m, d, n, csq->sticky);
    set_private(m, d, n, csq->private);

    if (d->focus != NULL && d->focus->client->fullscreen)
        set_fullscreen(d->focus, false);

    set_fullscreen(n, csq->fullscreen);
    c->transient = csq->transient;

    arrange(m, d);

    bool give_focus = (csq->focus && (d == mon->desk || csq->follow));

    if (give_focus)
        focus_node(m, d, n);
    else if (csq->focus)
        pseudo_focus(d, n);
    else
        stack(n, STACK_ABOVE);

    uint32_t values[] = {get_event_mask(n->client)};
    xcb_change_window_attributes(dpy, c->window, XCB_CW_EVENT_MASK, values);

    if (visible) {
        if (d == m->desk)
            window_show(n->client->window);
        else
            window_hide(n->client->window);
    }

    /* the same function is already called in `focus_node` but has no effects on unmapped windows */
    if (give_focus)
        xcb_set_input_focus(dpy, XCB_INPUT_FOCUS_POINTER_ROOT, win, XCB_CURRENT_TIME);

    num_clients++;
    ewmh_set_wm_desktop(n, d);
    ewmh_update_client_list();
}

void unmanage_window(xcb_window_t win)
{
    coordinates_t loc;
    if (locate_window(win, &loc)) {
        PRINTF("unmanage %X\n", win);
        remove_node(loc.monitor, loc.desktop, loc.node);
        arrange(loc.monitor, loc.desktop);
    } else {
        for (pending_rule_t *pr = pending_rule_head; pr != NULL; pr = pr->next) {
            if (pr->win == win) {
                remove_pending_rule(pr);
                return;
            }
        }
    }
}

void window_draw_border(node_t *n, bool focused_window, bool focused_monitor)
{
    if (n == NULL || (n->client->border_width < 1 && !n->client->frame)) {
        return;
    }

    if (n->client->frame) {
        draw_frame_background(n, focused_window, focused_monitor);
        return;
    }

    xcb_window_t win = n->client->window;
    uint32_t border_color_pxl = get_border_color(n->client, focused_window, focused_monitor);

    if (n->split_mode == MODE_AUTOMATIC) {
        xcb_change_window_attributes(dpy, win, XCB_CW_BORDER_PIXEL, &border_color_pxl);
    } else {
        unsigned int border_width = n->client->border_width;
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

void draw_frame_background(node_t *n, bool focused_window, bool focused_monitor)
{
    if (n == NULL)
        return;

    xcb_window_t win = n->client->window;
    uint32_t border_color_pxl = get_border_color(n->client, focused_window, focused_monitor);
    uint32_t opacity = (focused_window ? (focused_monitor ? focused_frame_opacity : active_frame_opacity) : normal_frame_opacity) * 0xffffffff;
    xcb_change_property(dpy, XCB_PROP_MODE_REPLACE, win, _NET_WM_WINDOW_OPACITY, XCB_ATOM_CARDINAL, 32, 1, &opacity);
    uint8_t win_depth = root_depth;
    xcb_get_geometry_reply_t *geo = xcb_get_geometry_reply(dpy, xcb_get_geometry(dpy, win), NULL);
    if (geo != NULL)
        win_depth = geo->depth;
    free(geo);
    xcb_rectangle_t rectangle = get_rectangle(n->client);
    rectangle.x = rectangle.y = 0;
    uint16_t width = rectangle.width;
    uint16_t height = rectangle.height;
    xcb_pixmap_t pixmap = xcb_generate_id(dpy);
    xcb_create_pixmap(dpy, win_depth, pixmap, win, width, height);
    xcb_gcontext_t gc = xcb_generate_id(dpy);
    xcb_create_gc(dpy, gc, pixmap, 0, NULL);
    xcb_change_gc(dpy, gc, XCB_GC_FOREGROUND, &border_color_pxl);
    xcb_poly_fill_rectangle(dpy, pixmap, gc, 1, &rectangle);
    if (n->split_mode == MODE_MANUAL) {
        xcb_rectangle_t presel_rectangle = rectangle;
        uint32_t presel_border_color_pxl;
        get_color(presel_border_color, win, &presel_border_color_pxl);
        xcb_change_gc(dpy, gc, XCB_GC_FOREGROUND, &presel_border_color_pxl);
        switch (n->split_dir) {
            case DIR_UP:
                presel_rectangle.height = n->split_ratio * rectangle.height;
                break;
            case DIR_RIGHT:
                presel_rectangle.width = (1 - n->split_ratio) * rectangle.width;
                presel_rectangle.x = rectangle.width - presel_rectangle.width;
                break;
            case DIR_DOWN:
                presel_rectangle.height = (1 - n->split_ratio) * rectangle.height;
                presel_rectangle.y = rectangle.height - presel_rectangle.height;
                break;
            case DIR_LEFT:
                presel_rectangle.width = n->split_ratio * rectangle.width;
                break;
        }
        xcb_poly_fill_rectangle(dpy, pixmap, gc, 1, &presel_rectangle);
    }
    xcb_copy_area(dpy, pixmap, win, gc, 0, 0, 0, 0, width, height);
    xcb_free_gc(dpy, gc);
    xcb_free_pixmap(dpy, pixmap);
}

pointer_state_t *make_pointer_state(void)
{
    pointer_state_t *p = malloc(sizeof(pointer_state_t));
    p->monitor = NULL;
    p->desktop = NULL;
    p->node = p->vertical_fence = p->horizontal_fence = NULL;
    p->client = NULL;
    p->window = XCB_NONE;
    p->action = ACTION_NONE;
    return p;
}

bool contains(xcb_rectangle_t a, xcb_rectangle_t b)
{
    return (a.x <= b.x && (a.x + a.width) >= (b.x + b.width)
            && a.y <= b.y && (a.y + a.height) >= (b.y + b.height));
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
        if (xcb_ewmh_get_wm_desktop_reply(ewmh, xcb_ewmh_get_wm_desktop(ewmh, win), &idx, NULL) == 1)
            schedule_window(win);
    }

    free(qtr);
}

void window_close(node_t *n)
{
    if (n == NULL || n->client->locked)
        return;

    PRINTF("close window %X\n", n->client->window);

    send_client_message(n->client->window, ewmh->WM_PROTOCOLS, WM_DELETE_WINDOW);
}

void window_kill(monitor_t *m, desktop_t *d, node_t *n)
{
    if (n == NULL)
        return;

    xcb_window_t win = n->client->window;
    PRINTF("kill window %X\n", win);

    xcb_kill_client(dpy, win);
    remove_node(m, d, n);
}

void set_fullscreen(node_t *n, bool value)
{
    if (n == NULL || n->client->frame || n->client->fullscreen == value)
        return;

    client_t *c = n->client;

    PRINTF("fullscreen %X: %s\n", c->window, BOOLSTR(value));

    c->fullscreen = value;
    if (value)
        ewmh_wm_state_add(c, ewmh->_NET_WM_STATE_FULLSCREEN);
    else
        ewmh_wm_state_remove(c, ewmh->_NET_WM_STATE_FULLSCREEN);
    stack(n, STACK_ABOVE);
}

void set_floating(node_t *n, bool value)
{
    if (n == NULL || n->client->transient || n->client->fullscreen || n->client->frame || n->client->floating == value)
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

    stack(n, STACK_ABOVE);
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

void set_sticky(monitor_t *m, desktop_t *d, node_t *n, bool value)
{
    if (n == NULL || n->client->sticky == value)
        return;

    client_t *c = n->client;

    PRINTF("set sticky %X: %s\n", c->window, BOOLSTR(value));

    if (d != m->desk)
        transfer_node(m, d, n, m, m->desk, m->desk->focus);

    c->sticky = value;
    if (value) {
        ewmh_wm_state_add(c, ewmh->_NET_WM_STATE_STICKY);
        m->num_sticky++;
    } else {
        ewmh_wm_state_remove(c, ewmh->_NET_WM_STATE_STICKY);
        m->num_sticky--;
    }

    window_draw_border(n, d->focus == n, m == mon);
}

void set_private(monitor_t *m, desktop_t *d, node_t *n, bool value)
{
    if (n == NULL || n->client->private == value)
        return;

    client_t *c = n->client;

    PRINTF("set private %X: %s\n", c->window, BOOLSTR(value));

    c->private = value;
    update_privacy_level(n, value);
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
        else if (c->sticky)
            get_color(focused_sticky_border_color, c->window, &pxl);
        else if (c->private)
            get_color(focused_private_border_color, c->window, &pxl);
        else
            get_color(focused_border_color, c->window, &pxl);
    } else if (focused_window) {
        if (c->urgent)
            get_color(urgent_border_color, c->window, &pxl);
        else if (c->locked)
            get_color(active_locked_border_color, c->window, &pxl);
        else if (c->sticky)
            get_color(active_sticky_border_color, c->window, &pxl);
        else if (c->private)
            get_color(active_private_border_color, c->window, &pxl);
        else
            get_color(active_border_color, c->window, &pxl);
    } else {
        if (c->urgent)
            get_color(urgent_border_color, c->window, &pxl);
        else if (c->locked)
            get_color(normal_locked_border_color, c->window, &pxl);
        else if (c->sticky)
            get_color(normal_sticky_border_color, c->window, &pxl);
        else if (c->private)
            get_color(normal_private_border_color, c->window, &pxl);
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

bool window_focus(xcb_window_t win)
{
    coordinates_t loc;
    if (locate_window(win, &loc)) {
        if (loc.node != mon->desk->focus)
            focus_node(loc.monitor, loc.desktop, loc.node);
        return true;
    }
    return false;
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

void window_center(monitor_t *m, client_t *c)
{
    xcb_rectangle_t *r = &c->floating_rectangle;
    xcb_rectangle_t a = m->rectangle;
    if (r->width >= a.width)
        r->x = a.x;
    else
        r->x = a.x + (a.width - r->width) / 2;
    if (r->height >= a.height)
        r->y = a.y;
    else
        r->y = a.y + (a.height - r->height) / 2;
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
    PRINTF("window hide %X\n", win);
    window_set_visibility(win, false);
}

void window_show(xcb_window_t win)
{
    PRINTF("window show %X\n", win);
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

uint32_t get_event_mask(client_t *c)
{
    return CLIENT_EVENT_MASK | (c->frame ? XCB_EVENT_MASK_EXPOSURE : 0) | (focus_follows_pointer ? XCB_EVENT_MASK_ENTER_WINDOW : 0);
}
