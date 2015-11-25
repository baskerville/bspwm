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
#include "parse.h"
#include "monitor.h"
#include "tree.h"
#include "query.h"
#include "jsmn.h"

void query_tree(FILE *rsp)
{
	fprintf(rsp, "{");
	fprintf(rsp, "\"focusedMonitorName\": \"%s\", ", mon->name);
	fprintf(rsp, "\"numClients\": %i, ", num_clients);
	fprintf(rsp, "\"monitors\": ");
	fprintf(rsp, "[");
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		query_monitor(m, rsp);
		if (m->next != NULL) {
			fprintf(rsp, ", ");
		}
	}
	fprintf(rsp, "]");
	fprintf(rsp, "}");

}

void query_monitor(monitor_t *m, FILE *rsp)
{
	fprintf(rsp, "{");
	fprintf(rsp, "\"name\": \"%s\", ", m->name);
	fprintf(rsp, "\"id\": %u, ", m->id);
	fprintf(rsp, "\"wired\": %s, ", BOOL_STR(m->wired));
	fprintf(rsp, "\"topPadding\": %i, ", m->top_padding);
	fprintf(rsp, "\"rightPadding\": %i, ", m->right_padding);
	fprintf(rsp, "\"bottomPadding\": %i, ", m->bottom_padding);
	fprintf(rsp, "\"leftPadding\": %i, ", m->left_padding);
	fprintf(rsp, "\"numSticky\": %i, ", m->num_sticky);
	fprintf(rsp, "\"rectangle\": ");
	query_rectangle(m->rectangle, rsp);
	fprintf(rsp, ", ");
	fprintf(rsp, "\"focusedDesktopName\": \"%s\", ", m->desk->name);
	fprintf(rsp, "\"desktops\": ");
	fprintf(rsp, "[");
	for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
		query_desktop(d, rsp);
		if (d->next != NULL) {
			fprintf(rsp, ", ");
		}
	}
	fprintf(rsp, "]");
	fprintf(rsp, "}");
}

void query_desktop(desktop_t *d, FILE *rsp)
{
	fprintf(rsp, "{");
	fprintf(rsp, "\"name\": \"%s\", ", d->name);
	fprintf(rsp, "\"layout\": \"%s\", ", LAYOUT_STR(d->layout));
	fprintf(rsp, "\"topPadding\": %i, ", d->top_padding);
	fprintf(rsp, "\"rightPadding\": %i, ", d->right_padding);
	fprintf(rsp, "\"bottomPadding\": %i, ", d->bottom_padding);
	fprintf(rsp, "\"leftPadding\": %i, ", d->left_padding);
	fprintf(rsp, "\"windowGap\": %i, ", d->window_gap);
	fprintf(rsp, "\"borderWidth\": %u, ", d->border_width);
	fprintf(rsp, "\"focusedWindow\": %u, ", d->focus != NULL ? d->focus->client->window : 0);
	fprintf(rsp, "\"root\": ");
	query_node(d->root, rsp);
	fprintf(rsp, "}");
}

void query_node(node_t *n, FILE *rsp)
{
	if (n == NULL) {
		fprintf(rsp, "null");
	} else {
		fprintf(rsp, "{");
		fprintf(rsp, "\"splitType\": \"%s\", ", SPLIT_TYPE_STR(n->split_type));
		fprintf(rsp, "\"splitRatio\": %lf, ", n->split_ratio);
		fprintf(rsp, "\"splitMode\": \"%s\", ", SPLIT_MODE_STR(n->split_mode));
		fprintf(rsp, "\"splitDir\": \"%s\", ", SPLIT_DIR_STR(n->split_dir));
		fprintf(rsp, "\"birthRotation\": %i, ", n->birth_rotation);
		fprintf(rsp, "\"privacyLevel\": %i, ", n->privacy_level);
		fprintf(rsp, "\"vacant\": %s, ", BOOL_STR(n->vacant));
		fprintf(rsp, "\"rectangle\": ");
		query_rectangle(n->rectangle, rsp);
		fprintf(rsp, ", ");
		fprintf(rsp, "\"firstChild\": ");
		query_node(n->first_child, rsp);
		fprintf(rsp, ", ");
		fprintf(rsp, "\"secondChild\": ");
		query_node(n->second_child, rsp);
		fprintf(rsp, ", ");
		fprintf(rsp, "\"client\": ");
		query_client(n->client, rsp);
		fprintf(rsp, "}");
	}
}

