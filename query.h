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

#ifndef BSPWM_QUERY_H
#define BSPWM_QUERY_H

#include <jansson.h>

typedef enum {
	DOMAIN_MONITOR,
	DOMAIN_DESKTOP,
	DOMAIN_WINDOW,
	DOMAIN_TREE,
	DOMAIN_HISTORY,
	DOMAIN_STACK
} domain_t;

void query_monitors(coordinates_t loc, domain_t dom, FILE *rsp);
void query_desktops(monitor_t *m, domain_t dom, coordinates_t loc, unsigned int depth, FILE *rsp);
void query_tree(desktop_t *d, node_t *n, FILE *rsp, unsigned int depth);
void query_history(coordinates_t loc, FILE *rsp);
void query_stack(FILE *rsp);
void query_windows(coordinates_t loc, FILE *rsp);
void query_monitors_json(coordinates_t loc, domain_t dom, json_t *jmsg);
void query_desktops_json(monitor_t *m, coordinates_t loc, json_t *jmsg);
void query_history_json(coordinates_t loc, json_t *jmsg);
void query_stack_json(json_t *jmsg);
void query_windows_json(coordinates_t loc, json_t *jmsg);
client_select_t make_client_select(void);
desktop_select_t make_desktop_select(void);
void cleanup_client_select(client_select_t *sel);
bool node_from_desc(char *desc, coordinates_t *ref, coordinates_t *dst);
bool desktop_from_desc(char *desc, coordinates_t *ref, coordinates_t *dst);
bool monitor_from_desc(char *desc, coordinates_t *ref, coordinates_t *dst);
bool locate_window(xcb_window_t win, coordinates_t *loc);
bool locate_desktop(char *name, coordinates_t *loc);
bool locate_monitor(char *name, coordinates_t *loc);
bool desktop_from_index(int i, coordinates_t *loc, monitor_t *mm);
bool monitor_from_index(int i, coordinates_t *loc);
bool node_matches(coordinates_t *loc, coordinates_t *ref, client_select_t sel);
bool desktop_matches(coordinates_t *loc, coordinates_t *ref, desktop_select_t sel);

#endif
