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

#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "bspwm.h"
#include "desktop.h"
#include "ewmh.h"
#include "history.h"
#include "monitor.h"
#include "query.h"
#include "stack.h"
#include "tree.h"
#include "settings.h"
#include "restore.h"
#include "helpers.h"
#include "common.h"
#include "parse.h"
#include "jsmn.h"

bool restore_tree(const char *file_path)
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

	while (mon_head != NULL) {
		remove_monitor(mon_head);
	}

	int num = tokens[0].size;
	jsmntok_t *t = tokens + 1;
	char *focusedMonitorName = NULL;

	for (int i = 0; i < num; i++) {
		if (keyeq("focusedMonitorName", t, json)) {
			focusedMonitorName = copy_string(t+1, json);
			t++;
		} else if (keyeq("numClients", t, json)) {
			t++;
			sscanf(json + t->start, "%i", &num_clients);
		} else if (keyeq("monitors", t, json)) {
			t++;
			int s = t->size;
			t++;
			for (int j = 0; j < s; j++) {
				monitor_t *m = restore_monitor(&t, json);
				add_monitor(m);
				t++;
			}
		}
		t++;
	}

	if (focusedMonitorName != NULL) {
		coordinates_t loc;
		if (locate_monitor(focusedMonitorName, &loc)) {
			mon = loc.monitor;
		}
	}

	free(focusedMonitorName);

	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root)) {
				uint32_t values[] = {CLIENT_EVENT_MASK | (focus_follows_pointer ? XCB_EVENT_MASK_ENTER_WINDOW : 0)};
				xcb_change_window_attributes(dpy, n->client->window, XCB_CW_EVENT_MASK, values);
			}
		}
	}

	ewmh_update_client_list();
	ewmh_update_number_of_desktops();
	ewmh_update_current_desktop();
	ewmh_update_desktop_names();

	free(tokens);
	free(json);

	return true;
}

