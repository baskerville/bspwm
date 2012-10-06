#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "types.h"
#include "tree.h"
#include "bspwm.h"
#include "settings.h"
#include "ewmh.h"
#include "window.h"

#define p_clear(p, count)       ((void)memset((p), 0, sizeof(*(p)) * (count)))

bool locate_window(xcb_window_t win, window_location_t *loc)
{
    node_t *n;
    desktop_t *d = desk_head;

    if (d == NULL)
        return false;

    while (d != NULL) {
        n = first_extrema(d->root);
        while (n != NULL) {
            if (n->client->window == win) {
                loc->desktop = d;
                loc->node = n;
                return true;
            }
            n = next_leaf(n);
        }
        d = d->next;
    }

    return false;
}

void window_draw_border(node_t *n, bool focused)
{
    if (n == NULL)
        return;

    if (border_width < 1 || n->client->border_width < 1)
        return;

    xcb_window_t win = n->client->window;

    xcb_rectangle_t actual_rectangle = (is_tiled(n->client) ? n->client->tiled_rectangle : n->client->floating_rectangle);

    uint16_t width = actual_rectangle.width;
    uint16_t height = actual_rectangle.height;

    uint16_t full_width = width + 2 * border_width;
    uint16_t full_height = height + 2 * border_width;

    xcb_rectangle_t inner_rectangles[] =
    {
        { width, 0, 2 * border_width, height + 2 * border_width },
        { 0, height, width + 2 * border_width, 2 * border_width }
    };

    xcb_rectangle_t main_rectangles[] =
    {
        { width + inner_border_width, 0, 2 * (main_border_width + outer_border_width), height + 2 * border_width },
        { 0, height + inner_border_width, width + 2 * border_width, 2 * (main_border_width + outer_border_width) }
    };

    xcb_rectangle_t outer_rectangles[] =
    {
        { width + inner_border_width + main_border_width, 0, 2 * outer_border_width, height + 2 * border_width },
        { 0, height + inner_border_width + main_border_width, width + 2 * border_width, 2 * outer_border_width }
    };

    xcb_rectangle_t *presel_rectangles;

    xcb_pixmap_t pix = xcb_generate_id(dpy);
    xcb_create_pixmap(dpy, root_depth, pix, win, full_width, full_height);

    xcb_gcontext_t gc = xcb_generate_id(dpy);
    xcb_create_gc(dpy, gc, pix, 0, NULL);

    uint32_t main_border_color_pxl = get_main_border_color(n->client, focused);

    /* inner border */
    if (inner_border_width > 0) {
        xcb_change_gc(dpy, gc, XCB_GC_FOREGROUND, &inner_border_color_pxl);
        xcb_poly_fill_rectangle(dpy, pix, gc, LENGTH(inner_rectangles), inner_rectangles);
    }

    /* main border */
    if (main_border_width > 0) {
        xcb_change_gc(dpy, gc, XCB_GC_FOREGROUND, &main_border_color_pxl);
        xcb_poly_fill_rectangle(dpy, pix, gc, LENGTH(main_rectangles), main_rectangles);
    }

    /* outer border */
    if (outer_border_width > 0) {
        xcb_change_gc(dpy, gc, XCB_GC_FOREGROUND, &outer_border_color_pxl);
        xcb_poly_fill_rectangle(dpy, pix, gc, LENGTH(outer_rectangles), outer_rectangles);
    }

    if (split_mode == MODE_MANUAL && focused) {
        uint16_t fence = (int16_t) (n->split_ratio * ((split_dir == DIR_UP || split_dir == DIR_DOWN) ? height : width));
        presel_rectangles = malloc(2 * sizeof(xcb_rectangle_t));
        switch (split_dir) {
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
        xcb_poly_fill_rectangle(dpy, pix, gc, 2, presel_rectangles);
        free(presel_rectangles);
    }

    /* apply border pixmap */
    xcb_change_window_attributes(dpy, win, XCB_CW_BORDER_PIXMAP, &pix);

    xcb_free_gc(dpy, gc);
    xcb_free_pixmap(dpy, pix);
}

void window_close(node_t *n)
{
    if (n == NULL || n->client->locked)
        return;

    PRINTF("close window %X\n", n->client->window);

    xcb_atom_t WM_DELETE_WINDOW;
    xcb_window_t win = n->client->window;
    xcb_client_message_event_t e;

    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(dpy, xcb_intern_atom(dpy, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW"), NULL);
    if (reply) {
        WM_DELETE_WINDOW = reply->atom;
        free(reply);
    } else {
        warn("close_window %X: could not acquire WM_DELETE_WINDOW atom\n", win);
        return;
    }

    e.response_type = XCB_CLIENT_MESSAGE;
    e.window = win;
    e.format = 32;
    e.sequence = 0;
    e.type = ewmh->WM_PROTOCOLS;
    e.data.data32[0] = WM_DELETE_WINDOW;
    e.data.data32[1] = XCB_CURRENT_TIME;

    xcb_send_event(dpy, false, win, XCB_EVENT_MASK_NO_EVENT, (char *) &e);
}

void window_kill(desktop_t *d, node_t *n)
{
    if (n == NULL)
        return;

    PRINTF("kill window %X\n", n->client->window);

    xcb_kill_client(dpy, n->client->window);
    remove_node(d, n);
}

void toggle_fullscreen(client_t *c)
{
    PRINTF("toggle fullscreen %X\n", c->window);

    if (c->fullscreen) {
        c->fullscreen = false;
        xcb_atom_t values[] = {XCB_NONE};
        xcb_ewmh_set_wm_state(ewmh, c->window, LENGTH(values), values);
    } else {
        c->fullscreen = true;
        xcb_atom_t values[] = {ewmh->_NET_WM_STATE_FULLSCREEN};
        xcb_ewmh_set_wm_state(ewmh, c->window, LENGTH(values), values);
        window_raise(c->window);
        window_border_width(c->window, 0);
        window_move_resize(c->window, 0, 0, screen_width, screen_height);
    }
}

void toggle_floating(node_t *n)
{
    if (n == NULL || n->client->transient)
        return;

    PRINTF("toggle floating %X\n", c->window);

    client_t *c = n->client;
    c->floating = !c->floating;
    n->vacant = !n->vacant;
    update_vacant_state(n->parent);
    if (c->floating)
        window_raise(c->window);
    else if (is_tiled(c))
        window_lower(c->window);
}

void toggle_locked(client_t *c)
{
    PRINTF("toggle locked %X\n", c->window);

    c->locked = !c->locked;
}

void list_windows(char *rsp)
{
    char line[MAXLEN];

    desktop_t *d = desk_head;

    while (d != NULL) {
        node_t *n = first_extrema(d->root);
        while (n != NULL) {
            snprintf(line, sizeof(line), "0x%X\n", n->client->window);
            strncat(rsp, line, REMLEN(rsp));
            n = next_leaf(n);
        }
        d = d->next;
    }
}

uint32_t get_main_border_color(client_t *c, bool focused)
{
    if (c == NULL)
        return 0;

    if (focused) {
        if (c->locked)
            return active_locked_border_color_pxl;
        else
            return active_border_color_pxl;
    } else {
        if (c->urgent)
            return urgent_border_color_pxl;
        else if (c->locked)
            return normal_locked_border_color_pxl;
        else
            return normal_border_color_pxl;
    }
}

void update_floating_rectangle(client_t *c)
{
    xcb_get_geometry_reply_t *geom = xcb_get_geometry_reply(dpy, xcb_get_geometry(dpy, c->window), NULL);

    if (geom) {
        c->floating_rectangle = (xcb_rectangle_t) {geom->x, geom->y, geom->width, geom->height};
        free(geom);
    } else {
        c->floating_rectangle = (xcb_rectangle_t) {0, 0, 1, 1};
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

void window_lower(xcb_window_t win)
{
    uint32_t values[] = {XCB_STACK_MODE_BELOW};
    xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_STACK_MODE, values);
}

void window_set_visibility(xcb_window_t win, bool visible) {
    uint32_t values_off[] = {ROOT_EVENT_MASK & ~XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY};
    uint32_t values_on[] = {ROOT_EVENT_MASK};
    xcb_change_window_attributes(dpy, screen->root, XCB_CW_EVENT_MASK, values_off);
    if (visible)
        xcb_map_window(dpy, win);
    else
        xcb_unmap_window(dpy, win);
    xcb_change_window_attributes(dpy, screen->root, XCB_CW_EVENT_MASK, values_on);
}

void window_hide(xcb_window_t win)
{
    window_set_visibility(win, false);
}

void window_show(xcb_window_t win)
{
    window_set_visibility(win, true);
}
