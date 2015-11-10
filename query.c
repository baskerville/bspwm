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

json_t* query_rectangle_json(xcb_rectangle_t rec)
{
	return json_pack(
		"{"
			"s:i,"
			"s:i,"
			"s:i,"
			"s:i"
		"}",
			"x", rec.x,
			"y", rec.y,
			"width", rec.width,
			"height", rec.height
	);
}

json_t* query_desktops_array_json(monitor_t *m)
{
	json_t *jdesktops = json_array();
	for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
		json_array_append_new(jdesktops, json_string(d->name));
	}
	return jdesktops;
}

json_t* query_monitor_json(monitor_t *m)
{
	return json_pack(
		"{"
			"s:s,"
			"s:i,"
			"s:o,"
			"s:i,"
			"s:b,"
			"s:{"
				"s:i,"
				"s:i,"
				"s:i,"
				"s:i"
			"},"
			"s:o,"
			"s:o,"
			"s:o,"
			"s:o,"
			"s:o,"
			"s:o,"
			"s:o,"
			"s:i,"
			"s:o,"
			"s:b"
		"}",
			"name", m->name,
			"identifier", m->id,
			"rectangle", query_rectangle_json(m->rectangle),
			"rootWindowId", m->root,
			"wired", m->wired,
			"padding",
				"top", m->top_padding,
				"right", m->right_padding,
				"bottom", m->bottom_padding,
				"left", m->left_padding,
			"desktop", m->desk != NULL ? json_string(m->desk->name) : json_null() ,
			"desktopHead", m->desk_head != NULL ? json_string(m->desk_head->name) : json_null(),
			"desktopTail", m->desk_tail != NULL ? json_string(m->desk_tail->name) : json_null(),
			"prevName", m->prev != NULL ? json_string(m->prev->name) : json_null(),
			"prevIdentifier", m->prev != NULL ? json_integer(m->prev->id) : json_null(),
			"nextName", m->next != NULL ? json_string(m->next->name) : json_null(),
			"nextIdentifier", m->next != NULL ? json_integer(m->next->id) : json_null(),
			"stickyNumber", m->num_sticky,
			"desktops", query_desktops_array_json(m),
			"focused", m == mon ? true : false
	);
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

json_t* query_client_state_json(client_state_t state)
{
	return
		state == STATE_TILED ? json_string("tiled") :
		state == STATE_PSEUDO_TILED ? json_string("pseudo_tiled") :
		state == STATE_FLOATING ? json_string("floating") :
		state == STATE_FULLSCREEN ? json_string("fullscreen") :
		json_null();
}
json_t* query_stack_layer_json(stack_layer_t layer)
{
	return
		layer == LAYER_BELOW ? json_string("below") :
		layer == LAYER_NORMAL ? json_string("normal") :
		layer == LAYER_ABOVE ? json_string("above") :
		json_null();
}

json_t* query_client_json(client_t *c)
{
	return json_pack(
		"{"
			"s:i,"
			"s:s,"
			"s:s,"
			"s:i,"
			"s:b,"
			"s:b,"
			"s:b,"
			"s:b,"
			"s:b,"
			"s:o,"
			"s:o,"
			"s:o,"
			"s:o,"
			"s:o,"
			"s:o,"
			"s:{"
				"s:i,"
				"s:i,"
			"},"
			"s:{"
				"s:i,"
				"s:i,"
			"},"
			"s:i"
		"}",
			"windowId", c->window,
			"nameClass", c->class_name,
			"nameInstance", c->instance_name,
			"borderWidth", c->border_width,
			"locked", c->locked,
			"sticky", c->sticky,
			"urgent", c->urgent,
			"private", c->private,
			"icccm_focus", c->icccm_focus,
			"state", query_client_state_json(c->state),
			"stateLast", query_client_state_json(c->last_state),
			"layer", query_stack_layer_json(c->layer),
			"layerLast", query_stack_layer_json(c->last_layer),
			"rectangleFloating", query_rectangle_json(c->floating_rectangle),
			"rectangleTiled", query_rectangle_json(c->tiled_rectangle),
			"sizeMinimum",
				"width", c->min_width,
				"height", c->min_height,
			"sizeMaximum",
				"width", c->max_width,
				"height", c->max_height,
			"stateNumber", c->num_states
	);
}

json_t* query_split_type_json(split_type_t t)
{
	return
		t == TYPE_HORIZONTAL ? json_string("horizontal") :
		t == TYPE_VERTICAL ? json_string("vertical") :
		json_null();
}
json_t* query_split_mode_json(split_mode_t m)
{
	return
		m == MODE_AUTOMATIC ? json_string("automatic") :
		m == MODE_MANUAL ? json_string("manual") :
		json_null();
}
json_t* query_split_direction_json(direction_t d)
{
	return
		d == DIR_RIGHT ? json_string("right") :
		d == DIR_DOWN ? json_string("down") :
		d == DIR_LEFT ? json_string("left") :
		d == DIR_UP ? json_string("up") :
		json_null();
}

json_t* query_node_json(node_t *n)
{
	return json_pack(
		"{"
			"s:{"
				"s:o,"
				"s:f,"
				"s:o,"
				"s:o,"
			"},"
			"s:i,"
			"s:o,"
			"s:b,"
			"s:i,"
			"s:o,"
			"s:o,"
			"s:o,"
			"s:b"
		"}",
			"split",
				"type", query_split_type_json(n->split_type),
				"ratio", n->split_ratio,
				"mode", query_split_mode_json(n->split_mode),
				"direction", query_split_direction_json(n->split_dir),
			"birthRotation", n->birth_rotation,
			"rectangle", query_rectangle_json(n->rectangle),
			"vacant", n->vacant,
			"privacyLevel", n->privacy_level,
			"firstChild", n->first_child != NULL ? query_node_json(n->first_child) : json_null(),
			"secondChild", n->second_child != NULL ? query_node_json(n->second_child) : json_null(),
			"client", is_leaf(n) ? query_client_json(n->client) : json_null(),
			"focused", n == mon->desk->focus ? true : false
	);
}

json_t* query_layout_json(layout_t l)
{
	return
		l == LAYOUT_TILED ? json_string("tiled") :
		l == LAYOUT_MONOCLE ? json_string("monocle") :
		json_null();
}

json_t* query_desktop_json(desktop_t *d)
{
	return json_pack(
		"{"
			"s:s,"
			"s:o,"
			"s:o,"
			"s:i,"
			"s:i,"
			"s:{"
				"s:i,"
				"s:i,"
				"s:i,"
				"s:i"
			"},"
			"s:b"
		"}",
			"name", d->name,
			"layout", query_layout_json(d->layout),
			"nodeRoot", d->root != NULL ? query_node_json(d->root) : json_null(),
			"borderWidth", d->border_width,
			"windowGap", d->window_gap,
			"padding",
				"top", d->top_padding,
				"right", d->right_padding,
				"bottom", d->bottom_padding,
				"left", d->left_padding,
			"focused", d == mon->desk ? true : false
	);
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

json_t* query_windows_json(coordinates_t loc)
{
	json_t *jwindows = json_array();
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		if (loc.monitor != NULL && m != loc.monitor)
			continue;
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			if (loc.desktop != NULL && d != loc.desktop)
				continue;
			for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root)) {
				if (loc.node != NULL && n != loc.node)
					continue;
				json_array_append_new(jwindows, json_integer(n->client->window));
			}
		}
	}
	return jwindows;
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

bool node_from_desc(char *desc, coordinates_t *ref, coordinates_t *dst)
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

bool desktop_from_desc(char *desc, coordinates_t *ref, coordinates_t *dst)
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

bool monitor_from_desc(char *desc, coordinates_t *ref, coordinates_t *dst)
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

bool locate_desktop(char *name, coordinates_t *loc)
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

bool locate_monitor(char *name, coordinates_t *loc)
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
