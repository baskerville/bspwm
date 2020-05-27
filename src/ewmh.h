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

#ifndef BSPWM_EWMH_H
#define BSPWM_EWMH_H

#include <xcb/xcb_ewmh.h>

extern xcb_ewmh_connection_t *ewmh;

void ewmh_init(void);
void ewmh_update_active_window(void);
void ewmh_update_number_of_desktops(void);
uint32_t ewmh_get_desktop_index(desktop_t *d);
bool ewmh_locate_desktop(uint32_t i, coordinates_t *loc);
void ewmh_update_current_desktop(void);
void ewmh_set_wm_desktop(node_t *n, desktop_t *d);
void ewmh_update_wm_desktops(void);
void ewmh_update_desktop_names(void);
void ewmh_update_desktop_viewport(void);
bool ewmh_handle_struts(xcb_window_t win);
void ewmh_update_client_list(bool stacking);
void ewmh_wm_state_update(node_t *n);
void ewmh_set_supporting(xcb_window_t win);

#endif
