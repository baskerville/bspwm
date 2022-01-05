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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include "bspwm.h"
#include "ewmh.h"
#include "window.h"
#include "query.h"
#include "parse.h"
#include "settings.h"
#include "rule.h"

rule_t *make_rule(void)
{
	rule_t *r = calloc(1, sizeof(rule_t));
	r->class_name[0] = r->instance_name[0] = r->name[0] = r->effect[0] = '\0';
	r->next = r->prev = NULL;
	r->one_shot = false;
	return r;
}

void add_rule(rule_t *r)
 {
	if (rule_head == NULL) {
		rule_head = rule_tail = r;
	} else {
		rule_tail->next = r;
		r->prev = rule_tail;
		rule_tail = r;
	}
}

void remove_rule(rule_t *r)
{
	if (r == NULL) {
		return;
	}
	rule_t *prev = r->prev;
	rule_t *next = r->next;
	if (prev != NULL) {
		prev->next = next;
	}
	if (next != NULL) {
		next->prev = prev;
	}
	if (r == rule_head) {
		rule_head = next;
	}
	if (r == rule_tail) {
		rule_tail = prev;
	}
	free(r);
}

void remove_rule_by_cause(char *cause)
{
	rule_t *r = rule_head;
	struct tokenize_state state;
	char *class_name = tokenize_with_escape(&state, cause, COL_TOK[0]);
	char *instance_name = tokenize_with_escape(&state, NULL, COL_TOK[0]);
	char *name = tokenize_with_escape(&state, NULL, COL_TOK[0]);
	if (!class_name || !instance_name || !name) {
		free(class_name);
		free(instance_name);
		free(name);
		return;
	}

	while (r != NULL) {
		rule_t *next = r->next;
		if ((class_name != NULL && (streq(class_name, MATCH_ANY) || streq(r->class_name, class_name))) &&
		    (instance_name == NULL || streq(instance_name, MATCH_ANY) || streq(r->instance_name, instance_name)) &&
		    (name == NULL || streq(name, MATCH_ANY) || streq(r->name, name))) {
			remove_rule(r);
		}
		r = next;
	}
	free(class_name);
	free(instance_name);
	free(name);
}

bool remove_rule_by_index(int idx)
{
	for (rule_t *r = rule_head; r != NULL; r = r->next, idx--) {
		if (idx == 0) {
			remove_rule(r);
			return true;
		}
	}
	return false;
}

rule_consequence_t *make_rule_consequence(void)
{
	rule_consequence_t *rc = calloc(1, sizeof(rule_consequence_t));
	rc->manage = rc->focus = rc->border = true;
	rc->layer = NULL;
	rc->state = NULL;
	rc->rect = NULL;
	return rc;
}

pending_rule_t *make_pending_rule(int fd, xcb_window_t win, rule_consequence_t *csq)
{
	pending_rule_t *pr = calloc(1, sizeof(pending_rule_t));
	pr->prev = pr->next = NULL;
	pr->event_head = pr->event_tail = NULL;
	pr->fd = fd;
	pr->win = win;
	pr->csq = csq;
	return pr;
}

void add_pending_rule(pending_rule_t *pr)
{
	if (pr == NULL) {
		return;
	}
	if (pending_rule_head == NULL) {
		pending_rule_head = pending_rule_tail = pr;
	} else {
		pending_rule_tail->next = pr;
		pr->prev = pending_rule_tail;
		pending_rule_tail = pr;
	}
}

void remove_pending_rule(pending_rule_t *pr)
{
	if (pr == NULL) {
		return;
	}
	pending_rule_t *a = pr->prev;
	pending_rule_t *b = pr->next;
	if (a != NULL) {
		a->next = b;
	}
	if (b != NULL) {
		b->prev = a;
	}
	if (pr == pending_rule_head) {
		pending_rule_head = b;
	}
	if (pr == pending_rule_tail) {
		pending_rule_tail = a;
	}
	close(pr->fd);
	free(pr->csq);
	event_queue_t *eq = pr->event_head;
	while (eq != NULL) {
		event_queue_t *next = eq->next;
		free(eq);
		eq = next;
	}
	free(pr);
}

void postpone_event(pending_rule_t *pr, xcb_generic_event_t *evt)
{
	event_queue_t *eq = make_event_queue(evt);
	if (pr->event_tail == NULL) {
		pr->event_head = pr->event_tail = eq;
	} else {
		pr->event_tail->next = eq;
		eq->prev = pr->event_tail;
		pr->event_tail = eq;
	}
}

event_queue_t *make_event_queue(xcb_generic_event_t *evt)
{
	event_queue_t *eq = calloc(1, sizeof(event_queue_t));
	eq->prev = eq->next = NULL;
	eq->event = *evt;
	return eq;
}


#define SET_CSQ_SPLIT_DIR(val) \
	do { \
		if (csq->split_dir == NULL) { \
			csq->split_dir = calloc(1, sizeof(direction_t)); \
		} \
		*(csq->split_dir) = (val); \
	} while (0)

