/* Copyright (c) 2012-2014, Bastien Dejean
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
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */

#include <stdio.h>
#include <strings.h>
#include <string.h>
#include "bspwm.h"
#include "desktop.h"
#include "history.h"
#include "messages.h"
#include "monitor.h"
#include "tree.h"
#include "query.h"

void query_monitors(coordinates_t loc, domain_t dom, FILE *rsp)
{
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		if (loc.monitor != NULL && m != loc.monitor)
			continue;
		if (dom != DOMAIN_DESKTOP) {
			if (dom == DOMAIN_MONITOR) {
				fprintf(rsp, "%s\n", m->name);
				continue;
			} else {
				fprintf(rsp, "%s %ux%u%+i%+i %i,%i,%i,%i%s\n", m->name,
				         m->rectangle.width,m->rectangle.height, m->rectangle.x, m->rectangle.y,
				         m->top_padding, m->right_padding, m->bottom_padding, m->left_padding,
				         (m == mon ? " *" : ""));
			}
		}
		query_desktops(m, dom, loc, (dom == DOMAIN_DESKTOP ? 0 : 1), rsp);
	}
}

void query_desktops(monitor_t *m, domain_t dom, coordinates_t loc, unsigned int depth, FILE *rsp)
{
	for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
		if (loc.desktop != NULL && d != loc.desktop)
			continue;
		for (unsigned int i = 0; i < depth; i++)
			fprintf(rsp, "\t");
		if (dom == DOMAIN_DESKTOP) {
			fprintf(rsp, "%s\n", d->name);
			continue;
		} else {
			fprintf(rsp, "%s %u %i %i,%i,%i,%i %c %c%s\n", d->name, d->border_width,
			        d->window_gap,
			        d->top_padding, d->right_padding, d->bottom_padding, d->left_padding,
			        (d->layout == LAYOUT_TILED ? 'T' : 'M'), (d->floating ? 'f' : '-'),
			        (d == m->desk ? " *" : ""));
		}
		query_tree(d, d->root, rsp, depth + 1);
	}
}

void query_tree(desktop_t *d, node_t *n, FILE *rsp, unsigned int depth)
{
	if (n == NULL)
		return;

	for (unsigned int i = 0; i < depth; i++)
		fprintf(rsp, "\t");

	if (is_leaf(n)) {
		client_t *c = n->client;
		fprintf(rsp, "%c %s %s 0x%X %u %ux%u%+i%+i %c %c%c%c%c%c%c%c%c%s\n",
		         (n->birth_rotation == 90 ? 'a' : (n->birth_rotation == 270 ? 'c' : 'm')),
		         c->class_name, c->instance_name, c->window, c->border_width,
		         c->floating_rectangle.width, c->floating_rectangle.height,
		         c->floating_rectangle.x, c->floating_rectangle.y,
		         (n->split_dir == DIR_UP ? 'U' : (n->split_dir == DIR_RIGHT ? 'R' : (n->split_dir == DIR_DOWN ? 'D' : 'L'))),
		         (c->floating ? 'f' : '-'), (c->pseudo_tiled ? 'd' : '-'), (c->fullscreen ? 'F' : '-'),
		         (c->urgent ? 'u' : '-'), (c->locked ? 'l' : '-'), (c->sticky ? 's' : '-'),
		         (c->private ? 'i' : '-'), (n->split_mode ? 'p' : '-'),
		         (n == d->focus ? " *" : ""));
	} else {
		fprintf(rsp, "%c %c %lf\n", (n->split_type == TYPE_HORIZONTAL ? 'H' : 'V'),
		        (n->birth_rotation == 90 ? 'a' : (n->birth_rotation == 270 ? 'c' : 'm')), n->split_ratio);
	}

	query_tree(d, n->first_child, rsp, depth + 1);
	query_tree(d, n->second_child, rsp, depth + 1);
}

void query_history(coordinates_t loc, FILE *rsp)
{
	for (history_t *h = history_head; h != NULL; h = h->next) {
		if ((loc.monitor != NULL && h->loc.monitor != loc.monitor)
				|| (loc.desktop != NULL && h->loc.desktop != loc.desktop))
			continue;
		xcb_window_t win = XCB_NONE;
		if (h->loc.node != NULL)
			win = h->loc.node->client->window;
		fprintf(rsp, "%s %s 0x%X\n", h->loc.monitor->name, h->loc.desktop->name, win);
	}
}

