#ifndef _UTILS_H
#define _UTILS_H

#include <stdarg.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>

void die(const char *, ...);
uint32_t color_pixel(char *);
uint32_t get_color(char *);
void draw_triple_border(xcb_connection_t *, xcb_window_t);

#endif
