#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "helpers.h"
#include "types.h"
#include "tree.h"
#include "bspwm.h"
#include "settings.h"
#include "utils.h"

void die(const char *errstr, ...) {
    va_list ap;
    va_start(ap, errstr);
    vfprintf(stderr, errstr, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
}

xcb_screen_t *screen_of_display(xcb_connection_t *dpy, int default_screen)
{
    xcb_screen_iterator_t iter;
    for (iter = xcb_setup_roots_iterator(xcb_get_setup(dpy)); iter.rem; --default_screen, xcb_screen_next(&iter))
        if (default_screen == 0)
            return iter.data;
    return NULL;
}

window_location_t locate_window(xcb_window_t w)
{
    node_t *n;
    desktop_t *d = desk_head;
    window_location_t l = {NULL, NULL};

    if (d == NULL)
        return l;

    while (d != NULL) {
        n = first_extrema(d->root);
        while (n != NULL) {
            if (n->client->window == w) {
                l.desktop = d;
                l.node = n;
                return l;
            }
            n = next_leaf(n);
        }
        d = d->next;
    }

    return l;
}

bool is_managed(xcb_window_t w)
{
    window_location_t l = locate_window(w);
    return (l.desktop != NULL && l.node != NULL);
}

uint32_t get_color(char *col)
{
    xcb_colormap_t map = screen->default_colormap;
    uint32_t pxl = 0;

    if (col[0] == '#') {
        unsigned int red, green, blue;
        if (sscanf(col + 1, "%02x%02x%02x", &red, &green, &blue) == 3) {
            /* 2**16 - 1 == 0xffff and 0x101 * 0xij == 0xijij */
            red *= 0x101;
            green *= 0x101;
            blue *= 0x101;
            xcb_alloc_color_reply_t *reply = xcb_alloc_color_reply(dpy, xcb_alloc_color(dpy, map, red, green, blue), NULL);
            if (reply != NULL) {
                pxl = reply->pixel;
                free(reply);
            }
        }
    } else {
        xcb_alloc_named_color_reply_t *reply = xcb_alloc_named_color_reply(dpy, xcb_alloc_named_color(dpy, map, strlen(col), col), NULL);
        if (reply != NULL) {
            pxl = reply->pixel;
            free(reply);
        }
    }

    return pxl;
}

void draw_triple_border(node_t *n, uint32_t main_border_color_pxl)
{
    if (n == NULL)
        return;

    if (border_width < 1)
        return;

    xcb_window_t win = n->client->window;
    xcb_get_geometry_reply_t *geom = xcb_get_geometry_reply(dpy, xcb_get_geometry(dpy, win), NULL);

    uint16_t width = geom->width;
    uint16_t height = geom->height;

    uint8_t depth = geom->depth;

    uint16_t full_width = width + 2 * border_width;
    uint16_t full_height = height + 2 * border_width;

    uint16_t split_pos;

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
    xcb_create_pixmap(dpy, depth, pix, win, full_width, full_height);

    xcb_gcontext_t gc = xcb_generate_id(dpy);
    xcb_create_gc(dpy, gc, pix, 0, NULL);

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

    if (split_mode == MODE_MANUAL) {
        split_pos = (int16_t) n->split_ratio * ((split_dir == DIR_UP || split_dir == DIR_DOWN) ? height : width);
        presel_rectangles = malloc(2 * sizeof(xcb_rectangle_t));
        switch (split_dir) {
            case DIR_UP:
                presel_rectangles[0] = (xcb_rectangle_t) {width, 0, 2 * border_width, split_pos};
                presel_rectangles[1] = (xcb_rectangle_t) {0, height + border_width, full_width, border_width};
                break;
            case DIR_DOWN:
                presel_rectangles[0] = (xcb_rectangle_t) {width, split_pos + 1, 2 * border_width, height + border_width - (split_pos + 1)};
                presel_rectangles[1] = (xcb_rectangle_t) {0, height + border_width, full_width, border_width};
                break;
            case DIR_LEFT:
                presel_rectangles[0] = (xcb_rectangle_t) {0, height, split_pos, 2 * border_width};
                presel_rectangles[1] = (xcb_rectangle_t) {width + border_width, 0, border_width, full_height};
                break;
            case DIR_RIGHT:
                presel_rectangles[0] = (xcb_rectangle_t) {split_pos + 1, height, width + border_width - (split_pos + 1), 2 * border_width};
                presel_rectangles[1] = (xcb_rectangle_t) {width, 0, border_width, full_height};
                break;
        }
        xcb_change_gc(dpy, gc, XCB_GC_FOREGROUND, &presel_border_color_pxl);
        xcb_poly_fill_rectangle(dpy, pix, gc, LENGTH(presel_rectangles), presel_rectangles);
        free(presel_rectangles);
    }


    /* apply border pixmap */
    xcb_change_window_attributes(dpy, win, XCB_CW_BORDER_PIXMAP, &pix);

    free(geom);
    xcb_free_gc(dpy, gc);
    xcb_free_pixmap(dpy, pix);
}

void toggle_fullscreen(client_t *c)
{
    if (c->fullscreen) {
        c->fullscreen = false;
        xcb_rectangle_t rect = c->rectangle;
        window_border_width(c->window, border_width);
        window_move_resize(c->window, rect.x, rect.y, rect.width, rect.height);
    } else {
        c->fullscreen = true;
        xcb_get_geometry_reply_t *geom = xcb_get_geometry_reply(dpy, xcb_get_geometry(dpy, c->window), NULL);
        if (geom != NULL) {
            c->rectangle = (xcb_rectangle_t) {geom->x, geom->y, geom->width, geom->height};
            free(geom);
        }
        window_border_width(c->window, 0);
        window_move_resize(c->window, 0, 0, screen_width, screen_height);
    }
}

void transfer_rectangle(xcb_rectangle_t rect, uint32_t *a)
{
    a[0] = rect.x;
    a[1] = rect.y;
    a[2] = rect.width;
    a[3] = rect.height;
}

void window_border_width(xcb_window_t win, uint32_t bw)
{
    uint32_t values[] = {bw};
    xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_BORDER_WIDTH, values);
}

void window_move_resize(xcb_window_t win, int16_t x, int16_t y, uint16_t w, uint16_t h)
{
    uint32_t values[] = {x, y, w, h};
    xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_X_Y_WIDTH_HEIGHT, values);
}
