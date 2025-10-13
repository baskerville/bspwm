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

#ifndef BSPWM_EVENTS_H
#define BSPWM_EVENTS_H

#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_ewmh.h>

#define ERROR_CODE_BAD_WINDOW  3

extern uint8_t randr_base;
static const xcb_button_index_t BUTTONS[] = {XCB_BUTTON_INDEX_1, XCB_BUTTON_INDEX_2, XCB_BUTTON_INDEX_3};

void handle_event(xcb_generic_event_t *evt);
void map_request(xcb_generic_event_t *evt);
void configure_request(xcb_generic_event_t *evt);
void configure_notify(xcb_generic_event_t *evt);
void destroy_notify(xcb_generic_event_t *evt);
void unmap_notify(xcb_generic_event_t *evt);
void property_notify(xcb_generic_event_t *evt);
void client_message(xcb_generic_event_t *evt);
void wm_move_resize_node(xcb_client_message_event_t* e, coordinates_t loc);
void wm_move_node(xcb_client_message_event_t* e, coordinates_t loc);
void wm_resize_node(xcb_client_message_event_t* e, coordinates_t loc);
void focus_in(xcb_generic_event_t *evt);
void button_press(xcb_generic_event_t *evt);
void enter_notify(xcb_generic_event_t *evt);
void motion_notify(xcb_generic_event_t *evt);
void handle_state(monitor_t *m, desktop_t *d, node_t *n, xcb_atom_t state, unsigned int action);
void mapping_notify(xcb_generic_event_t *evt);
void process_error(xcb_generic_event_t *evt);

#endif
