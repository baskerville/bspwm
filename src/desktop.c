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
#include "bspwm.h"
#include "ewmh.h"
#include "history.h"
#include "monitor.h"
#include "query.h"
#include "tree.h"
#include "window.h"
#include "desktop.h"
#include "subscribe.h"
#include "settings.h"

bool activate_desktop(monitor_t *m, desktop_t *d)
{
	if (d != NULL && m == mon) {
		return false;
	}

	if (d == NULL) {
		d = m->desk;
		if (d == NULL) {
			d = history_last_desktop(m, NULL);
		}
		if (d == NULL) {
			d = m->desk_head;
		}
	}

	if (d == NULL || d == m->desk) {
		return false;
	}

	if (m->sticky_count > 0 && m->desk != NULL) {
		transfer_sticky_nodes(m, m->desk, m, d, m->desk->root);
	}

	show_desktop(d);
	hide_desktop(m->desk);

	m->desk = d;

	history_add(m, d, NULL, false);

	put_status(SBSC_MASK_DESKTOP_ACTIVATE, "desktop_activate 0x%08X 0x%08X\n", m->id, d->id);
	put_status(SBSC_MASK_REPORT);

	return true;
}

bool find_closest_desktop(coordinates_t *ref, coordinates_t *dst, cycle_dir_t dir, desktop_select_t *sel)
{
	monitor_t *m = ref->monitor;
	desktop_t *d = ref->desktop;
	d = (dir == CYCLE_PREV ? d->prev : d->next);

#define HANDLE_BOUNDARIES(m, d)  \
	if (d == NULL) { \
		m = (dir == CYCLE_PREV ? m->prev : m->next); \
		if (m == NULL) { \
			m = (dir == CYCLE_PREV ? mon_tail : mon_head); \
		} \
		d = (dir == CYCLE_PREV ? m->desk_tail : m->desk_head); \
	}
	HANDLE_BOUNDARIES(m, d)

	while (d != ref->desktop) {
		coordinates_t loc = {m, d, NULL};
		if (desktop_matches(&loc, ref, sel)) {
			*dst = loc;
			return true;
		}
		d = (dir == CYCLE_PREV ? d->prev : d->next);
		HANDLE_BOUNDARIES(m, d)
	}
#undef HANDLE_BOUNDARIES

	return false;
}

bool find_any_desktop(coordinates_t *ref, coordinates_t *dst, desktop_select_t *sel)
{
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			coordinates_t loc = {m, d, NULL};
			if (desktop_matches(&loc, ref, sel)) {
				*dst = loc;
				return true;
			}
		}
	}
	return false;
}

bool set_layout(monitor_t *m, desktop_t *d, layout_t l, bool user)
{
	if ((user && d->user_layout == l) || (!user && d->layout == l)) {
		return false;
	}

	layout_t old_layout = d->layout;

	if (user) {
		d->user_layout = l;
	} else {
		d->layout = l;
	}

	if (user && (!single_monocle || tiled_count(d->root, true) > 1)) {
		d->layout = l;
	}

	if (d->layout != old_layout) {
		handle_presel_feedbacks(m, d);

		if (user) {
			arrange(m, d);
		}

		put_status(SBSC_MASK_DESKTOP_LAYOUT, "desktop_layout 0x%08X 0x%08X %s\n", m->id, d->id, LAYOUT_STR(d->layout));

		if (d == m->desk) {
			put_status(SBSC_MASK_REPORT);
		}
	}

	return true;
}

void handle_presel_feedbacks(monitor_t *m, desktop_t *d)
{
	if (m->desk != d) {
		return;
	}
	if (d->layout == LAYOUT_MONOCLE) {
		hide_presel_feedbacks(m, d, d->root);
	} else {
		show_presel_feedbacks(m, d, d->root);
	}
}

