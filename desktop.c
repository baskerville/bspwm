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
#include <string.h>
#include "bspwm.h"
#include "ewmh.h"
#include "history.h"
#include "monitor.h"
#include "query.h"
#include "tree.h"
#include "desktop.h"
#include "subscribe.h"
#include "settings.h"

void focus_desktop(monitor_t *m, desktop_t *d)
{
	focus_monitor(m);

	show_desktop(d);
	if (m->desk != d) {
		hide_desktop(m->desk);
	}

	m->desk = d;
	ewmh_update_current_desktop();

	put_status(SBSC_MASK_DESKTOP_FOCUS, "desktop_focus %s %s\n", m->name, d->name);
}

void activate_desktop(monitor_t *m, desktop_t *d)
{
	if (d == m->desk) {
		return;
	}

	show_desktop(d);
	hide_desktop(m->desk);

	m->desk = d;

	put_status(SBSC_MASK_DESKTOP_ACTIVATE, "desktop_activate %s %s\n", m->name, d->name);
	put_status(SBSC_MASK_REPORT);
}

desktop_t *closest_desktop(monitor_t *m, desktop_t *d, cycle_dir_t dir, desktop_select_t sel)
{
	desktop_t *f = (dir == CYCLE_PREV ? d->prev : d->next);
	if (f == NULL) {
		f = (dir == CYCLE_PREV ? m->desk_tail : m->desk_head);
	}

	while (f != d) {
		coordinates_t loc = {m, f, NULL};
		if (desktop_matches(&loc, &loc, sel)) {
			return f;
		}
		f = (dir == CYCLE_PREV ? f->prev : f->next);
		if (f == NULL) {
			f = (dir == CYCLE_PREV ? m->desk_tail : m->desk_head);
		}
	}

	return NULL;
}

void change_layout(monitor_t *m, desktop_t *d, layout_t l)
{
	d->layout = l;
	arrange(m, d);

	put_status(SBSC_MASK_DESKTOP_LAYOUT, "desktop_layout %s %s %s\n", m->name, d->name, l==LAYOUT_TILED?"tiled":"monocle");

	if (d == m->desk) {
		put_status(SBSC_MASK_REPORT);
	}
}

bool transfer_desktop(monitor_t *ms, monitor_t *md, desktop_t *d)
{
	if (ms == NULL || md == NULL || d == NULL || ms == md) {
		return false;
	}

	bool was_active = (d == ms->desk);

	unlink_desktop(ms, d);

	if (ms->sticky_count > 0 && was_active && ms->desk != NULL) {
		sticky_still = false;
		transfer_sticky_nodes(ms, d, ms->desk, d->root);
		sticky_still = true;
	}

	if (md->desk != NULL) {
		hide_desktop(d);
	}

	insert_desktop(md, d);

	history_transfer_desktop(md, d);
	adapt_geometry(&ms->rectangle, &md->rectangle, d->root);
	arrange(md, d);

	if (was_active && ms->desk != NULL) {
		if (mon == ms) {
			focus_node(ms, ms->desk, ms->desk->focus);
		} else {
			activate_node(ms, ms->desk, ms->desk->focus);
		}
	}

	if (md->desk == d) {
		if (mon == md) {
			focus_node(md, d, d->focus);
		} else {
			activate_node(md, d, d->focus);
		}
	}

	ewmh_update_wm_desktops();
	ewmh_update_desktop_names();
	ewmh_update_current_desktop();

	put_status(SBSC_MASK_DESKTOP_TRANSFER, "desktop_transfer %s %s %s\n", ms->name, d->name, md->name);
	put_status(SBSC_MASK_REPORT);

	return true;
}

desktop_t *make_desktop(const char *name)
{
	desktop_t *d = malloc(sizeof(desktop_t));
	if (name == NULL) {
		snprintf(d->name, sizeof(d->name), "%s%d", DEFAULT_DESK_NAME, ++desktop_uid);
	} else {
		snprintf(d->name, sizeof(d->name), "%s", name);
	}
	d->prev = d->next = NULL;
	d->root = d->focus = NULL;
	initialize_desktop(d);
	return d;
}

void rename_desktop(monitor_t *m, desktop_t *d, const char *name)
{

	put_status(SBSC_MASK_DESKTOP_RENAME, "desktop_rename %s %s %s\n", m->name, d->name, name);

	snprintf(d->name, sizeof(d->name), "%s", name);

	put_status(SBSC_MASK_REPORT);
	ewmh_update_desktop_names();
}

void initialize_desktop(desktop_t *d)
{
	d->layout = LAYOUT_TILED;
	d->top_padding = d->right_padding = d->bottom_padding = d->left_padding = 0;
	d->window_gap = window_gap;
	d->border_width = border_width;
}

void insert_desktop(monitor_t *m, desktop_t *d)
{
	if (m->desk == NULL) {
		m->desk = d;
		m->desk_head = d;
		m->desk_tail = d;
	} else {
		m->desk_tail->next = d;
		d->prev = m->desk_tail;
		m->desk_tail = d;
	}
}

void add_desktop(monitor_t *m, desktop_t *d)
{
	put_status(SBSC_MASK_DESKTOP_ADD, "desktop_add %s %s\n", m->name, d->name);

	insert_desktop(m, d);
	ewmh_update_number_of_desktops();
	ewmh_update_desktop_names();
	ewmh_update_wm_desktops();
	put_status(SBSC_MASK_REPORT);
}

desktop_t *find_desktop_in(const char *name, monitor_t *m)
{
	if (m == NULL) {
		return NULL;
	}

	for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
		if (streq(d->name, name)) {
			return d;
		}
	}

	return NULL;
}

void empty_desktop(monitor_t *m, desktop_t *d)
{
	destroy_tree(m, d, d->root);
	d->root = d->focus = NULL;
}

