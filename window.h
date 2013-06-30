#ifndef _WINDOW_H
#define _WINDOW_H

#include <stdarg.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "types.h"

void center(xcb_rectangle_t, xcb_rectangle_t *);
bool contains(xcb_rectangle_t, xcb_rectangle_t);
bool might_cover(desktop_t *, node_t *);
bool locate_window(xcb_window_t, window_location_t *);
bool locate_desktop(char *, desktop_location_t *);
bool is_inside(monitor_t *, xcb_point_t);
xcb_rectangle_t get_rectangle(client_t *);
void get_side_handle(client_t *, direction_t, xcb_point_t *);
monitor_t *monitor_from_point(xcb_point_t);
monitor_t *underlying_monitor(client_t *);
void manage_window(monitor_t *, desktop_t *, xcb_window_t);
void adopt_orphans(void);
void window_draw_border(node_t *, bool, bool);
uint32_t get_border_color(client_t *, bool, bool);
void update_floating_rectangle(client_t *);
void query_pointer(xcb_window_t *, xcb_point_t *);
void list_windows(char *);
void window_close(node_t *);
void window_kill(monitor_t *, desktop_t *, node_t *);
void toggle_fullscreen(desktop_t *, node_t *);
void toggle_floating(desktop_t *, node_t *);
void toggle_locked(monitor_t *, desktop_t *, node_t *);
void set_urgency(monitor_t *, desktop_t *, node_t *, bool);
void set_shadow(xcb_window_t, uint32_t);
void enable_shadow(xcb_window_t);
void disable_shadow(xcb_window_t);
void window_border_width(xcb_window_t, uint32_t);
void window_move(xcb_window_t, int16_t, int16_t);
void window_resize(xcb_window_t, uint16_t, uint16_t);
void window_move_resize(xcb_window_t, int16_t, int16_t, uint16_t, uint16_t);
void window_focus(xcb_window_t);
void window_raise(xcb_window_t);
void stack_tiled(desktop_t *);
void stack(desktop_t *, node_t *);
void window_lower(xcb_window_t);
void window_set_visibility(xcb_window_t, bool);
void window_hide(xcb_window_t);
void window_show(xcb_window_t);
void toggle_visibility(void);
void desktop_show(desktop_t *);
void desktop_hide(desktop_t *);
void enable_motion_recorder(void);
void disable_motion_recorder(void);
void update_motion_recorder(void);
void update_input_focus(void);
void set_input_focus(node_t *);
void clear_input_focus(void);
void center_pointer(monitor_t *);

#endif
