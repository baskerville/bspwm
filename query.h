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

#define PTH_TOK  "/"

typedef enum {
	DOMAIN_TREE,
	DOMAIN_MONITOR,
	DOMAIN_DESKTOP,
	DOMAIN_NODE
} domain_t;

void query_tree(FILE *rsp);
void query_monitor(monitor_t *m, FILE *rsp);
void query_desktop(desktop_t *d, FILE *rsp);
void query_node(node_t *n, FILE *rsp);
void query_presel(presel_t *p, FILE *rsp);
void query_client(client_t *c, FILE *rsp);
void query_rectangle(xcb_rectangle_t r, FILE *rsp);
void query_wm_state(xcb_atom_t *wm_state, int wm_states_count, FILE *rsp);
void query_history(FILE *rsp);
void query_coordinates(coordinates_t *loc, FILE *rsp);
void query_stack(FILE *rsp);
void query_node_ids(coordinates_t loc, node_select_t *sel, FILE *rsp);
void query_node_ids_in(node_t *n, desktop_t *d, monitor_t *m, coordinates_t loc, node_select_t *sel, FILE *rsp);
void query_desktop_names(coordinates_t loc, desktop_select_t *sel, FILE *rsp);
void query_monitor_names(coordinates_t loc, monitor_select_t *sel, FILE *rsp);
node_select_t make_node_select(void);
desktop_select_t make_desktop_select(void);
monitor_select_t make_monitor_select(void);
bool node_from_desc(char *desc, coordinates_t *ref, coordinates_t *dst);
bool desktop_from_desc(char *desc, coordinates_t *ref, coordinates_t *dst);
bool monitor_from_desc(char *desc, coordinates_t *ref, coordinates_t *dst);
bool locate_window(xcb_window_t win, coordinates_t *loc);
bool locate_desktop(char *name, coordinates_t *loc);
bool locate_monitor(char *name, coordinates_t *loc);
bool desktop_from_index(int i, coordinates_t *loc, monitor_t *mm);
bool monitor_from_index(int i, coordinates_t *loc);
bool node_matches(coordinates_t *loc, coordinates_t *ref, node_select_t sel);
bool desktop_matches(coordinates_t *loc, coordinates_t *ref, desktop_select_t sel);
bool monitor_matches(coordinates_t *loc, __attribute__((unused)) coordinates_t *ref, monitor_select_t sel);

#endif
