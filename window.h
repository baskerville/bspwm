#ifndef _WINDOW_H
#define _WINDOW_H

#include <stdarg.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "types.h"

#define MIN_WIDTH  32
#define MIN_HEIGHT 32

bool locate_window(xcb_window_t, window_location_t *);
void draw_triple_border(node_t *, uint32_t);
void close_window(desktop_t *, node_t *);
void toggle_fullscreen(client_t *);
void list_windows(char *);
void window_border_width(xcb_window_t, uint32_t);
void window_move_resize(xcb_window_t, int16_t, int16_t, uint16_t, uint16_t);
void window_raise(xcb_window_t);

#endif
