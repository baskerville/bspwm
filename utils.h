#ifndef _UTILS_H
#define _UTILS_H

#include <stdarg.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "types.h"

void die(const char *, ...);
uint32_t color_pixel(char *);
uint32_t get_color(char *);
Node *win_to_node(xcb_window_t);
void draw_triple_border(Node *, uint32_t);

#endif
