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
#include <strings.h>
#include <string.h>
#include "bspwm.h"
#include "desktop.h"
#include "history.h"
#include "messages.h"
#include "monitor.h"
#include "tree.h"
#include "query.h"
#include "json.h"

json_t* query_desktops_array_json(monitor_t *m)
{
	json_t *jdesktops = json_array();
	for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
		json_array_append_new(jdesktops, json_string(d->name));
	}
	return jdesktops;
}

json_t* query_node_json(node_t *n)
{
	json_t* json = json_serialize_node_type(n);
	json_object_set_new(json, "focused", n == mon->desk->focus ? json_true() : json_false());
	return json;
}

json_t* query_desktop_json(desktop_t *d)
{
	json_t* json = json_serialize_desktop_type(d);
	json_object_set_new(json, "focused", d == mon->desk ? json_true() : json_false());
	json_object_set_new(json, "nodes", d->root != NULL ? query_node_json(d->root) : json_null());
	return json;
}

json_t* query_monitor_json(monitor_t *m)
{
	json_t* json = json_serialize_monitor_type(m);
	json_object_set_new(json, "desktops", query_desktops_array_json(m));
	json_object_set_new(json, "focused", m == mon ? json_true() : json_false());
	return json;
}

json_t* query_windows_json(coordinates_t loc)
{
	json_t *jwindows = json_object();
	char id[11];
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		if (loc.monitor != NULL && m != loc.monitor)
			continue;
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			if (loc.desktop != NULL && d != loc.desktop)
				continue;
			for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root)) {
				if (loc.node != NULL && n != loc.node)
					continue;
				sprintf(id, "%d", n->client->window);
				json_t *jnode = json_pack("{s:o}", id, query_node_json(n));
				json_object_update(jwindows, jnode);
				json_decref(jnode);
			}
		}
	}
	return jwindows;
}

json_t* query_desktops_json(coordinates_t loc)
{
	json_t *jdesktops = json_object();
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		if (loc.monitor != NULL && m != loc.monitor)
			continue;
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			if (loc.desktop != NULL && d != loc.desktop)
				continue;
			json_t *jdesktop = json_pack("{s:o}", d->name, query_desktop_json(d));
			json_object_update(jdesktops, jdesktop);
			json_decref(jdesktop);
		}
	}
	return jdesktops;
}

json_t* query_monitors_json(coordinates_t loc)
{
	json_t *jmonitors = json_object();
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		if (loc.monitor != NULL && m != loc.monitor)
			continue;
		json_t *jmonitor = json_pack("{s:o}", m->name, query_monitor_json(m));
		json_object_update(jmonitors, jmonitor);
		json_decref(jmonitor);
	}
	return jmonitors;
}

json_t* query_tree_json(coordinates_t loc)
{
	json_t *jtree = json_object();
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		if (loc.monitor != NULL && m != loc.monitor)
			continue;
		json_t *jmonitor = query_monitor_json(m);
		json_t *jdesktops = json_array();
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			if (loc.desktop != NULL && d != loc.desktop)
				continue;
			json_array_append_new(jdesktops, query_desktop_json(d));
		}
		json_object_set_new(jmonitor, "desktops", jdesktops);
		json_object_set_new(jtree, m->name, jmonitor);
		json_object_clear(jdesktops);
	}
	return jtree;
}

json_t* query_history_json(coordinates_t loc)
{
	json_t *jhistory = json_array();
	for (history_t *h = history_head; h != NULL; h = h->next) {
		if ((loc.monitor != NULL && h->loc.monitor != loc.monitor)
				|| (loc.desktop != NULL && h->loc.desktop != loc.desktop))
			continue;
		xcb_window_t win = XCB_NONE;
		if (h->loc.node != NULL)
			win = h->loc.node->client->window;

		json_array_append_new(jhistory, json_pack(
			"{"
				"s:s,"
				"s:s,"
				"s:i"
			"}",
				"monitorName", h->loc.monitor->name,
				"desktopName", h->loc.desktop->name,
				"windowId", win
		));
	}
	return jhistory;
}

json_t* query_stack_json()
{
	json_t *jstack = json_array();
	for (stacking_list_t *s = stack_head; s != NULL; s = s->next) {
		json_array_append_new(jstack, json_integer(s->node->client->window));
	}
	return jstack;
}

client_select_t make_client_select(void)
{
	client_select_t sel = {
		.tiled = OPTION_NONE,
		.pseudo_tiled = OPTION_NONE,
		.floating = OPTION_NONE,
		.fullscreen = OPTION_NONE,
		.locked = OPTION_NONE,
		.sticky = OPTION_NONE,
		.private = OPTION_NONE,
		.urgent = OPTION_NONE,
		.same_class = OPTION_NONE,
		.automatic = OPTION_NONE,
		.local = OPTION_NONE,
		.focused = OPTION_NONE,
		.layer = NULL
	};
	return sel;
}

