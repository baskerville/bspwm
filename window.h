/* Copyright (c) 2012, Bastien Dejean
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef BSPWM_WINDOW_H
#define BSPWM_WINDOW_H

#include <stdarg.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_icccm.h>
#include "types.h"

void schedule_window(xcb_window_t win);
void manage_window(xcb_window_t win, rule_consequence_t *csq, int fd);
void unmanage_window(xcb_window_t win);
void window_draw_border(node_t *n, bool focused_window, bool focused_monitor);
pointer_state_t *make_pointer_state(void);
bool contains(xcb_rectangle_t a, xcb_rectangle_t b);
xcb_rectangle_t get_rectangle(monitor_t *m, client_t *c);
void get_side_handle(client_t *c, direction_t dir, xcb_point_t *pt);
void adopt_orphans(void);
void window_close(node_t *n);
void window_kill(monitor_t *m, desktop_t *d, node_t *n);
void set_layer(monitor_t *m, desktop_t *d, node_t *n, stack_layer_t l);
void set_state(monitor_t *m, desktop_t *d, node_t *n, client_state_t s);
void set_floating(monitor_t *m, desktop_t *d, node_t *n, bool value);
void set_fullscreen(monitor_t *m, desktop_t *d, node_t *n, bool value);
void neutralize_obscuring_windows(monitor_t *m, desktop_t *d, node_t *n);
void set_locked(monitor_t *m, desktop_t *d, node_t *n, bool value);
void set_sticky(monitor_t *m, desktop_t *d, node_t *n, bool value);
void set_private(monitor_t *m, desktop_t *d, node_t *n, bool value);
void set_urgency(monitor_t *m, desktop_t *d, node_t *n, bool value);
uint32_t get_border_color(client_t *c, bool focused_window, bool focused_monitor);
void update_floating_rectangle(client_t *c);
void restrain_floating_width(client_t *c, int *width);
void restrain_floating_height(client_t *c, int *height);
void restrain_floating_size(client_t *c, int *width, int *height);
void query_pointer(xcb_window_t *win, xcb_point_t *pt);
void window_border_width(xcb_window_t win, uint32_t bw);
void window_move(xcb_window_t win, int16_t x, int16_t y);
void window_resize(xcb_window_t win, uint16_t w, uint16_t h);
void window_move_resize(xcb_window_t win, int16_t x, int16_t y, uint16_t w, uint16_t h);
void window_raise(xcb_window_t win);
void window_center(monitor_t *m, client_t *c);
void window_stack(xcb_window_t w1, xcb_window_t w2, uint32_t mode);
void window_above(xcb_window_t w1, xcb_window_t w2);
void window_below(xcb_window_t w1, xcb_window_t w2);
void window_lower(xcb_window_t win);
void window_set_visibility(xcb_window_t win, bool visible);
void window_hide(xcb_window_t win);
void window_show(xcb_window_t win);
void toggle_visibility(void);
void enable_motion_recorder(void);
void disable_motion_recorder(void);
void update_motion_recorder(void);
void update_input_focus(void);
void set_input_focus(node_t *n);
void clear_input_focus(void);
void center_pointer(xcb_rectangle_t r);
void get_atom(char *name, xcb_atom_t *atom);
void set_atom(xcb_window_t win, xcb_atom_t atom, uint32_t value);
bool has_proto(xcb_atom_t atom, xcb_icccm_get_wm_protocols_reply_t *protocols);
void send_client_message(xcb_window_t win, xcb_atom_t property, xcb_atom_t value);

#endif
