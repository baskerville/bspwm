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

#include "types.h"

#define ERROR_CODE_BAD_WINDOW  3

extern uint8_t randr_base;

/* Button list — values from backend.h */
static const uint8_t BUTTONS[] = {BSP_BUTTON_1, BSP_BUTTON_2, BSP_BUTTON_3};

/* Event dispatch — called from the X11 event loop. */
void handle_event(void *evt);
void map_request(void *evt);
void configure_request(void *evt);
void configure_notify(void *evt);
void destroy_notify(void *evt);
void unmap_notify(void *evt);
void property_notify(void *evt);
void client_message(void *evt);
void focus_in(void *evt);
void button_press(void *evt);
void enter_notify(void *evt);
void motion_notify(void *evt);
void handle_state(monitor_t *m, desktop_t *d, node_t *n, uint32_t state, unsigned int action);
void key_press(void *evt);
void mapping_notify(void *evt);
void process_error(void *evt);

#endif