desktop_select_t make_desktop_select(void)
{
	desktop_select_t sel = {
		.occupied = OPTION_NONE,
		.urgent = OPTION_NONE,
		.local = OPTION_NONE
	};
	return sel;
}

void cleanup_client_select(client_select_t *sel)
{
	free(sel->layer);
}

bool node_from_desc(const char *desc, coordinates_t *ref, coordinates_t *dst)
{
	client_select_t sel = make_client_select();
	char *tok;
	while ((tok = strrchr(desc, CAT_CHR)) != NULL) {
		tok[0] = '\0';
		tok++;
		if (streq("tiled", tok)) {
			sel.tiled = OPTION_TRUE;
		} else if (streq("!tiled", tok)) {
			sel.tiled = OPTION_FALSE;
		} else if (streq("pseudo_tiled", tok)) {
			sel.pseudo_tiled = OPTION_TRUE;
		} else if (streq("!pseudo_tiled", tok)) {
			sel.pseudo_tiled = OPTION_FALSE;
		} else if (streq("floating", tok)) {
			sel.floating = OPTION_TRUE;
		} else if (streq("!floating", tok)) {
			sel.floating = OPTION_FALSE;
		} else if (streq("same_class", tok)) {
			sel.same_class = OPTION_TRUE;
		} else if (streq("!same_class", tok)) {
			sel.same_class = OPTION_FALSE;
		} else if (streq("automatic", tok)) {
			sel.automatic = OPTION_TRUE;
		} else if (streq("!automatic", tok)) {
			sel.automatic = OPTION_FALSE;
		} else if (streq("fullscreen", tok)) {
			sel.fullscreen = OPTION_TRUE;
		} else if (streq("!fullscreen", tok)) {
			sel.fullscreen = OPTION_FALSE;
		} else if (streq("urgent", tok)) {
			sel.urgent = OPTION_TRUE;
		} else if (streq("!urgent", tok)) {
			sel.urgent = OPTION_FALSE;
		} else if (streq("local", tok)) {
			sel.local = OPTION_TRUE;
		} else if (streq("!local", tok)) {
			sel.local = OPTION_FALSE;
		} else if (streq("private", tok)) {
			sel.private = OPTION_TRUE;
		} else if (streq("!private", tok)) {
			sel.private = OPTION_FALSE;
		} else if (streq("sticky", tok)) {
			sel.sticky = OPTION_TRUE;
		} else if (streq("!sticky", tok)) {
			sel.sticky = OPTION_FALSE;
		} else if (streq("locked", tok)) {
			sel.locked = OPTION_TRUE;
		} else if (streq("!locked", tok)) {
			sel.locked = OPTION_FALSE;
		} else if (streq("focused", tok)) {
			sel.focused = OPTION_TRUE;
		} else if (streq("!focused", tok)) {
			sel.focused = OPTION_FALSE;
		} else if (streq("below", tok)) {
			if (sel.layer == NULL) {
				sel.layer = malloc(sizeof(stack_layer_t));
			}
			*(sel.layer) = LAYER_BELOW;
		} else if (streq("normal", tok)) {
			if (sel.layer == NULL) {
				sel.layer = malloc(sizeof(stack_layer_t));
			}
			*(sel.layer) = LAYER_NORMAL;
		} else if (streq("above", tok)) {
			if (sel.layer == NULL) {
				sel.layer = malloc(sizeof(stack_layer_t));
			}
			*(sel.layer) = LAYER_ABOVE;
		}
	}

	dst->monitor = ref->monitor;
	dst->desktop = ref->desktop;
	dst->node = NULL;

	direction_t dir;
	cycle_dir_t cyc;
	history_dir_t hdi;
	if (parse_direction(desc, &dir)) {
		dst->node = nearest_neighbor(ref->monitor, ref->desktop, ref->node, dir, sel);
		if (dst->node == NULL && num_monitors > 1) {
			monitor_t *m = nearest_monitor(ref->monitor, dir, make_desktop_select());
			if (m != NULL) {
				coordinates_t loc = {m, m->desk, m->desk->focus};
				if (node_matches(&loc, ref, sel)) {
					dst->monitor = m;
					dst->desktop = m->desk;
					dst->node = m->desk->focus;
				}
			}
		}
	} else if (parse_cycle_direction(desc, &cyc)) {
		dst->node = closest_node(ref->monitor, ref->desktop, ref->node, cyc, sel);
	} else if (parse_history_direction(desc, &hdi)) {
		history_find_node(hdi, ref, dst, sel);
	} else if (streq("last", desc)) {
		history_find_node(HISTORY_OLDER, ref, dst, sel);
	} else if (streq("biggest", desc)) {
		dst->node = find_biggest(ref->monitor, ref->desktop, ref->node, sel);
	} else if (streq("focused", desc)) {
		coordinates_t loc = {mon, mon->desk, mon->desk->focus};
		if (node_matches(&loc, ref, sel)) {
			dst->monitor = mon;
			dst->desktop = mon->desk;
			dst->node = mon->desk->focus;
		}
	} else {
		long int wid;
		if (parse_window_id(desc, &wid))
			locate_window(wid, dst);
	}

	cleanup_client_select(&sel);

	return (dst->node != NULL);
}

