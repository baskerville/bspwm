#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "bspwm.h"
#include "utils.h"

void die(const char *errstr, ...) {
    va_list ap;
    va_start(ap, errstr);
    vfprintf(stderr, errstr, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
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

void transfer_rectangle(xcb_rectangle_t rect, uint32_t *a)
{
    a[0] = rect.x;
    a[1] = rect.y;
    a[2] = rect.width;
    a[3] = rect.height;
}
