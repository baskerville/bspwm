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
#include <string.h>
#include <unistd.h>
#include "bspwm.h"
#include "desktop.h"
#include "ewmh.h"
#include "history.h"
#include "pointer.h"
#include "monitor.h"
#include "query.h"
#include "stack.h"
#include "tree.h"
#include "settings.h"
#include "subscribe.h"
#include "restore.h"
#include "window.h"
#include "parse.h"

bool restore_state(const char *file_path)
{
	size_t jslen;
	char *json = read_string(file_path, &jslen);

	if (json == NULL) {
		return false;
	}

	int nbtok = 256;
	jsmn_parser parser;
	jsmntok_t *tokens = malloc(nbtok * sizeof(jsmntok_t));

	if (tokens == NULL) {
		perror("Restore tree: malloc");
		free(json);
		return false;
	}

	jsmn_init(&parser);
	int ret;

	while ((ret = jsmn_parse(&parser, json, jslen, tokens, nbtok)) == JSMN_ERROR_NOMEM) {
		nbtok *= 2;
		jsmntok_t *rtokens = realloc(tokens, nbtok * sizeof(jsmntok_t));
		if (rtokens == NULL) {
			perror("Restore tree: realloc");
			free(tokens);
			free(json);
			return false;
		} else {
			tokens = rtokens;
		}
	}

	if (ret < 0) {
		warn("Restore tree: jsmn_parse: ");
		switch (ret) {
			case JSMN_ERROR_NOMEM:
				warn("not enough memory.\n");
				break;
			case JSMN_ERROR_INVAL:
				warn("found invalid character inside JSON string.\n");
				break;
			case JSMN_ERROR_PART:
				warn("not a full JSON packet.\n");
				break;
			default:
				warn("unknown error.\n");
				break;
		}

		free(tokens);
		free(json);

		return false;
	}

	int num = tokens[0].size;

	if (num < 1) {
		free(tokens);
		free(json);

		return false;
	}

	mon = NULL;
	while (mon_head != NULL) {
		remove_monitor(mon_head);
	}

	jsmntok_t *t = tokens + 1;
	uint32_t focused_monitor_id = 0, primary_monitor_id = 0;
	jsmntok_t *focus_history_token = NULL, *stacking_list_token = NULL;

	for (int i = 0; i < num; i++) {
		if (keyeq("focusedMonitorId", t, json)) {
			t++;
			sscanf(json + t->start, "%u", &focused_monitor_id);
		} else if (keyeq("primaryMonitorId", t, json)) {
			t++;
			sscanf(json + t->start, "%u", &primary_monitor_id);
		} else if (keyeq("clientsCount", t, json)) {
			t++;
			sscanf(json + t->start, "%u", &clients_count);
		} else if (keyeq("monitors", t, json)) {
			t++;
			int s = t->size;
			t++;
			for (int j = 0; j < s; j++) {
				monitor_t *m = restore_monitor(&t, json);
				if (m->desk == NULL) {
					add_desktop(m, make_desktop(NULL, XCB_NONE));
				}
				add_monitor(m);
			}
			continue;
		} else if (keyeq("focusHistory", t, json)) {
			t++;
			if (mon == NULL) {
				focus_history_token = t;
			}
			restore_history(&t, json);
			continue;
		} else if (keyeq("stackingList", t, json)) {
			t++;
			if (mon == NULL) {
				stacking_list_token = t;
			}
			restore_stack(&t, json);
			continue;
		} else if (keyeq("eventSubscribers", t, json)) {
			t++;
			restore_subscribers(&t, json);
			continue;
		}
		t++;
	}

	if (focused_monitor_id != 0) {
		coordinates_t loc;
		if (monitor_from_id(focused_monitor_id, &loc)) {
			mon = loc.monitor;
		}
	}

	if (primary_monitor_id != 0) {
		coordinates_t loc;
		if (monitor_from_id(primary_monitor_id, &loc)) {
			pri_mon = loc.monitor;
		}
	}

	if (focus_history_token != NULL) {
		restore_history(&focus_history_token, json);
	}

	if (stacking_list_token != NULL) {
		restore_stack(&stacking_list_token, json);
	}

	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		m->id = xcb_generate_id(dpy);
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			d->id = xcb_generate_id(dpy);
			regenerate_ids_in(d->root);
			refresh_presel_feedbacks(m, d, d->root);
			restack_presel_feedbacks(d);

			for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root)) {
				if (n->client == NULL) {
					continue;
				}
				initialize_client(n);
				uint32_t values[] = {CLIENT_EVENT_MASK | (focus_follows_pointer ? XCB_EVENT_MASK_ENTER_WINDOW : 0)};
				xcb_change_window_attributes(dpy, n->id, XCB_CW_EVENT_MASK, values);
				window_grab_buttons(n->id);
			}
		}
	}

	ewmh_update_number_of_desktops();
	ewmh_update_desktop_names();
	ewmh_update_desktop_viewport();
	ewmh_update_current_desktop();
	ewmh_update_client_list(false);
	ewmh_update_client_list(true);
	ewmh_update_active_window();

	free(tokens);
	free(json);

	return true;
}