bool desktop_from_desc(const char *desc, coordinates_t *ref, coordinates_t *dst)
{
	desktop_select_t sel = make_desktop_select();
	char *tok;
	while ((tok = strrchr(desc, CAT_CHR)) != NULL) {
		tok[0] = '\0';
		tok++;
		if (streq("occupied", tok)) {
			sel.occupied = OPTION_TRUE;
		} else if (streq("!occupied", tok)) {
			sel.occupied = OPTION_FALSE;
		} else if (streq("urgent", tok)) {
			sel.urgent = OPTION_TRUE;
		} else if (streq("!urgent", tok)) {
			sel.urgent = OPTION_FALSE;
		} else if (streq("local", tok)) {
			sel.local = OPTION_TRUE;
		} else if (streq("!local", tok)) {
			sel.local = OPTION_FALSE;
		}
	}

	dst->desktop = NULL;

	cycle_dir_t cyc;
	history_dir_t hdi;
	char *colon;
	int idx;
	if (parse_cycle_direction(desc, &cyc)) {
		dst->monitor = ref->monitor;
		dst->desktop = closest_desktop(ref->monitor, ref->desktop, cyc, sel);
	} else if (parse_history_direction(desc, &hdi)) {
		history_find_desktop(hdi, ref, dst, sel);
	} else if (streq("last", desc)) {
		history_find_desktop(HISTORY_OLDER, ref, dst, sel);
	} else if (streq("focused", desc)) {
		coordinates_t loc = {mon, mon->desk, NULL};
		if (desktop_matches(&loc, ref, sel)) {
			dst->monitor = mon;
			dst->desktop = mon->desk;
		}
	} else if ((colon = strchr(desc, ':')) != NULL) {
		*colon = '\0';
		if (monitor_from_desc(desc, ref, dst)) {
			if (streq("focused", colon + 1)) {
				dst->desktop = dst->monitor->desk;
			} else if (parse_index(colon + 1, &idx)) {
				desktop_from_index(idx, dst, dst->monitor);
			}
		}
	} else if (parse_index(desc, &idx)) {
		desktop_from_index(idx, dst, NULL);
	} else {
		locate_desktop(desc, dst);
	}

	return (dst->desktop != NULL);
}

bool monitor_from_desc(const char *desc, coordinates_t *ref, coordinates_t *dst)
{
	desktop_select_t sel = make_desktop_select();
	char *tok;
	while ((tok = strrchr(desc, CAT_CHR)) != NULL) {
		tok[0] = '\0';
		tok++;
		if (streq("occupied", tok)) {
			sel.occupied = OPTION_TRUE;
		} else if (streq("!occupied", tok)) {
			sel.occupied = OPTION_FALSE;
		}
	}

	dst->monitor = NULL;

	direction_t dir;
	cycle_dir_t cyc;
	history_dir_t hdi;
	int idx;
	if (parse_direction(desc, &dir)) {
		dst->monitor = nearest_monitor(ref->monitor, dir, sel);
	} else if (parse_cycle_direction(desc, &cyc)) {
		dst->monitor = closest_monitor(ref->monitor, cyc, sel);
	} else if (parse_history_direction(desc, &hdi)) {
		history_find_monitor(hdi, ref, dst, sel);
	} else if (streq("last", desc)) {
		history_find_monitor(HISTORY_OLDER, ref, dst, sel);
	} else if (streq("primary", desc)) {
		if (pri_mon != NULL) {
			coordinates_t loc = {pri_mon, pri_mon->desk, NULL};
			if (desktop_matches(&loc, ref, sel))
				dst->monitor = pri_mon;
		}
	} else if (streq("focused", desc)) {
		coordinates_t loc = {mon, mon->desk, NULL};
		if (desktop_matches(&loc, ref, sel))
			dst->monitor = mon;
	} else if (parse_index(desc, &idx)) {
		monitor_from_index(idx, dst);
	} else {
		locate_monitor(desc, dst);
	}

	return (dst->monitor != NULL);
}

