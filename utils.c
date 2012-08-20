#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "helpers.h"
#include "types.h"
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
    uint32_t rgb, pxl;
    uint16_t r, g, b;

    rgb = color_pixel(col);
    r = rgb >> 16;
    g = rgb >> 8 & 0xFF;
    b = rgb & 0xFF;
    rpl = xcb_alloc_color_reply(dpy, xcb_alloc_color(dpy, map, r * 257, g * 257, b * 257), NULL);

    if (!rpl)
        die("error: cannot allocate color '%s'\n", col);

    pxl = rpl->pixel;
    free(rpl);
    return pxl;
}

void draw_triple_border(xcb_connection_t *dpy, xcb_window_t win)
{
    xcb_get_geometry_reply_t *geom = xcb_get_geometry_reply(dpy, xcb_get_geometry(dpy, win), NULL);

    int width = geom->width;
    int height = geom->height;

    unsigned int depth = geom->depth;

    int full_width = width + 2 * border_width;
    int full_height = height + 2 * border_width;

    xcb_rectangle_t inner_rectangles[] =
    {
        { width, 0, 2 * border_width, height + 2 * border_width },
        { 0, height, width + 2 * border_width, 2 * border_width },
    };

    xcb_rectangle_t main_rectangles[] =
    {
        { width + inner_border_width, 0, 2 * (main_border_width + outer_border_width), height + 2 * border_width },
        { 0, height + inner_border_width, width + 2 * border_width, 2 * (main_border_width + outer_border_width) },
    };

    xcb_rectangle_t outer_rectangles[] =
    {
        { width + inner_border_width + main_border_width, 0, 2 * outer_border_width, height + 2 * border_width },
        { 0, height + inner_border_width + main_border_width, width + 2 * border_width, 2 * outer_border_width },
    };

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
        xcb_change_gc(dpy, gc, XCB_GC_FOREGROUND, &normal_border_color_pxl);
        xcb_poly_fill_rectangle(dpy, pix, gc, LENGTH(main_rectangles), main_rectangles);
    }

    /* outer border */
    if (outer_border_width > 0) {
        xcb_change_gc(dpy, gc, XCB_GC_FOREGROUND, &outer_border_color_pxl);
        xcb_poly_fill_rectangle(dpy, pix, gc, LENGTH(outer_rectangles), outer_rectangles);
    }

    /* apply border pixmap */
    xcb_change_window_attributes(dpy, win, XCB_CW_BORDER_PIXMAP, &pix);

    free(geom);
    xcb_free_gc(dpy, gc);
    xcb_free_pixmap(dpy, pix);
}