#define RESTORE_INT(k, p) \
	} else if (keyeq(#k, *t, json)) { \
		(*t)++; \
		sscanf(json + (*t)->start, "%i", p);

#define RESTORE_UINT(k, p) \
	} else if (keyeq(#k, *t, json)) { \
		(*t)++; \
		sscanf(json + (*t)->start, "%u", p);

#define RESTORE_USINT(k, p) \
	} else if (keyeq(#k, *t, json)) { \
		(*t)++; \
		sscanf(json + (*t)->start, "%hu", p);

#define RESTORE_DOUBLE(k, p) \
	} else if (keyeq(#k, *t, json)) { \
		(*t)++; \
		sscanf(json + (*t)->start, "%lf", p);

#define RESTORE_ANY(k, p, f) \
	} else if (keyeq(#k, *t, json)) { \
		(*t)++; \
		char *val = copy_string(json + (*t)->start, (*t)->end - (*t)->start); \
		f(val, p); \
		free(val);

#define RESTORE_BOOL(k, p)  RESTORE_ANY(k, p, parse_bool)

monitor_t *restore_monitor(jsmntok_t **t, char *json)
{
	int num = (*t)->size;
	(*t)++;
	monitor_t *m = make_monitor(NULL, NULL, UINT32_MAX);
	uint32_t focused_desktop_id = 0;

	for (int i = 0; i < num; i++) {
		if (keyeq("name", *t, json)) {
			(*t)++;
			snprintf(m->name, (*t)->end - (*t)->start + 1, "%s", json + (*t)->start);
		RESTORE_UINT(id, &m->id)
		RESTORE_UINT(randrId, &m->randr_id)
		RESTORE_BOOL(wired, &m->wired)
		RESTORE_UINT(stickyCount, &m->sticky_count)
		RESTORE_INT(windowGap, &m->window_gap)
		RESTORE_UINT(borderWidth, &m->border_width)
		RESTORE_UINT(focusedDesktopId, &focused_desktop_id)
		} else if (keyeq("padding", *t, json)) {
			(*t)++;
			restore_padding(&m->padding, t, json);
			continue;
		} else if (keyeq("rectangle", *t, json)) {
			(*t)++;
			restore_rectangle(&m->rectangle, t, json);
			update_root(m, &m->rectangle);
			continue;
		} else if (keyeq("desktops", *t, json)) {
			(*t)++;
			int s = (*t)->size;
			(*t)++;
			for (int j = 0; j < s; j++) {
				desktop_t *d = restore_desktop(t, json);
				add_desktop(m, d);
			}
			continue;
		} else {
			warn("Restore monitor: unknown key: '%.*s'.\n", (*t)->end - (*t)->start, json + (*t)->start);
			(*t)++;
		}
		(*t)++;
	}

	if (focused_desktop_id != 0) {
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			if (d->id == focused_desktop_id) {
				m->desk = d;
				break;
			}
		}
	}

	return m;
}

desktop_t *restore_desktop(jsmntok_t **t, char *json)
{
	int s = (*t)->size;
	(*t)++;
	desktop_t *d = make_desktop(NULL, UINT32_MAX);
	xcb_window_t focusedNodeId = XCB_NONE;

	for (int i = 0; i < s; i++) {
		if (keyeq("name", *t, json)) {
			(*t)++;
			snprintf(d->name, (*t)->end - (*t)->start + 1, "%s", json + (*t)->start);
		RESTORE_UINT(id, &d->id)
		RESTORE_ANY(layout, &d->layout, parse_layout)
		RESTORE_ANY(userLayout, &d->user_layout, parse_layout)
		RESTORE_INT(windowGap, &d->window_gap)
		RESTORE_UINT(borderWidth, &d->border_width)
		} else if (keyeq("focusedNodeId", *t, json)) {
			(*t)++;
			sscanf(json + (*t)->start, "%u", &focusedNodeId);
		} else if (keyeq("padding", *t, json)) {
			(*t)++;
			restore_padding(&d->padding, t, json);
			continue;
		} else if (keyeq("root", *t, json)) {
			(*t)++;
			d->root = restore_node(t, json);
			continue;
		} else {
			warn("Restore desktop: unknown key: '%.*s'.\n", (*t)->end - (*t)->start, json + (*t)->start);
			(*t)++;
		}
		(*t)++;
	}

	if (focusedNodeId != XCB_NONE) {
		d->focus = find_by_id_in(d->root, focusedNodeId);
	}

	return d;
}

node_t *restore_node(jsmntok_t **t, char *json)
{
	if ((*t)->type == JSMN_PRIMITIVE) {
		(*t)++;
		return NULL;
	} else {
		int s = (*t)->size;
		(*t)++;
		/* hack to prevent a new ID from being generated */
		node_t *n = make_node(UINT32_MAX);

		for (int i = 0; i < s; i++) {
			if (keyeq("id", *t, json)) {
				(*t)++;
				sscanf(json + (*t)->start, "%u", &n->id);
			RESTORE_ANY(splitType, &n->split_type, parse_split_type)
			RESTORE_DOUBLE(splitRatio, &n->split_ratio)
			RESTORE_BOOL(vacant, &n->vacant)
			RESTORE_BOOL(hidden, &n->hidden)
			RESTORE_BOOL(sticky, &n->sticky)
			RESTORE_BOOL(private, &n->private)
			RESTORE_BOOL(locked, &n->locked)
			RESTORE_BOOL(marked, &n->marked)
			} else if (keyeq("presel", *t, json)) {
				(*t)++;
				n->presel = restore_presel(t, json);
				continue;
			} else if (keyeq("rectangle", *t, json)) {
				(*t)++;
				restore_rectangle(&n->rectangle, t, json);
				continue;
			} else if (keyeq("constraints", *t, json)) {
				(*t)++;
				restore_constraints(&n->constraints, t, json);
				continue;
			} else if (keyeq("firstChild", *t, json)) {
				(*t)++;
				node_t *fc = restore_node(t, json);
				n->first_child = fc;
				if (fc != NULL) {
					fc->parent = n;
				}
				continue;
			} else if (keyeq("secondChild", *t, json)) {
				(*t)++;
				node_t *sc = restore_node(t, json);
				n->second_child = sc;
				if (sc != NULL) {
					sc->parent = n;
				}
				continue;
			} else if (keyeq("client", *t, json)) {
				(*t)++;
				n->client = restore_client(t, json);
				continue;
			} else {
				warn("Restore node: unknown key: '%.*s'.\n", (*t)->end - (*t)->start, json + (*t)->start);
				(*t)++;
			}
			(*t)++;
		}

		return n;
	}
}

presel_t *restore_presel(jsmntok_t **t, char *json)
{
	if ((*t)->type == JSMN_PRIMITIVE) {
		(*t)++;
		return NULL;
	} else {
		int s = (*t)->size;
		(*t)++;
		presel_t *p = make_presel();

		for (int i = 0; i < s; i++) {
			if (keyeq("splitRatio", *t, json)) {
				(*t)++;
				sscanf(json + (*t)->start, "%lf", &p->split_ratio);
			RESTORE_ANY(splitDir, &p->split_dir, parse_direction)
			}

			(*t)++;
		}

		return p;
	}
}


client_t *restore_client(jsmntok_t **t, char *json)
{
	if ((*t)->type == JSMN_PRIMITIVE) {
		(*t)++;
		return NULL;
	} else {
		int s = (*t)->size;
		(*t)++;
		client_t *c = make_client();

		for (int i = 0; i < s; i++) {
			if (keyeq("className", *t, json)) {
				(*t)++;
				snprintf(c->class_name, (*t)->end - (*t)->start + 1, "%s", json + (*t)->start);
			} else if (keyeq("instanceName", *t, json)) {
				(*t)++;
				snprintf(c->instance_name, (*t)->end - (*t)->start + 1, "%s", json + (*t)->start);
			RESTORE_ANY(state, &c->state, parse_client_state)
			RESTORE_ANY(lastState, &c->last_state, parse_client_state)
			RESTORE_ANY(layer, &c->layer, parse_stack_layer)
			RESTORE_ANY(lastLayer, &c->last_layer, parse_stack_layer)
			RESTORE_UINT(borderWidth, &c->border_width)
			RESTORE_BOOL(urgent, &c->urgent)
			RESTORE_BOOL(shown, &c->shown)
			} else if (keyeq("tiledRectangle", *t, json)) {
				(*t)++;
				restore_rectangle(&c->tiled_rectangle, t, json);
				continue;
			} else if (keyeq("floatingRectangle", *t, json)) {
				(*t)++;
				restore_rectangle(&c->floating_rectangle, t, json);
				continue;
			} else {
				warn("Restore client: unknown key: '%.*s'.\n", (*t)->end - (*t)->start, json + (*t)->start);
				(*t)++;
			}

			(*t)++;
		}

		return c;
	}
}

void restore_rectangle(xcb_rectangle_t *r, jsmntok_t **t, char *json)
{
	int s = (*t)->size;
	(*t)++;

	for (int i = 0; i < s; i++) {
		if (keyeq("x", *t, json)) {
			(*t)++;
			sscanf(json + (*t)->start, "%hi", &r->x);
		} else if (keyeq("y", *t, json)) {
			(*t)++;
			sscanf(json + (*t)->start, "%hi", &r->y);
		} else if (keyeq("width", *t, json)) {
			(*t)++;
			sscanf(json + (*t)->start, "%hu", &r->width);
		} else if (keyeq("height", *t, json)) {
			(*t)++;
			sscanf(json + (*t)->start, "%hu", &r->height);
		}
		(*t)++;
	}
}

void restore_constraints(constraints_t *c, jsmntok_t **t, char *json)
{
	int s = (*t)->size;
	(*t)++;

	for (int i = 0; i < s; i++) {
		if (keyeq("min_width", *t, json)) {
			(*t)++;
			sscanf(json + (*t)->start, "%hu", &c->min_width);
		} else if (keyeq("min_height", *t, json)) {
			(*t)++;
			sscanf(json + (*t)->start, "%hu", &c->min_height);
		}
		(*t)++;
	}
}

void restore_padding(padding_t *p, jsmntok_t **t, char *json)
{
	int s = (*t)->size;
	(*t)++;

	for (int i = 0; i < s; i++) {
		if (keyeq("top", *t, json)) {
			(*t)++;
			sscanf(json + (*t)->start, "%i", &p->top);
		} else if (keyeq("right", *t, json)) {
			(*t)++;
			sscanf(json + (*t)->start, "%i", &p->right);
		} else if (keyeq("bottom", *t, json)) {
			(*t)++;
			sscanf(json + (*t)->start, "%i", &p->bottom);
		} else if (keyeq("left", *t, json)) {
			(*t)++;
			sscanf(json + (*t)->start, "%i", &p->left);
		}
		(*t)++;
	}
}

void restore_history(jsmntok_t **t, char *json)
{
	int s = (*t)->size;
	(*t)++;

	for (int i = 0; i < s; i++) {
		coordinates_t loc = {NULL, NULL, NULL};
		restore_coordinates(&loc, t, json);
		if (loc.monitor != NULL && loc.desktop != NULL) {
			history_add(loc.monitor, loc.desktop, loc.node, true);
		}
	}
}

void restore_subscribers(jsmntok_t **t, char *json)
{
	int s = (*t)->size;
	(*t)++;

	for (int i = 0; i < s; i++) {
		subscriber_list_t *s = make_subscriber(NULL, NULL, 0, 0);
		restore_subscriber(s, t, json);
		add_subscriber(s);
	}
}

void restore_subscriber(subscriber_list_t *s, jsmntok_t **t, char *json)
{
	int n = (*t)->size;
	(*t)++;

	for (int i = 0; i < n; i++) {
		if (keyeq("fileDescriptor", *t, json)) {
			(*t)++;
			int fd;
			sscanf(json + (*t)->start, "%i", &fd);
			s->stream = fdopen(fd, "w");
		} else if (keyeq("fifoPath", *t, json)) {
			(*t)++;
			free(s->fifo_path);
			s->fifo_path = copy_string(json + (*t)->start, (*t)->end - (*t)->start);
		RESTORE_INT(field, &s->field)
		RESTORE_INT(count, &s->count)
		}
		(*t)++;
	}
}

void restore_coordinates(coordinates_t *loc, jsmntok_t **t, char *json)
{
	int s = (*t)->size;
	(*t)++;
	uint32_t id = 0;

	for (int i = 0; i < s; i++) {
		if (keyeq("monitorId", *t, json)) {
			(*t)++;
			sscanf(json + (*t)->start, "%u", &id);
			loc->monitor = find_monitor(id);
		} else if (keyeq("desktopId", *t, json)) {
			(*t)++;
			sscanf(json + (*t)->start, "%u", &id);
			loc->desktop = find_desktop_in(id, loc->monitor);
		} else if (keyeq("nodeId", *t, json)) {
			(*t)++;
			sscanf(json + (*t)->start, "%u", &id);
			loc->node = find_by_id_in(loc->desktop != NULL ? loc->desktop->root : NULL, id);
		}
		(*t)++;
	}
}

void restore_stack(jsmntok_t **t, char *json)
{
	int s = (*t)->size;
	(*t)++;

	for (int i = 0; i < s; i++) {
		uint32_t id;
		sscanf(json + (*t)->start, "%u", &id);
		coordinates_t loc;
		if (locate_window(id, &loc)) {
			stack_insert_after(stack_tail, loc.node);
		}
		(*t)++;
	}
}

#undef RESTORE_INT
#undef RESTORE_UINT
#undef RESTORE_USINT
#undef RESTORE_DOUBLE
#undef RESTORE_ANY
#undef RESTORE_BOOL

bool keyeq(char *s, jsmntok_t *key, char *json)
{
	size_t n = key->end - key->start;
	return (strlen(s) == n && strncmp(s, json + key->start, n) == 0);
}