void unlink_desktop(monitor_t *m, desktop_t *d)
{
	desktop_t *prev = d->prev;
	desktop_t *next = d->next;

	if (prev != NULL) {
		prev->next = next;
	}

	if (next != NULL) {
		next->prev = prev;
	}

	if (m->desk_head == d) {
		m->desk_head = next;
	}

	if (m->desk_tail == d) {
		m->desk_tail = prev;
	}

	if (m->desk == d) {
		m->desk = history_last_desktop(m, d);
		if (m->desk == NULL) {
			m->desk = (prev == NULL ? next : prev);
		}
	}

	d->prev = d->next = NULL;
}

void remove_desktop(monitor_t *m, desktop_t *d)
{
	put_status(SBSC_MASK_DESKTOP_REMOVE, "desktop_remove %s %s\n", m->name, d->name);

	bool was_focused = (mon != NULL && d == mon->desk);
	bool was_active = (d == m->desk);
	history_remove(d, NULL, false);
	unlink_desktop(m, d);
	empty_desktop(m, d);
	free(d);

	ewmh_update_current_desktop();
	ewmh_update_number_of_desktops();
	ewmh_update_desktop_names();

	if (mon != NULL && m->desk != NULL) {
		if (was_focused) {
			update_focused();
		} else if (was_active) {
			activate_node(m, m->desk, m->desk->focus);
		}
	}

	put_status(SBSC_MASK_REPORT);
}

void merge_desktops(monitor_t *ms, desktop_t *ds, monitor_t *md, desktop_t *dd)
{
	if (ds == NULL || dd == NULL || ds == dd) {
		return;
	}
	node_t *n = first_extrema(ds->root);
	while (n != NULL) {
		node_t *next = next_leaf(n, ds->root);
		transfer_node(ms, ds, n, md, dd, dd->focus);
		n = next;
	}
}

bool swap_desktops(monitor_t *m1, desktop_t *d1, monitor_t *m2, desktop_t *d2)
{
	if (d1 == NULL || d2 == NULL || d1 == d2 ||
	    (m1->desk == d1 && m1->sticky_count > 0) ||
	    (m2->desk == d2 && m2->sticky_count > 0)) {
		return false;
	}

	put_status(SBSC_MASK_DESKTOP_SWAP, "desktop_swap %s %s %s %s\n", m1->name, d1->name, m2->name, d2->name);

	bool d1_focused = (m1->desk == d1);
	bool d2_focused = (m2->desk == d2);

	if (m1 != m2) {
		if (m1->desk == d1) {
			m1->desk = d2;
		}
		if (m1->desk_head == d1) {
			m1->desk_head = d2;
		}
		if (m1->desk_tail == d1) {
			m1->desk_tail = d2;
		}
		if (m2->desk == d2) {
			m2->desk = d1;
		}
		if (m2->desk_head == d2) {
			m2->desk_head = d1;
		}
		if (m2->desk_tail == d2) {
			m2->desk_tail = d1;
		}
	} else {
		if (m1->desk == d1) {
			m1->desk = d2;
		} else if (m1->desk == d2) {
			m1->desk = d1;
		}
		if (m1->desk_head == d1) {
			m1->desk_head = d2;
		} else if (m1->desk_head == d2) {
			m1->desk_head = d1;
		}
		if (m1->desk_tail == d1) {
			m1->desk_tail = d2;
		} else if (m1->desk_tail == d2) {
			m1->desk_tail = d1;
		}
	}

	desktop_t *p1 = d1->prev;
	desktop_t *n1 = d1->next;
	desktop_t *p2 = d2->prev;
	desktop_t *n2 = d2->next;

	if (p1 != NULL && p1 != d2) {
		p1->next = d2;
	}
	if (n1 != NULL && n1 != d2) {
		n1->prev = d2;
	}
	if (p2 != NULL && p2 != d1) {
		p2->next = d1;
	}
	if (n2 != NULL && n2 != d1) {
		n2->prev = d1;
	}

	d1->prev = p2 == d1 ? d2 : p2;
	d1->next = n2 == d1 ? d2 : n2;
	d2->prev = p1 == d2 ? d1 : p1;
	d2->next = n1 == d2 ? d1 : n1;

	if (m1 != m2) {
		adapt_geometry(&m1->rectangle, &m2->rectangle, d1->root);
		adapt_geometry(&m2->rectangle, &m1->rectangle, d2->root);
		history_swap_desktops(m1, d1, m2, d2);
		arrange(m1, d2);
		arrange(m2, d1);
	}

	if (d1_focused && !d2_focused) {
		hide_desktop(d1);
		show_desktop(d2);
	} else if (!d1_focused && d2_focused) {
		show_desktop(d1);
		hide_desktop(d2);
	}

	if (d1 == mon->desk) {
		focus_node(m2, d1, d1->focus);
	} else if (d1 == m2->desk) {
		activate_node(m2, d1, d1->focus);
	}

	if (d2 == mon->desk) {
		focus_node(m1, d2, d2->focus);
	} else if (d2 == m1->desk) {
		activate_node(m1, d2, d2->focus);
	}

	ewmh_update_wm_desktops();
	ewmh_update_desktop_names();
	ewmh_update_current_desktop();

	put_status(SBSC_MASK_REPORT);

	return true;
}

void show_desktop(desktop_t *d)
{
	if (d == NULL) {
		return;
	}
	show_node(d->root);
}

void hide_desktop(desktop_t *d)
{
	if (d == NULL) {
		return;
	}
	hide_node(d->root);
}

bool is_urgent(desktop_t *d)
{
	for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root)) {
		if (n->client->urgent) {
			return true;
		}
	}
	return false;
}