void query_stack(FILE *rsp)
{
	for (stacking_list_t *s = stack_head; s != NULL; s = s->next)
		fprintf(rsp, "0x%X\n", s->node->client->window);
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

bool node_from_desc(char *desc, coordinates_t *ref, coordinates_t *dst)
{
	client_select_t sel = {CLIENT_TYPE_ALL, CLIENT_CLASS_ALL, CLIENT_MODE_ALL, false, false};
	char *tok;
	while ((tok = strrchr(desc, CAT_CHR)) != NULL) {
		tok[0] = '\0';
		tok++;
		if (streq("tiled", tok)) {
			sel.type = CLIENT_TYPE_TILED;
		} else if (streq("floating", tok)) {
			sel.type = CLIENT_TYPE_FLOATING;
		} else if (streq("like", tok)) {
			sel.class = CLIENT_CLASS_EQUAL;
		} else if (streq("unlike", tok)) {
			sel.class = CLIENT_CLASS_DIFFER;
		} else if (streq("manual", tok)) {
			sel.mode = CLIENT_MODE_MANUAL;
		} else if (streq("automatic", tok)) {
			sel.mode = CLIENT_MODE_AUTOMATIC;
		} else if (streq("urgent", tok)) {
			sel.urgent = true;
		} else if (streq("local", tok)) {
			sel.local = true;
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
			monitor_t *m = nearest_monitor(ref->monitor, dir, (desktop_select_t) {DESKTOP_STATUS_ALL, false, false});
			if (m != NULL) {
				dst->monitor = m;
				dst->desktop = m->desk;
				dst->node = m->desk->focus;
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

	return (dst->node != NULL || dst->desktop != ref->desktop);
}

bool desktop_from_desc(char *desc, coordinates_t *ref, coordinates_t *dst)
{
	desktop_select_t sel = {DESKTOP_STATUS_ALL, false, false};
	char *tok;
	while ((tok = strrchr(desc, CAT_CHR)) != NULL) {
		tok[0] = '\0';
		tok++;
		if (streq("free", tok)) {
			sel.status = DESKTOP_STATUS_FREE;
		} else if (streq("occupied", tok)) {
			sel.status = DESKTOP_STATUS_OCCUPIED;
		} else if (streq("urgent", tok)) {
			sel.urgent = true;
		} else if (streq("local", tok)) {
			sel.local = true;
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
	desktop_select_t sel = {DESKTOP_STATUS_ALL, false, false};
	char *tok;
	while ((tok = strrchr(desc, CAT_CHR)) != NULL) {
		tok[0] = '\0';
		tok++;
		if (streq("free", tok)) {
			sel.status = DESKTOP_STATUS_FREE;
		} else if (streq("occupied", tok)) {
			sel.status = DESKTOP_STATUS_OCCUPIED;
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
	if (ref->node == NULL || loc->node == NULL)
		return false;

	if (sel.type != CLIENT_TYPE_ALL &&
	    is_tiled(loc->node->client)
	    ? sel.type == CLIENT_TYPE_FLOATING
	    : sel.type == CLIENT_TYPE_TILED)
		return false;

	if (sel.class != CLIENT_CLASS_ALL &&
	    streq(loc->node->client->class_name, ref->node->client->class_name)
	    ? sel.class == CLIENT_CLASS_DIFFER
	    : sel.class == CLIENT_CLASS_EQUAL)
		return false;

	if (sel.mode != CLIENT_MODE_ALL &&
	    loc->node->split_mode == MODE_MANUAL
	    ? sel.mode == CLIENT_MODE_AUTOMATIC
	    : sel.mode == CLIENT_MODE_MANUAL)
		return false;

	if (sel.local && loc->desktop != ref->desktop)
		return false;

	if (sel.urgent && !loc->node->client->urgent)
		return false;

	return true;
}

bool desktop_matches(coordinates_t *loc, coordinates_t *ref, desktop_select_t sel)
{
	if (sel.status != DESKTOP_STATUS_ALL &&
	    loc->desktop->root == NULL
	    ? sel.status == DESKTOP_STATUS_OCCUPIED
	    : sel.status == DESKTOP_STATUS_FREE)
		return false;

	if (sel.urgent && !is_urgent(loc->desktop))
		return false;

	if (sel.local && ref->monitor != loc->monitor)
		return false;

	return true;
}