void query_client(client_t *c, FILE *rsp)
{
	if (c == NULL) {
		fprintf(rsp, "null");
	} else {
		fprintf(rsp, "{");
		fprintf(rsp, "\"window\": %u, ", c->window);
		fprintf(rsp, "\"className\": \"%s\", ", c->class_name);
		fprintf(rsp, "\"instanceName\": \"%s\", ", c->instance_name);
		fprintf(rsp, "\"borderWidth\": %u, ", c->border_width);
		fprintf(rsp, "\"state\": \"%s\", ", STATE_STR(c->state));
		fprintf(rsp, "\"lastState\": \"%s\", ", STATE_STR(c->last_state));
		fprintf(rsp, "\"layer\": \"%s\", ", LAYER_STR(c->layer));
		fprintf(rsp, "\"lastLayer\": \"%s\", ", LAYER_STR(c->last_layer));
		fprintf(rsp, "\"locked\": %s, ", BOOL_STR(c->locked));
		fprintf(rsp, "\"sticky\": %s, ", BOOL_STR(c->sticky));
		fprintf(rsp, "\"urgent\": %s, ", BOOL_STR(c->urgent));
		fprintf(rsp, "\"private\": %s, ", BOOL_STR(c->private));
		fprintf(rsp, "\"icccmFocus\": %s, ", BOOL_STR(c->icccm_focus));
		fprintf(rsp, "\"icccmInput\": %s, ", BOOL_STR(c->icccm_input));
		fprintf(rsp, "\"minWidth\": %u, ", c->min_width);
		fprintf(rsp, "\"maxWidth\": %u, ", c->max_width);
		fprintf(rsp, "\"minHeight\": %u, ", c->min_height);
		fprintf(rsp, "\"maxHeight\": %u, ", c->max_height);
		fprintf(rsp, "\"numStates\": %i, ", c->num_states);
		fprintf(rsp, "\"wmState\": ");
		query_wm_state(c->wm_state, c->num_states, rsp);
		fprintf(rsp, ", ");
		fprintf(rsp, "\"tiledRectangle\": ");
		query_rectangle(c->tiled_rectangle, rsp);
		fprintf(rsp, ", ");
		fprintf(rsp, "\"floatingRectangle\": ");
		query_rectangle(c->floating_rectangle, rsp);
		fprintf(rsp, "}");
	}
}

void query_rectangle(xcb_rectangle_t r, FILE *rsp)
{
		fprintf(rsp, "{\"x\": %i, \"y\": %i, \"width\": %u, \"height\": %u}", r.x, r.y, r.width, r.height);
}

void query_wm_state(xcb_atom_t *wm_state, int num_states, FILE *rsp)
{
	fprintf(rsp, "[");
	for (int i = 0; i < num_states; i++) {
		fprintf(rsp, "%u", wm_state[i]);
		if (i < num_states - 1) {
			fprintf(rsp, ", ");
		}
	}
	fprintf(rsp, "]");
}

void query_history(coordinates_t loc, FILE *rsp)
{
	for (history_t *h = history_head; h != NULL; h = h->next) {
		if ((loc.monitor != NULL && h->loc.monitor != loc.monitor)
		    || (loc.desktop != NULL && h->loc.desktop != loc.desktop)) {
			continue;
		}
		xcb_window_t win = XCB_NONE;
		if (h->loc.node != NULL) {
			win = h->loc.node->client->window;
		}
		fprintf(rsp, "%s %s 0x%X\n", h->loc.monitor->name, h->loc.desktop->name, win);
	}
}

void query_stack(FILE *rsp)
{
	for (stacking_list_t *s = stack_head; s != NULL; s = s->next) {
		fprintf(rsp, "0x%X\n", s->node->client->window);
	}
}

void query_windows(coordinates_t loc, FILE *rsp)
{
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		if (loc.monitor != NULL && m != loc.monitor)
			continue;
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			if (loc.desktop != NULL && d != loc.desktop)
				continue;
			for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root)) {
				if (loc.node != NULL && n != loc.node)
					continue;
				fprintf(rsp, "0x%X\n", n->client->window);
			}
		}
	}
}

