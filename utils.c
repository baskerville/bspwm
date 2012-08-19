#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "utils.h"

void die(const char *errstr, ...) {
    va_list ap;

    va_start(ap, errstr);
    vfprintf(stderr, errstr, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
}

void draw_triple_border(xcb_connection_t *dpy, xcb_window_t win, int ibw, int mbw, int obw, uint32_t ibc, uint32_t mbc, uint32_t obc)
{
    int bw = ibw + mbw + obw;
    xcb_get_geometry_reply_t *geom = xcb_get_geometry_reply(dpy, xcb_get_geometry(dpy, win), NULL);

    int width = geom->width;
    int height = geom->height;

    unsigned int depth = geom->depth;

    int full_width = width + 2 * bw;
    int full_height = height + 2 * bw;

    xcb_rectangle_t irectangles[] =
    {
        { width, 0, 2 * bw, height + 2 * bw },
        { 0, height, width + 2 * bw, 2 * bw },
    };

    xcb_rectangle_t mrectangles[] =
    {
        { width + ibw, 0, 2 * (mbw + obw), height + 2 * bw },
        { 0, height + ibw, width + 2 * bw, 2 * (mbw + obw) },
    };

    xcb_rectangle_t orectangles[] =
    {
        { width + ibw + mbw, 0, 2 * obw, height + 2 * bw },
        { 0, height + ibw + mbw, width + 2 * bw, 2 * obw },
    };

    xcb_pixmap_t pix = xcb_generate_id(dpy);
    xcb_create_pixmap(dpy, depth, pix, win, full_width, full_height);
    xcb_gcontext_t gc = xcb_generate_id(dpy);
    xcb_create_gc(dpy, gc, pix, 0, NULL);

    /* inner border */
    xcb_change_gc(dpy, gc, XCB_GC_FOREGROUND, &ibc);
    xcb_poly_fill_rectangle(dpy, pix, gc, LENGTH(irectangles), irectangles);

    /* main border */
    xcb_change_gc(dpy, gc, XCB_GC_FOREGROUND, &mbc);
    xcb_poly_fill_rectangle(dpy, pix, gc, LENGTH(mrectangles), mrectangles);

    /* outer border */
    xcb_change_gc(dpy, gc, XCB_GC_FOREGROUND, &obc);
    xcb_poly_fill_rectangle(dpy, pix, gc, LENGTH(orectangles), orectangles);

    /* apply border pixmap */
    xcb_change_window_attributes(dpy, win, XCB_CW_BORDER_PIXMAP, &pix);

    free(geom);
    xcb_free_gc(dpy, gc);
    xcb_free_pixmap(dpy, pix);
}
