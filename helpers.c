#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "bspwm.h"
#include "helpers.h"

void warn(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

__attribute__((noreturn))
void err(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
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

double distance(xcb_point_t a, xcb_point_t b)
{
    return hypot(a.x - b.x, a.y - b.y);
}