bool locate_window(xcb_window_t win, coordinates_t *loc)
{
	for (monitor_t *m = mon_head; m != NULL; m = m->next)
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next)
			for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root))
				if (n->client->window == win) {
					loc->monitor = m;
					loc->desktop = d;
					loc->node = n;
					return true;
				}
	return false;
}

bool locate_desktop(const char *name, coordinates_t *loc)
{
	for (monitor_t *m = mon_head; m != NULL; m = m->next)
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next)
			if (streq(d->name, name)) {
				loc->monitor = m;
				loc->desktop = d;
				return true;
			}
	return false;
}

bool locate_monitor(const char *name, coordinates_t *loc)
{
	for (monitor_t *m = mon_head; m != NULL; m = m->next)
		if (streq(m->name, name)) {
			loc->monitor = m;
			return true;
		}
	return false;
}

bool desktop_from_index(int i, coordinates_t *loc, monitor_t *mm)
{
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		if (mm != NULL && m != mm)
			continue;
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next, i--)
			if (i == 1) {
				loc->monitor = m;
				loc->desktop = d;
				loc->node = NULL;
				return true;
			}
	}
	return false;
}

bool monitor_from_index(int i, coordinates_t *loc)
{
	for (monitor_t *m = mon_head; m != NULL; m = m->next, i--)
		if (i == 1) {
			loc->monitor = m;
			loc->desktop = NULL;
			loc->node = NULL;
			return true;
		}
	return false;
}

bool node_matches(coordinates_t *loc, coordinates_t *ref, client_select_t sel)
{
	if (loc->node == NULL)
		return false;

#define WSTATE(prop) \
	if (sel.prop != OPTION_NONE && \
	    !loc->node->client->prop \
	    ? sel.prop == OPTION_TRUE \
	    : sel.prop == OPTION_FALSE) { \
		return false; \
	}
	WSTATE(locked)
	WSTATE(sticky)
	WSTATE(private)
	WSTATE(urgent)
#undef MATCHSTATE

	if (sel.tiled != OPTION_NONE &&
	    loc->node->client->state != STATE_TILED
	    ? sel.tiled == OPTION_TRUE
	    : sel.tiled == OPTION_FALSE) {
		return false;
	}

	if (sel.pseudo_tiled != OPTION_NONE &&
	    loc->node->client->state != STATE_PSEUDO_TILED
	    ? sel.pseudo_tiled == OPTION_TRUE
	    : sel.pseudo_tiled == OPTION_FALSE) {
		return false;
	}

	if (sel.floating != OPTION_NONE &&
	    loc->node->client->state != STATE_FLOATING
	    ? sel.floating == OPTION_TRUE
	    : sel.floating == OPTION_FALSE) {
		return false;
	}

	if (sel.fullscreen != OPTION_NONE &&
	    loc->node->client->state != STATE_FULLSCREEN
	    ? sel.fullscreen == OPTION_TRUE
	    : sel.fullscreen == OPTION_FALSE) {
		return false;
	}

	if (sel.same_class != OPTION_NONE && ref->node != NULL &&
	    streq(loc->node->client->class_name, ref->node->client->class_name)
	    ? sel.same_class == OPTION_FALSE
	    : sel.same_class == OPTION_TRUE) {
		return false;
	}

	if (sel.automatic != OPTION_NONE &&
	    loc->node->split_mode == MODE_MANUAL
	    ? sel.automatic == OPTION_TRUE
	    : sel.automatic == OPTION_FALSE) {
		return false;
	}

	if (sel.local != OPTION_NONE &&
	    loc->desktop != ref->desktop
	    ? sel.local == OPTION_TRUE
	    : sel.local == OPTION_FALSE) {
		return false;
	}

	if (sel.layer != NULL && loc->node->client->layer != *(sel.layer)) {
		return false;
	}

	if (sel.focused != OPTION_NONE &&
	    loc->node != loc->desktop->focus
	    ? sel.focused == OPTION_TRUE
	    : sel.focused == OPTION_FALSE) {
		return false;
	}

	return true;
}

bool desktop_matches(coordinates_t *loc, coordinates_t *ref, desktop_select_t sel)
{
	if (sel.occupied != OPTION_NONE &&
	    loc->desktop->root == NULL
	    ? sel.occupied == OPTION_TRUE
	    : sel.occupied == OPTION_FALSE) {
		return false;
	}

	if (sel.urgent != OPTION_NONE &&
	    !is_urgent(loc->desktop)
	    ? sel.urgent == OPTION_TRUE
	    : sel.urgent == OPTION_FALSE) {
		return false;
	}

	if (sel.local != OPTION_NONE &&
	    ref->monitor != loc->monitor
	    ? sel.local == OPTION_TRUE
	    : sel.local == OPTION_FALSE) {
		return false;
	}

	return true;
}