bool transfer_desktop(monitor_t *ms, monitor_t *md, desktop_t *d, bool follow)
{
	if (ms == NULL || md == NULL || d == NULL || ms == md) {
		return false;
	}

	bool d_was_active = (d == ms->desk);
	bool ms_was_focused = (ms == mon);

	unlink_desktop(ms, d);

	if ((!follow || !d_was_active || !ms_was_focused) && md->desk != NULL) {
		hide_sticky = false;
		hide_desktop(d);
		hide_sticky = true;
	}

	insert_desktop(md, d);
	history_remove(d, NULL, false);

	if (d_was_active) {
		if (follow) {
			if (activate_desktop(ms, NULL)) {
				activate_node(ms, ms->desk, NULL);
			}
			if (ms_was_focused) {
				focus_node(md, d, d->focus);
			}
		} else {
			if (ms_was_focused) {
				focus_node(ms, ms->desk, NULL);
			} else if (activate_desktop(ms, NULL)) {
				activate_node(ms, ms->desk, NULL);
			}
		}
	}

	if (ms->sticky_count > 0 && d_was_active) {
		if (ms->desk != NULL) {
			transfer_sticky_nodes(md, d, ms, ms->desk, d->root);
		} else {
			ms->sticky_count -= sticky_count(d->root);
			md->sticky_count += sticky_count(d->root);
			if (d != md->desk) {
				transfer_sticky_nodes(md, d, md, md->desk, d->root);
			}
		}
	}

	adapt_geometry(&ms->rectangle, &md->rectangle, d->root);
	arrange(md, d);

	if ((!follow || !d_was_active || !ms_was_focused) && md->desk == d) {
		if (md == mon) {
			focus_node(md, d, d->focus);
		} else {
			activate_node(md, d, d->focus);
		}
	}

	ewmh_update_wm_desktops();
	ewmh_update_desktop_names();
	ewmh_update_desktop_viewport();
	ewmh_update_current_desktop();

	put_status(SBSC_MASK_DESKTOP_TRANSFER, "desktop_transfer 0x%08X 0x%08X 0x%08X\n", ms->id, d->id, md->id);
	put_status(SBSC_MASK_REPORT);

	return true;
}

desktop_t *make_desktop(const char *name, uint32_t id)
{
	desktop_t *d = calloc(1, sizeof(desktop_t));
	snprintf(d->name, sizeof(d->name), "%s", name == NULL ? DEFAULT_DESK_NAME : name);
	if (id == XCB_NONE) {
		d->id = xcb_generate_id(dpy);
	}
	d->prev = d->next = NULL;
	d->root = d->focus = NULL;
	d->user_layout = LAYOUT_TILED;
	d->layout = single_monocle ? LAYOUT_MONOCLE : LAYOUT_TILED;
	d->padding = (padding_t) PADDING;
	d->window_gap = window_gap;
	d->border_width = border_width;
	return d;
}

void rename_desktop(monitor_t *m, desktop_t *d, const char *name)
{

	put_status(SBSC_MASK_DESKTOP_RENAME, "desktop_rename 0x%08X 0x%08X %s %s\n", m->id, d->id, d->name, name);

	snprintf(d->name, sizeof(d->name), "%s", name);

	put_status(SBSC_MASK_REPORT);
	ewmh_update_desktop_names();
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
	put_status(SBSC_MASK_DESKTOP_ADD, "desktop_add 0x%08X 0x%08X %s\n", m->id, d->id, d->name);

	d->border_width = m->border_width;
	d->window_gap = m->window_gap;
	insert_desktop(m, d);
	ewmh_update_current_desktop();
	ewmh_update_number_of_desktops();
	ewmh_update_desktop_names();
	ewmh_update_desktop_viewport();
	ewmh_update_wm_desktops();
	put_status(SBSC_MASK_REPORT);
}

desktop_t *find_desktop_in(uint32_t id, monitor_t *m)
{
	if (m == NULL) {
		return NULL;
	}

	for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
		if (d->id == id) {
			return d;
		}
	}

	return NULL;
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
		m->desk = NULL;
	}

	d->prev = d->next = NULL;
}