#define SET_CSQ_STATE(val) \
	do { \
		if (csq->state == NULL) { \
			csq->state = calloc(1, sizeof(client_state_t)); \
		} \
		*(csq->state) = (val); \
	} while (0)

#define SET_CSQ_LAYER(val) \
	do { \
		if (csq->layer == NULL) { \
			csq->layer = calloc(1, sizeof(stack_layer_t)); \
		} \
		*(csq->layer) = (val); \
	} while (0)

void _apply_window_type(xcb_window_t win, rule_consequence_t *csq)
{
	xcb_ewmh_get_atoms_reply_t win_type;
	if (xcb_ewmh_get_wm_window_type_reply(ewmh, xcb_ewmh_get_wm_window_type(ewmh, win), &win_type, NULL) == 1) {
		for (unsigned int i = 0; i < win_type.atoms_len; i++) {
			xcb_atom_t a = win_type.atoms[i];
			if (a == ewmh->_NET_WM_WINDOW_TYPE_TOOLBAR ||
			    a == ewmh->_NET_WM_WINDOW_TYPE_UTILITY) {
				csq->focus = false;
			} else if (a == ewmh->_NET_WM_WINDOW_TYPE_DIALOG) {
				SET_CSQ_STATE(STATE_FLOATING);
				csq->center = true;
			} else if (a == ewmh->_NET_WM_WINDOW_TYPE_DOCK ||
			           a == ewmh->_NET_WM_WINDOW_TYPE_DESKTOP ||
			           a == ewmh->_NET_WM_WINDOW_TYPE_NOTIFICATION) {
				csq->manage = false;
				if (a == ewmh->_NET_WM_WINDOW_TYPE_DESKTOP) {
					window_lower(win);
				}
			}
		}
		xcb_ewmh_get_atoms_reply_wipe(&win_type);
	}
}

void _apply_window_state(xcb_window_t win, rule_consequence_t *csq)
{
	xcb_ewmh_get_atoms_reply_t win_state;
	if (xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, win), &win_state, NULL) == 1) {
		for (unsigned int i = 0; i < win_state.atoms_len; i++) {
			xcb_atom_t a = win_state.atoms[i];
			if (a == ewmh->_NET_WM_STATE_FULLSCREEN) {
				SET_CSQ_STATE(STATE_FULLSCREEN);
			} else if (a == ewmh->_NET_WM_STATE_BELOW) {
				SET_CSQ_LAYER(LAYER_BELOW);
			} else if (a == ewmh->_NET_WM_STATE_ABOVE) {
				SET_CSQ_LAYER(LAYER_ABOVE);
			} else if (a == ewmh->_NET_WM_STATE_STICKY) {
				csq->sticky = true;
			}
		}
		xcb_ewmh_get_atoms_reply_wipe(&win_state);
	}
}

void _apply_transient(xcb_window_t win, rule_consequence_t *csq)
{
	xcb_window_t transient_for = XCB_NONE;
	xcb_icccm_get_wm_transient_for_reply(dpy, xcb_icccm_get_wm_transient_for(dpy, win), &transient_for, NULL);
	if (transient_for != XCB_NONE) {
		SET_CSQ_STATE(STATE_FLOATING);
	}
}

void _apply_hints(xcb_window_t win, rule_consequence_t *csq)
{
	xcb_size_hints_t size_hints;
	if (xcb_icccm_get_wm_normal_hints_reply(dpy, xcb_icccm_get_wm_normal_hints(dpy, win), &size_hints, NULL) == 1) {
		if ((size_hints.flags & (XCB_ICCCM_SIZE_HINT_P_MIN_SIZE | XCB_ICCCM_SIZE_HINT_P_MAX_SIZE)) &&
		    size_hints.min_width == size_hints.max_width && size_hints.min_height == size_hints.max_height) {
			SET_CSQ_STATE(STATE_FLOATING);
		}
	}
}

void _apply_class(xcb_window_t win, rule_consequence_t *csq)
{
	xcb_icccm_get_wm_class_reply_t reply;
	if (xcb_icccm_get_wm_class_reply(dpy, xcb_icccm_get_wm_class(dpy, win), &reply, NULL) == 1) {
		snprintf(csq->class_name, sizeof(csq->class_name), "%s", reply.class_name);
		snprintf(csq->instance_name, sizeof(csq->instance_name), "%s", reply.instance_name);
		xcb_icccm_get_wm_class_reply_wipe(&reply);
	}
}

void _apply_name(xcb_window_t win, rule_consequence_t *csq)
{
	xcb_icccm_get_text_property_reply_t reply;
	if (xcb_icccm_get_wm_name_reply(dpy, xcb_icccm_get_wm_name(dpy, win), &reply, NULL) == 1) {
		snprintf(csq->name, sizeof(csq->name), "%s", reply.name);
		xcb_icccm_get_text_property_reply_wipe(&reply);
	}
}

