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

#ifndef BSPWM_POINTER_H
#define BSPWM_POINTER_H

#define XK_Num_Lock     0xff7f
#define XK_Caps_Lock    0xffe5
#define XK_Scroll_Lock  0xff14

extern uint16_t num_lock;
extern uint16_t caps_lock;
extern uint16_t scroll_lock;

extern bool grabbing;
extern node_t *grabbed_node;

void pointer_init(void);
void window_grab_buttons(xcb_window_t win);
void window_grab_button(xcb_window_t win, uint8_t button, uint16_t modifier);
void grab_buttons(void);
void ungrab_buttons(void);
int16_t modfield_from_keysym(xcb_keysym_t keysym);
resize_handle_t get_handle(node_t *n, xcb_point_t pos, pointer_action_t pac);
bool grab_pointer(pointer_action_t pac);
void track_pointer(coordinates_t loc, pointer_action_t pac, xcb_point_t pos);

#endif
