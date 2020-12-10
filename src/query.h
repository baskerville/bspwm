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

enum {
	SELECTOR_OK,
	SELECTOR_INVALID,
	SELECTOR_BAD_MODIFIERS,
	SELECTOR_BAD_DESCRIPTOR
};

typedef void (*monitor_printer_t)(monitor_t *m, FILE *rsp);
typedef void (*desktop_printer_t)(desktop_t *m, FILE *rsp);

void query_state(FILE *rsp);
void query_monitor(monitor_t *m, FILE *rsp);
void query_desktop(desktop_t *d, FILE *rsp);
void query_node(node_t *n, FILE *rsp);
void query_presel(presel_t *p, FILE *rsp);
void query_client(client_t *c, FILE *rsp);
void query_rectangle(xcb_rectangle_t r, FILE *rsp);
void query_constraints(constraints_t c, FILE *rsp);
void query_padding(padding_t p, FILE *rsp);
void query_history(FILE *rsp);
void query_coordinates(coordinates_t *loc, FILE *rsp);
void query_stack(FILE *rsp);
void query_subscribers(FILE *rsp);
int query_node_ids(coordinates_t *mon_ref, coordinates_t *desk_ref, coordinates_t* ref, coordinates_t *trg, monitor_select_t *mon_sel, desktop_select_t *desk_sel, node_select_t *sel, FILE *rsp);
int query_node_ids_in(node_t *n, desktop_t *d, monitor_t *m, coordinates_t *ref, coordinates_t *trg, node_select_t *sel, FILE *rsp);
int query_desktop_ids(coordinates_t* mon_ref, coordinates_t *ref, coordinates_t *trg, monitor_select_t *mon_sel, desktop_select_t *sel, desktop_printer_t printer, FILE *rsp);
int query_monitor_ids(coordinates_t *ref, coordinates_t *trg, monitor_select_t *sel, monitor_printer_t printer, FILE *rsp);
void fprint_monitor_id(monitor_t *m, FILE *rsp);
void fprint_monitor_name(monitor_t *m, FILE *rsp);
void fprint_desktop_id(desktop_t *d, FILE *rsp);
void fprint_desktop_name(desktop_t *d, FILE *rsp);
void print_ignore_request(state_transition_t st, FILE *rsp);
void print_modifier_mask(uint16_t m, FILE *rsp);
void print_button_index(int8_t b, FILE *rsp);
void print_pointer_action(pointer_action_t a, FILE *rsp);
void resolve_rule_consequence(rule_consequence_t *csq);
void print_rule_consequence(char **buf, rule_consequence_t *csq);
void print_rectangle(char **buf, xcb_rectangle_t *rect);
node_select_t make_node_select(void);
desktop_select_t make_desktop_select(void);
monitor_select_t make_monitor_select(void);
int node_from_desc(char *desc, coordinates_t *ref, coordinates_t *dst);
int desktop_from_desc(char *desc, coordinates_t *ref, coordinates_t *dst);
int monitor_from_desc(char *desc, coordinates_t *ref, coordinates_t *dst);
bool locate_leaf(xcb_window_t win, coordinates_t *loc);
bool locate_window(xcb_window_t win, coordinates_t *loc);
bool locate_desktop(char *name, coordinates_t *loc);
bool locate_monitor(char *name, coordinates_t *loc);
bool desktop_from_id(uint32_t id, coordinates_t *loc, monitor_t *mm);
bool desktop_from_name(char *name, coordinates_t *ref, coordinates_t *dst, desktop_select_t *sel, int *hits);
bool desktop_from_index(uint16_t idx, coordinates_t *loc, monitor_t *mm);
bool monitor_from_id(uint32_t id, coordinates_t *loc);
bool monitor_from_index(int idx, coordinates_t *loc);
bool node_matches(coordinates_t *loc, coordinates_t *ref, node_select_t *sel);
bool desktop_matches(coordinates_t *loc, coordinates_t *ref, desktop_select_t *sel);
bool monitor_matches(coordinates_t *loc, __attribute__((unused)) coordinates_t *ref, monitor_select_t *sel);

#endif
