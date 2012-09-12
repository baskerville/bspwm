#ifndef _UTILS_H
#define _UTILS_H

#include <stdarg.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "types.h"

void die(const char *, ...);
xcb_screen_t *screen_of_display(xcb_connection_t *, int);
uint32_t color_pixel(char *);
uint32_t get_color(char *);
window_location_t locate_window(xcb_window_t);
bool is_managed(xcb_window_t);
void draw_triple_border(node_t *, uint32_t);
void toggle_fullscreen(client_t *);
void window_border_width(xcb_window_t, uint32_t);
void window_move_resize(xcb_window_t, int16_t, int16_t, uint16_t, uint16_t);

#endif
