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

#ifndef BSPWM_BSPWM_H
#define BSPWM_BSPWM_H

#include "types.h"

#define ROOT_EVENT_MASK     (XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY)
#define CLIENT_EVENT_MASK   (XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_FOCUS_CHANGE)
#define META_WINDOW_IC      "wm\0Bspwm"
#define ROOT_WINDOW_IC      "root\0Bspwm"
#define MOTION_RECORDER_IC  "motion_recorder\0Bspwm"

xcb_connection_t *dpy;
int default_screen, screen_width, screen_height;
uint32_t num_clients;
unsigned int monitor_uid;
unsigned int desktop_uid;
xcb_screen_t *screen;
xcb_window_t root;
uint8_t root_depth;
char config_path[MAXLEN];

monitor_t *mon;
monitor_t *mon_head;
monitor_t *mon_tail;
monitor_t *pri_mon;
history_t *history_head;
history_t *history_tail;
history_t *history_needle;
rule_t *rule_head;
rule_t *rule_tail;
stacking_list_t *stack_head;
stacking_list_t *stack_tail;
subscriber_list_t *subscribe_head;
subscriber_list_t *subscribe_tail;
pending_rule_t *pending_rule_head;
pending_rule_t *pending_rule_tail;

pointer_state_t *frozen_pointer;
xcb_window_t meta_window;
xcb_window_t motion_recorder;
xcb_atom_t WM_TAKE_FOCUS;
xcb_atom_t WM_DELETE_WINDOW;
int exit_status;

bool visible;
bool auto_raise;
bool sticky_still;
bool record_history;
bool running;
bool randr;

void init(void);
void setup(void);
void register_events(void);
void cleanup(void);
bool check_connection (xcb_connection_t *dpy);
void sig_handler(int sig);

#endif