void remove_desktop(monitor_t *m, desktop_t *d)
{
	put_status(SBSC_MASK_DESKTOP_REMOVE, "desktop_remove 0x%08X 0x%08X\n", m->id, d->id);

	remove_node(m, d, d->root);
	unlink_desktop(m, d);
	history_remove(d, NULL, false);
	free(d);

	ewmh_update_current_desktop();
	ewmh_update_number_of_desktops();
	ewmh_update_desktop_names();
	ewmh_update_desktop_viewport();

	if (mon != NULL && m->desk == NULL) {
		if (m == mon) {
			focus_node(m, NULL, NULL);
		} else {
			activate_desktop(m, NULL);
			if (m->desk != NULL) {
				activate_node(m, m->desk, m->desk->focus);
			}
		}
	}

	put_status(SBSC_MASK_REPORT);
}

void merge_desktops(monitor_t *ms, desktop_t *ds, monitor_t *md, desktop_t *dd)
{
	if (ds == NULL || dd == NULL || ds == dd) {
		return;
	}
	/* TODO: Handle sticky nodes. */
	transfer_node(ms, ds, ds->root, md, dd, dd->focus, false);
}

bool swap_desktops(monitor_t *m1, desktop_t *d1, monitor_t *m2, desktop_t *d2, bool follow)
{
	if (d1 == NULL || d2 == NULL || d1 == d2 ||
	    (m1->desk == d1 && m1->sticky_count > 0) ||
	    (m2->desk == d2 && m2->sticky_count > 0)) {
		return false;
	}

	put_status(SBSC_MASK_DESKTOP_SWAP, "desktop_swap 0x%08X 0x%08X 0x%08X 0x%08X\n", m1->id, d1->id, m2->id, d2->id);

	bool d1_was_active = (m1->desk == d1);
	bool d2_was_active = (m2->desk == d2);
	bool d1_was_focused = (mon->desk == d1);
	bool d2_was_focused = (mon->desk == d2);

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
		history_remove(d1, NULL, false);
		history_remove(d2, NULL, false);
		arrange(m1, d2);
		arrange(m2, d1);
	}

	if (d1_was_active && !d2_was_active) {
		if ((!follow && m1 != m2) || !d1_was_focused) {
			hide_desktop(d1);
		}
		show_desktop(d2);
	} else if (!d1_was_active && d2_was_active) {
		show_desktop(d1);
		if ((!follow && m1 != m2) || !d2_was_focused) {
			hide_desktop(d2);
		}
	}

	if (follow || m1 == m2) {
		if (d1_was_focused) {
			focus_node(m2, d1, d1->focus);
		} else if (d1_was_active) {
			activate_node(m2, d1, d1->focus);
		}

		if (d2_was_focused) {
			focus_node(m1, d2, d2->focus);
		} else if (d2_was_active) {
			activate_node(m1, d2, d2->focus);
		}
	} else {
		if (d1_was_focused) {
			focus_node(m1, d2, d2->focus);
		} else if (d1_was_active) {
			activate_node(m1, d2, d2->focus);
		}

		if (d2_was_focused) {
			focus_node(m2, d1, d1->focus);
		} else if (d2_was_active) {
			activate_node(m2, d1, d1->focus);
		}
	}

	ewmh_update_wm_desktops();
	ewmh_update_desktop_names();
	ewmh_update_desktop_viewport();
	ewmh_update_current_desktop();

	put_status(SBSC_MASK_REPORT);

	return true;
}

void show_desktop(desktop_t *d)
{
	if (d == NULL) {
		return;
	}
	show_node(d, d->root);
}

void hide_desktop(desktop_t *d)
{
	if (d == NULL) {
		return;
	}
	hide_node(d, d->root);
}

bool is_urgent(desktop_t *d)
{
	for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root)) {
		if (n->client == NULL) {
			continue;
		}
		if (n->client->urgent) {
			return true;
		}
	}
	return false;
}
