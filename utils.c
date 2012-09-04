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

node_t *win_to_node(xcb_window_t win)
{
    node_t *n;
    desktop_t *d = desk_head;

    if (d == NULL)
        return NULL;

    while (d != NULL) {
        n = first_extrema(d->root);
        while (n != NULL) {
            if (n->client->window == win)
                return n;
            n = next_leaf(n);
        }
        d = d->next;
    }

    return NULL;
}

uint32_t color_pixel(char *hex)
{
    char strgroups[3][3]  = {{hex[1], hex[2], '\0'}, {hex[3], hex[4], '\0'}, {hex[5], hex[6], '\0'}};
    uint16_t rgb16[3] = {(strtol(strgroups[0], NULL, 16)), (strtol(strgroups[1], NULL, 16)), (strtol(strgroups[2], NULL, 16))};
    return (rgb16[0] << 16) + (rgb16[1] << 8) + rgb16[2];
}

uint32_t get_color(char *col)
{
    xcb_colormap_t map = screen->default_colormap;
    xcb_alloc_color_reply_t *rpl;
    xcb_alloc_named_color_reply_t *rpln;
    uint32_t rgb, pxl;
    uint16_t r, g, b;

    if (col[0] == '#') {
        rgb = color_pixel(col);
        r = rgb >> 16;
        g = rgb >> 8 & 0xFF;
        b = rgb & 0xFF;
        rpl = xcb_alloc_color_reply(dpy, xcb_alloc_color(dpy, map, r * 257, g * 257, b * 257), NULL);
        if (rpl != NULL) {
            pxl = rpl->pixel;
            free(rpl);
        }
    } else {
        rpln = xcb_alloc_named_color_reply(dpy, xcb_alloc_named_color(dpy, map, strlen(col), col), NULL);
        if (rpln != NULL) {
            pxl = rpln->pixel;
            free(rpln);
        }
    }

    /* if (!rpl) */
    /*     die("error: cannot allocate color '%s'\n", col); */

    return pxl;
}

void draw_triple_border(node_t *n, uint32_t main_border_color_pxl)
{
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
        printf("%i\n", split_pos);
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
