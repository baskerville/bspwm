#ifndef _UTILS_H
#define _UTILS_H

#include <stdarg.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "types.h"

void die(const char *, ...);
uint32_t color_pixel(char *);
uint32_t get_color(char *);
window_location_t locate_window(xcb_window_t);
bool is_managed(xcb_window_t);
void draw_triple_border(node_t *, uint32_t);

#endif