#define RESTORE_INT(o, k, p) \
	} else if (keyeq(#k, *t, json)) { \
		(*t)++; \
		sscanf(json + (*t)->start, "%i", &o->p);

#define RESTORE_UINT(o, k, p) \
	} else if (keyeq(#k, *t, json)) { \
		(*t)++; \
		sscanf(json + (*t)->start, "%u", &o->p);

#define RESTORE_USINT(o, k, p) \
	} else if (keyeq(#k, *t, json)) { \
		(*t)++; \
		sscanf(json + (*t)->start, "%hu", &o->p);

#define RESTORE_DOUBLE(o, k, p) \
	} else if (keyeq(#k, *t, json)) { \
		(*t)++; \
		sscanf(json + (*t)->start, "%lf", &o->p);

#define RESTORE_ANY(o, k, p, f) \
	} else if (keyeq(#k, *t, json)) { \
		(*t)++; \
		char *val = copy_string(*t, json); \
		f(val, &o->p); \
		free(val);

#define RESTORE_BOOL(o, k, p)  RESTORE_ANY(o, k, p, parse_bool)

monitor_t *restore_monitor(jsmntok_t **t, char *json)
{
	int num = (*t)->size;
	(*t)++;
	monitor_t *m = make_monitor(NULL);
	char *focusedDesktopName = NULL;

	for (int i = 0; i < num; i++) {
		if (keyeq("name", *t, json)) {
			(*t)++;
			snprintf(m->name, (*t)->end - (*t)->start + 1, "%s", json + (*t)->start);
		RESTORE_UINT(m, id, id)
		RESTORE_BOOL(m, wired, wired)
		RESTORE_INT(m, topPadding, top_padding)
		RESTORE_INT(m, rightPadding, right_padding)
		RESTORE_INT(m, bottomPadding, bottom_padding)
		RESTORE_INT(m, leftPadding, left_padding)
		RESTORE_INT(m, numSticky, num_sticky)
		} else if (keyeq("rectangle", *t, json)) {
			(*t)++;
			restore_rectangle(&m->rectangle, t, json);
			update_root(m, &m->rectangle);
			continue;
		} else if (keyeq("focusedDesktopName", *t, json)) {
			(*t)++;
			focusedDesktopName = copy_string(*t, json);
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

	if (focusedDesktopName != NULL) {
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			if (streq(focusedDesktopName, d->name)) {
				m->desk = d;
				break;
			}
		}
	}

	free(focusedDesktopName);

	return m;
}

desktop_t *restore_desktop(jsmntok_t **t, char *json)
{
	int s = (*t)->size;
	(*t)++;
	desktop_t *d = make_desktop(NULL);
	xcb_window_t focusedWindow = XCB_NONE;

	for (int i = 0; i < s; i++) {
		if (keyeq("name", *t, json)) {
			(*t)++;
			snprintf(d->name, (*t)->end - (*t)->start + 1, "%s", json + (*t)->start);
		} else if (keyeq("layout", *t, json)) {
			(*t)++;
			char *val = copy_string(*t, json);
			layout_t lyt;
			if (parse_layout(val, &lyt)) {
				d->layout = lyt;
			}
			free(val);
		RESTORE_INT(d, topPadding, top_padding)
		RESTORE_INT(d, rightPadding, right_padding)
		RESTORE_INT(d, bottomPadding, bottom_padding)
		RESTORE_INT(d, leftPadding, left_padding)
		RESTORE_INT(d, windowGap, window_gap)
		RESTORE_UINT(d, borderWidth, border_width)
		} else if (keyeq("focusedWindow", *t, json)) {
			(*t)++;
			sscanf(json + (*t)->start, "%u", &focusedWindow);
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

	if (focusedWindow != XCB_NONE) {
		for (node_t *f = first_extrema(d->root); f != NULL; f = next_leaf(f, d->root)) {
			if (f->client->window == focusedWindow) {
				d->focus = f;
				break;
			}
		}
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
		node_t *n = make_node();

		for (int i = 0; i < s; i++) {
			if (keyeq("splitType", *t, json)) {
				(*t)++;
				char *val = copy_string(*t, json);
				parse_split_type(val, &n->split_type);
				free(val);
			RESTORE_DOUBLE(n, splitRatio, split_ratio)
			RESTORE_ANY(n, splitMode, split_mode, parse_split_mode)
			RESTORE_ANY(n, splitDir, split_dir, parse_direction)
			RESTORE_INT(n, birthRotation, birth_rotation)
			RESTORE_INT(n, privacyLevel, privacy_level)
			RESTORE_ANY(n, vacant, vacant, parse_bool)
			} else if (keyeq("rectangle", *t, json)) {
				(*t)++;
				restore_rectangle(&n->rectangle, t, json);
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
			}
			(*t)++;
		}

		return n;
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
		client_t *c = make_client(XCB_NONE, 0);

		for (int i = 0; i < s; i++) {
			if (keyeq("window", *t, json)) {
				(*t)++;
				sscanf(json + (*t)->start, "%u", &c->window);
			} else if (keyeq("className", *t, json)) {
				(*t)++;
				snprintf(c->class_name, (*t)->end - (*t)->start + 1, "%s", json + (*t)->start);
			} else if (keyeq("instanceName", *t, json)) {
				(*t)++;
				snprintf(c->instance_name, (*t)->end - (*t)->start + 1, "%s", json + (*t)->start);
			RESTORE_ANY(c, state, state, parse_client_state)
			RESTORE_ANY(c, lastState, last_state, parse_client_state)
			RESTORE_ANY(c, layer, layer, parse_stack_layer)
			RESTORE_ANY(c, lastLayer, last_layer, parse_stack_layer)
			RESTORE_UINT(c, borderWidth, border_width)
			RESTORE_BOOL(c, locked, locked)
			RESTORE_BOOL(c, sticky, sticky)
			RESTORE_BOOL(c, urgent, urgent)
			RESTORE_BOOL(c, private, private)
			RESTORE_BOOL(c, icccmFocus, icccm_focus)
			RESTORE_BOOL(c, icccmInput, icccm_input)
			RESTORE_USINT(c, minWidth, min_width)
			RESTORE_USINT(c, maxWidth, max_width)
			RESTORE_USINT(c, minHeight, min_height)
			RESTORE_USINT(c, maxHeight, max_height)
			RESTORE_INT(c, numStates, num_states)
			} else if (keyeq("wmState", *t, json)) {
				(*t)++;
				restore_wm_state(c->wm_state, t, json);
				continue;
			} else if (keyeq("tiledRectangle", *t, json)) {
				(*t)++;
				restore_rectangle(&c->tiled_rectangle, t, json);
				continue;
			} else if (keyeq("floatingRectangle", *t, json)) {
				(*t)++;
				restore_rectangle(&c->floating_rectangle, t, json);
				continue;
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

void restore_wm_state(xcb_atom_t *w, jsmntok_t **t, char *json)
{
	int s = (*t)->size;
	(*t)++;

	for (int i = 0; i < s; i++) {
		sscanf(json + (*t)->start, "%u", &w[i]);
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
	return (strncmp(s, json + key->start, key->end - key->start) == 0);
}

char *copy_string(jsmntok_t *tok, char *json)
{
	size_t len = tok->end - tok->start + 1;
	char *res = malloc(len * sizeof(char));
	if (res == NULL) {
		perror("Copy string: malloc");
		return NULL;
	}
	strncpy(res, json+tok->start, len-1);
	res[len-1] = '\0';
	return res;
}

bool restore_history(const char *file_path)
{
	if (file_path == NULL) {
		return false;
	}

	FILE *snapshot = fopen(file_path, "r");
	if (snapshot == NULL) {
		perror("Restore history: fopen");
		return false;
	}

	char line[MAXLEN];
	char mnm[SMALEN];
	char dnm[SMALEN];
	xcb_window_t win;

	empty_history();

	while (fgets(line, sizeof(line), snapshot) != NULL) {
		if (sscanf(line, "%s %s %X", mnm, dnm, &win) == 3) {
			coordinates_t loc;
			if (win != XCB_NONE && !locate_window(win, &loc)) {
				warn("Can't locate window 0x%X.\n", win);
				continue;
			}
			node_t *n = (win == XCB_NONE ? NULL : loc.node);
			if (!locate_desktop(dnm, &loc)) {
				warn("Can't locate desktop '%s'.\n", dnm);
				continue;
			}
			desktop_t *d = loc.desktop;
			if (!locate_monitor(mnm, &loc)) {
				warn("Can't locate monitor '%s'.\n", mnm);
				continue;
			}
			monitor_t *m = loc.monitor;
			history_add(m, d, n);
		} else {
			warn("Can't parse history entry: '%s'\n", line);
		}
	}

	fclose(snapshot);
	return true;
}

bool restore_stack(const char *file_path)
{
	if (file_path == NULL) {
		return false;
	}

	FILE *snapshot = fopen(file_path, "r");
	if (snapshot == NULL) {
		perror("Restore stack: fopen");
		return false;
	}

	char line[MAXLEN];
	xcb_window_t win;

	while (stack_head != NULL) {
		remove_stack(stack_head);
	}

	while (fgets(line, sizeof(line), snapshot) != NULL) {
		if (sscanf(line, "%X", &win) == 1) {
			coordinates_t loc;
			if (locate_window(win, &loc)) {
				stack_insert_after(stack_tail, loc.node);
			} else {
				warn("Can't locate window 0x%X.\n", win);
			}
		} else {
			warn("Can't parse stack entry: '%s'\n", line);
		}
	}

	fclose(snapshot);
	return true;
}