void parse_keys_values(char *buf, rule_consequence_t *csq)
{
	char *key = strtok(buf, CSQ_BLK);
	char *value = strtok(NULL, CSQ_BLK);
	while (key != NULL && value != NULL) {
		parse_key_value(key, value, csq);
		key = strtok(NULL, CSQ_BLK);
		value = strtok(NULL, CSQ_BLK);
	}
}

void apply_rules(xcb_window_t win, rule_consequence_t *csq)
{
	_apply_window_type(win, csq);
	_apply_window_state(win, csq);
	_apply_transient(win, csq);
	_apply_hints(win, csq);
	_apply_class(win, csq);
	_apply_name(win, csq);

	rule_t *rule = rule_head;
	while (rule != NULL) {
		rule_t *next = rule->next;
		if ((streq(rule->class_name, MATCH_ANY) || streq(rule->class_name, csq->class_name)) &&
		    (streq(rule->instance_name, MATCH_ANY) || streq(rule->instance_name, csq->instance_name)) &&
		    (streq(rule->name, MATCH_ANY) || streq(rule->name, csq->name))) {
			char effect[MAXLEN];
			snprintf(effect, sizeof(effect), "%s", rule->effect);
			parse_keys_values(effect, csq);
			if (rule->one_shot) {
				remove_rule(rule);
				break;
			}
		}
		rule = next;
	}
}

bool schedule_rules(xcb_window_t win, rule_consequence_t *csq)
{
	if (external_rules_command[0] == '\0') {
		return false;
	}
	resolve_rule_consequence(csq);
	int fds[2];
	if (pipe(fds) == -1) {
		return false;
	}
	pid_t pid = fork();
	if (pid == 0) {
		if (dpy != NULL) {
			close(xcb_get_file_descriptor(dpy));
		}
		dup2(fds[1], 1);
		close(fds[0]);
		char wid[SMALEN];
		char *csq_buf;
		print_rule_consequence(&csq_buf, csq);
		snprintf(wid, sizeof(wid), "%i", win);
		setsid();
		execl(external_rules_command, external_rules_command, wid, csq->class_name, csq->instance_name, csq_buf, NULL);
		free(csq_buf);
		err("Couldn't spawn rule command.\n");
	} else if (pid > 0) {
		close(fds[1]);
		pending_rule_t *pr = make_pending_rule(fds[0], win, csq);
		add_pending_rule(pr);
	}
	return (pid != -1);
}

void parse_rule_consequence(int fd, rule_consequence_t *csq)
{
	if (fd == -1) {
		return;
	}
	char data[BUFSIZ];
	int nb;
	while ((nb = read(fd, data, sizeof(data))) > 0) {
		int end = MIN(nb, (int) sizeof(data) - 1);
		data[end] = '\0';
		parse_keys_values(data, csq);
	}
}

void parse_key_value(char *key, char *value, rule_consequence_t *csq)
{
	bool v;
	if (streq("monitor", key)) {
		snprintf(csq->monitor_desc, sizeof(csq->monitor_desc), "%s", value);
	} else if (streq("desktop", key)) {
		snprintf(csq->desktop_desc, sizeof(csq->desktop_desc), "%s", value);
	} else if (streq("node", key)) {
		snprintf(csq->node_desc, sizeof(csq->node_desc), "%s", value);
	} else if (streq("split_dir", key)) {
		direction_t dir;
		if (parse_direction(value, &dir)) {
			SET_CSQ_SPLIT_DIR(dir);
		}
	} else if (streq("state", key)) {
		client_state_t cst;
		if (parse_client_state(value, &cst)) {
			SET_CSQ_STATE(cst);
		}
	} else if (streq("layer", key)) {
		stack_layer_t lyr;
		if (parse_stack_layer(value, &lyr)) {
			SET_CSQ_LAYER(lyr);
		}
	} else if (streq("split_ratio", key)) {
		double rat;
		if (sscanf(value, "%lf", &rat) == 1 && rat > 0 && rat < 1) {
			csq->split_ratio = rat;
		}
	} else if (streq("rectangle", key)) {
		if (csq->rect == NULL) {
			csq->rect = calloc(1, sizeof(xcb_rectangle_t));
		}
		if (!parse_rectangle(value, csq->rect)) {
			free(csq->rect);
			csq->rect = NULL;
		}
	} else if (parse_bool(value, &v)) {
		if (streq("hidden", key)) {
			csq->hidden = v;
		}
#define SETCSQ(name) \
		else if (streq(#name, key)) { \
			csq->name = v; \
		}
		SETCSQ(sticky)
		SETCSQ(private)
		SETCSQ(locked)
		SETCSQ(marked)
		SETCSQ(center)
		SETCSQ(follow)
		SETCSQ(manage)
		SETCSQ(focus)
		SETCSQ(border)
#undef SETCSQ
	}
}

#undef SET_CSQ_LAYER
#undef SET_CSQ_STATE

void list_rules(FILE *rsp)
{
	for (rule_t *r = rule_head; r != NULL; r = r->next) {
		fprintf(rsp, "%s:%s:%s %c> %s\n", r->class_name, r->instance_name, r->name, r->one_shot?'-':'=', r->effect);
	}
}