void query_names(domain_t dom, coordinates_t loc, FILE *rsp)
{
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		if (loc.monitor != NULL && m != loc.monitor) {
			continue;
		}
		if (dom == DOMAIN_MONITOR) {
			fprintf(rsp, "%s\n", m->name);
		}
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			if (loc.desktop != NULL && d != loc.desktop) {
				continue;
			}
			if (dom == DOMAIN_DESKTOP) {
				fprintf(rsp, "%s\n", d->name);
			}
		}
	}
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
		.below = OPTION_NONE,
		.normal = OPTION_NONE,
		.above = OPTION_NONE
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

bool node_from_desc(char *desc, coordinates_t *ref, coordinates_t *dst)
{
	client_select_t sel = make_client_select();
	char *tok;
	while ((tok = strrchr(desc, CAT_CHR)) != NULL) {
		tok[0] = '\0';
		tok++;
#define GET_MOD(k) \
	} else if (streq(#k, tok)) { \
		sel.k = OPTION_TRUE; \
	} else if (streq("!" #k, tok)) { \
		sel.k = OPTION_FALSE;
		if (streq("tiled", tok)) {
			sel.tiled = OPTION_TRUE;
		} else if (streq("!tiled", tok)) {
			sel.tiled = OPTION_FALSE;
		GET_MOD(pseudo_tiled)
		GET_MOD(floating)
		GET_MOD(fullscreen)
		GET_MOD(locked)
		GET_MOD(sticky)
		GET_MOD(private)
		GET_MOD(urgent)
		GET_MOD(same_class)
		GET_MOD(automatic)
		GET_MOD(local)
		GET_MOD(focused)
		GET_MOD(below)
		GET_MOD(normal)
		GET_MOD(above)
		}
	}
#undef GET_MOD

	dst->monitor = ref->monitor;
	dst->desktop = ref->desktop;
	dst->node = NULL;

	direction_t dir;
	cycle_dir_t cyc;
	history_dir_t hdi;
	if (parse_direction(desc, &dir)) {
		dst->node = nearest_neighbor(ref->monitor, ref->desktop, ref->node, dir, sel);
		if (dst->node == NULL && mon_head != mon_tail) {
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
		if (parse_window_id(desc, &wid) && locate_window(wid, dst)) {
			return node_matches(dst, ref, sel);
		}
	}

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
				coordinates_t loc = {dst->monitor, dst->monitor->desk, NULL};
				if (desktop_matches(&loc, ref, sel)) {
					dst->desktop = dst->monitor->desk;
				}
			} else if (parse_index(colon + 1, &idx)) {
				if (desktop_from_index(idx, dst, dst->monitor)) {
					return desktop_matches(dst, ref, sel);
				}
			}
		}
	} else if (parse_index(desc, &idx)) {
		if (desktop_from_index(idx, dst, NULL)) {
			return desktop_matches(dst, ref, sel);
		}
	} else {
		if (locate_desktop(desc, dst)) {
			return desktop_matches(dst, ref, sel);
		}
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
			if (desktop_matches(&loc, ref, sel)) {
				dst->monitor = pri_mon;
			}
		}
	} else if (streq("focused", desc)) {
		coordinates_t loc = {mon, mon->desk, NULL};
		if (desktop_matches(&loc, ref, sel)) {
			dst->monitor = mon;
		}
	} else if (parse_index(desc, &idx)) {
		if (monitor_from_index(idx, dst)) {
			coordinates_t loc = {dst->monitor, dst->monitor->desk, NULL};
			return desktop_matches(&loc, ref, sel);
		}
	} else {
		if (locate_monitor(desc, dst)) {
			coordinates_t loc = {dst->monitor, dst->monitor->desk, NULL};
			return desktop_matches(&loc, ref, sel);
		}
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

	if (sel.below != OPTION_NONE &&
	    loc->node->client->layer != LAYER_BELOW
	    ? sel.below == OPTION_TRUE
	    : sel.below == OPTION_FALSE) {
		return false;
	}

	if (sel.normal != OPTION_NONE &&
	    loc->node->client->layer != LAYER_NORMAL
	    ? sel.normal == OPTION_TRUE
	    : sel.normal == OPTION_FALSE) {
		return false;
	}

	if (sel.above != OPTION_NONE &&
	    loc->node->client->layer != LAYER_ABOVE
	    ? sel.above == OPTION_TRUE
	    : sel.above == OPTION_FALSE) {
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
