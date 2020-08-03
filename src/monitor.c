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

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "bspwm.h"
#include "desktop.h"
#include "ewmh.h"
#include "query.h"
#include "pointer.h"
#include "settings.h"
#include "geometry.h"
#include "tree.h"
#include "subscribe.h"
#include "window.h"
#include "monitor.h"

monitor_t *make_monitor(const char *name, xcb_rectangle_t *rect, uint32_t id)
{
	monitor_t *m = calloc(1, sizeof(monitor_t));
	if (id == XCB_NONE) {
		m->id = xcb_generate_id(dpy);
	}
	m->randr_id = XCB_NONE;
	snprintf(m->name, sizeof(m->name), "%s", name == NULL ? DEFAULT_MON_NAME : name);
	m->padding = padding;
	m->border_width = border_width;
	m->window_gap = window_gap;
	m->root = XCB_NONE;
	m->prev = m->next = NULL;
	m->desk = m->desk_head = m->desk_tail = NULL;
	m->wired = true;
	m->sticky_count = 0;
	if (rect != NULL) {
		update_root(m, rect);
	} else {
		m->rectangle = (xcb_rectangle_t) {0, 0, screen_width, screen_height};
	}
	return m;
}

void update_root(monitor_t *m, xcb_rectangle_t *rect)
{
	xcb_rectangle_t last_rect = m->rectangle;
	m->rectangle = *rect;
	if (m->root == XCB_NONE) {
		uint32_t values[] = {XCB_EVENT_MASK_ENTER_WINDOW};
		m->root = xcb_generate_id(dpy);
		xcb_create_window(dpy, XCB_COPY_FROM_PARENT, m->root, root,
		                  rect->x, rect->y, rect->width, rect->height, 0,
		                  XCB_WINDOW_CLASS_INPUT_ONLY, XCB_COPY_FROM_PARENT, XCB_CW_EVENT_MASK, values);
		xcb_icccm_set_wm_class(dpy, m->root, sizeof(ROOT_WINDOW_IC), ROOT_WINDOW_IC);
		xcb_icccm_set_wm_name(dpy, m->root, XCB_ATOM_STRING, 8, strlen(m->name), m->name);
		window_lower(m->root);
		if (focus_follows_pointer) {
			window_show(m->root);
		}
	} else {
		window_move_resize(m->root, rect->x, rect->y, rect->width, rect->height);
		put_status(SBSC_MASK_MONITOR_GEOMETRY, "monitor_geometry 0x%08X %ux%u+%i+%i\n",
		           m->id, rect->width, rect->height, rect->x, rect->y);
	}
	for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
		for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root)) {
			if (n->client == NULL) {
				continue;
			}
			adapt_geometry(&last_rect, rect, n);
		}
		arrange(m, d);
	}
	reorder_monitor(m);
}

void reorder_monitor(monitor_t *m)
{
	if (m == NULL) {
		return;
	}
	monitor_t *prev = m->prev;
	while (prev != NULL && rect_cmp(m->rectangle, prev->rectangle) < 0) {
		swap_monitors(m, prev);
		prev = m->prev;
	}
	monitor_t *next = m->next;
	while (next != NULL && rect_cmp(m->rectangle, next->rectangle) > 0) {
		swap_monitors(m, next);
		next = m->next;
	}
}

void rename_monitor(monitor_t *m, const char *name)
{
	put_status(SBSC_MASK_MONITOR_RENAME, "monitor_rename 0x%08X %s %s\n", m->id, m->name, name);

	snprintf(m->name, sizeof(m->name), "%s", name);
	xcb_icccm_set_wm_name(dpy, m->root, XCB_ATOM_STRING, 8, strlen(m->name), m->name);

	put_status(SBSC_MASK_REPORT);
}

monitor_t *find_monitor(uint32_t id)
{
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		if (m->id == id) {
			return m;
		}
	}
	return NULL;
}

monitor_t *get_monitor_by_randr_id(xcb_randr_output_t id)
{
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		if (m->randr_id == id) {
			return m;
		}
	}
	return NULL;
}

void embrace_client(monitor_t *m, client_t *c)
{
	if ((c->floating_rectangle.x + c->floating_rectangle.width) <= m->rectangle.x) {
		c->floating_rectangle.x = m->rectangle.x;
	} else if (c->floating_rectangle.x >= (m->rectangle.x + m->rectangle.width)) {
		c->floating_rectangle.x = (m->rectangle.x + m->rectangle.width) - c->floating_rectangle.width;
	}
	if ((c->floating_rectangle.y + c->floating_rectangle.height) <= m->rectangle.y) {
		c->floating_rectangle.y = m->rectangle.y;
	} else if (c->floating_rectangle.y >= (m->rectangle.y + m->rectangle.height)) {
		c->floating_rectangle.y = (m->rectangle.y + m->rectangle.height) - c->floating_rectangle.height;
	}
}

void adapt_geometry(xcb_rectangle_t *rs, xcb_rectangle_t *rd, node_t *n)
{
	for (node_t *f = first_extrema(n); f != NULL; f = next_leaf(f, n)) {
		if (f->client == NULL) {
			continue;
		}
		client_t *c = f->client;
		/* Clip the rectangle to fit into the monitor.	Without this, the fitting
		 * algorithm doesn't work as expected. This also conserves the
		 * out-of-bounds regions */
		int left_adjust = MAX((rs->x - c->floating_rectangle.x), 0);
		int top_adjust = MAX((rs->y - c->floating_rectangle.y), 0);
		int right_adjust = MAX((c->floating_rectangle.x + c->floating_rectangle.width) - (rs->x + rs->width), 0);
		int bottom_adjust = MAX((c->floating_rectangle.y + c->floating_rectangle.height) - (rs->y + rs->height), 0);
		c->floating_rectangle.x += left_adjust;
		c->floating_rectangle.y += top_adjust;
		c->floating_rectangle.width -= (left_adjust + right_adjust);
		c->floating_rectangle.height -= (top_adjust + bottom_adjust);

		int dx_s = c->floating_rectangle.x - rs->x;
		int dy_s = c->floating_rectangle.y - rs->y;

		int nume_x = dx_s * (rd->width - c->floating_rectangle.width);
		int nume_y = dy_s * (rd->height - c->floating_rectangle.height);

		int deno_x = rs->width - c->floating_rectangle.width;
		int deno_y = rs->height - c->floating_rectangle.height;

		int dx_d = (deno_x == 0 ? 0 : nume_x / deno_x);
		int dy_d = (deno_y == 0 ? 0 : nume_y / deno_y);

		/* Translate and undo clipping */
		c->floating_rectangle.width += left_adjust + right_adjust;
		c->floating_rectangle.height += top_adjust + bottom_adjust;
		c->floating_rectangle.x = rd->x + dx_d - left_adjust;
		c->floating_rectangle.y = rd->y + dy_d - top_adjust;
	}
}

void add_monitor(monitor_t *m)
{
	xcb_rectangle_t r = m->rectangle;

	if (mon == NULL) {
		mon = m;
		mon_head = m;
		mon_tail = m;
	} else {
		monitor_t *a = mon_head;
		while (a != NULL && rect_cmp(m->rectangle, a->rectangle) > 0) {
			a = a->next;
		}
		if (a != NULL) {
			monitor_t *b = a->prev;
			if (b != NULL) {
				b->next = m;
			} else {
				mon_head = m;
			}
			m->prev = b;
			m->next = a;
			a->prev = m;
		} else {
			mon_tail->next = m;
			m->prev = mon_tail;
			mon_tail = m;
		}
	}

	put_status(SBSC_MASK_MONITOR_ADD, "monitor_add 0x%08X %s %ux%u+%i+%i\n", m->id, m->name, r.width, r.height, r.x, r.y);

	put_status(SBSC_MASK_REPORT);
}

void unlink_monitor(monitor_t *m)
{
	monitor_t *prev = m->prev;
	monitor_t *next = m->next;

	if (prev != NULL) {
		prev->next = next;
	}

	if (next != NULL) {
		next->prev = prev;
	}

	if (mon_head == m) {
		mon_head = next;
	}

	if (mon_tail == m) {
		mon_tail = prev;
	}

	if (pri_mon == m) {
		pri_mon = NULL;
	}

	if (mon == m) {
		mon = NULL;
	}
}

void remove_monitor(monitor_t *m)
{
	put_status(SBSC_MASK_MONITOR_REMOVE, "monitor_remove 0x%08X\n", m->id);

	while (m->desk_head != NULL) {
		remove_desktop(m, m->desk_head);
	}

	monitor_t *last_mon = mon;

	unlink_monitor(m);
	xcb_destroy_window(dpy, m->root);
	free(m);

	if (mon != last_mon) {
		focus_node(NULL, NULL, NULL);
	}

	put_status(SBSC_MASK_REPORT);
}

void merge_monitors(monitor_t *ms, monitor_t *md)
{
	if (ms == NULL || md == NULL || ms == md) {
		return;
	}

	desktop_t *d = ms->desk_head;
	while (d != NULL) {
		desktop_t *next = d->next;
		transfer_desktop(ms, md, d, false);
		d = next;
	}
}

bool swap_monitors(monitor_t *m1, monitor_t *m2)
{
	if (m1 == NULL || m2 == NULL || m1 == m2) {
		return false;
	}

	put_status(SBSC_MASK_MONITOR_SWAP, "monitor_swap 0x%08X 0x%08X\n", m1->id, m2->id);

	if (mon_head == m1) {
		mon_head = m2;
	} else if (mon_head == m2) {
		mon_head = m1;
	}
	if (mon_tail == m1) {
		mon_tail = m2;
	} else if (mon_tail == m2) {
		mon_tail = m1;
	}

	monitor_t *p1 = m1->prev;
	monitor_t *n1 = m1->next;
	monitor_t *p2 = m2->prev;
	monitor_t *n2 = m2->next;

	if (p1 != NULL && p1 != m2) {
		p1->next = m2;
	}
	if (n1 != NULL && n1 != m2) {
		n1->prev = m2;
	}
	if (p2 != NULL && p2 != m1) {
		p2->next = m1;
	}
	if (n2 != NULL && n2 != m1) {
		n2->prev = m1;
	}

	m1->prev = p2 == m1 ? m2 : p2;
	m1->next = n2 == m1 ? m2 : n2;
	m2->prev = p1 == m2 ? m1 : p1;
	m2->next = n1 == m2 ? m1 : n1;

	ewmh_update_wm_desktops();
	ewmh_update_desktop_names();
	ewmh_update_desktop_viewport();
	ewmh_update_current_desktop();

	put_status(SBSC_MASK_REPORT);
	return true;
}

monitor_t *closest_monitor(monitor_t *m, cycle_dir_t dir, monitor_select_t *sel)
{
	monitor_t *f = (dir == CYCLE_PREV ? m->prev : m->next);

	if (f == NULL) {
		f = (dir == CYCLE_PREV ? mon_tail : mon_head);
	}

	while (f != m) {
		coordinates_t loc = {f, NULL, NULL};
		if (monitor_matches(&loc, &loc, sel)) {
			return f;
		}
		f = (dir == CYCLE_PREV ? f->prev : f->next);
		if (f == NULL) {
			f = (dir == CYCLE_PREV ? mon_tail : mon_head);
		}
	}

	return NULL;
}

bool is_inside_monitor(monitor_t *m, xcb_point_t pt)
{
	return is_inside(pt, m->rectangle);
}

monitor_t *monitor_from_point(xcb_point_t pt)
{
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		if (is_inside_monitor(m, pt)) {
			return m;
		}
	}
	return NULL;
}

monitor_t *monitor_from_client(client_t *c)
{
	int16_t xc = c->floating_rectangle.x + c->floating_rectangle.width/2;
	int16_t yc = c->floating_rectangle.y + c->floating_rectangle.height/2;
	xcb_point_t pt = {xc, yc};
	monitor_t *nearest = monitor_from_point(pt);
	if (nearest == NULL) {
		int dmin = INT_MAX;
		for (monitor_t *m = mon_head; m != NULL; m = m->next) {
			xcb_rectangle_t r = m->rectangle;
			int d = abs((r.x + r.width / 2) - xc) + abs((r.y + r.height / 2) - yc);
			if (d < dmin) {
				dmin = d;
				nearest = m;
			}
		}
	}
	return nearest;
}

monitor_t *nearest_monitor(monitor_t *m, direction_t dir, monitor_select_t *sel)
{
	uint32_t dmin = UINT32_MAX;
	monitor_t *nearest = NULL;
	xcb_rectangle_t rect = m->rectangle;
	for (monitor_t *f = mon_head; f != NULL; f = f->next) {
		coordinates_t loc = {f, NULL, NULL};
		xcb_rectangle_t r = f->rectangle;
		if (f == m ||
		    !monitor_matches(&loc, &loc, sel) ||
		    !on_dir_side(rect, r, dir)) {
			continue;
		}
		uint32_t d = boundary_distance(rect, r, dir);
		if (d < dmin) {
			dmin = d;
			nearest = f;
		}
	}
	return nearest;
}

bool find_any_monitor(coordinates_t *ref, coordinates_t *dst, monitor_select_t *sel)
{
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		coordinates_t loc = {m, NULL, NULL};
		if (monitor_matches(&loc, ref, sel)) {
			*dst = loc;
			return true;
		}
	}
	return false;
}

bool update_monitors(void)
{
	xcb_randr_get_screen_resources_reply_t *sres = xcb_randr_get_screen_resources_reply(dpy, xcb_randr_get_screen_resources(dpy, root), NULL);
	if (sres == NULL) {
		return false;
	}

	monitor_t *last_wired = NULL;

	int len = xcb_randr_get_screen_resources_outputs_length(sres);
	xcb_randr_output_t *outputs = xcb_randr_get_screen_resources_outputs(sres);

	xcb_randr_get_output_info_cookie_t cookies[len];
	for (int i = 0; i < len; i++) {
		cookies[i] = xcb_randr_get_output_info(dpy, outputs[i], XCB_CURRENT_TIME);
	}

	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		m->wired = false;
	}

	for (int i = 0; i < len; i++) {
		xcb_randr_get_output_info_reply_t *info = xcb_randr_get_output_info_reply(dpy, cookies[i], NULL);
		if (info != NULL) {
			if (info->crtc != XCB_NONE) {
				xcb_randr_get_crtc_info_reply_t *cir = xcb_randr_get_crtc_info_reply(dpy, xcb_randr_get_crtc_info(dpy, info->crtc, XCB_CURRENT_TIME), NULL);
				if (cir != NULL) {
					xcb_rectangle_t rect = (xcb_rectangle_t) {cir->x, cir->y, cir->width, cir->height};
					last_wired = get_monitor_by_randr_id(outputs[i]);
					if (last_wired != NULL) {
						update_root(last_wired, &rect);
						last_wired->wired = true;
					} else {
						char *name = (char *) xcb_randr_get_output_info_name(info);
						size_t len = (size_t) xcb_randr_get_output_info_name_length(info);
						char *name_copy = copy_string(name, len);
						last_wired = make_monitor(name_copy, &rect, XCB_NONE);
						free(name_copy);
						last_wired->randr_id = outputs[i];
						add_monitor(last_wired);
					}
				}
				free(cir);
			} else if (!remove_disabled_monitors && info->connection != XCB_RANDR_CONNECTION_DISCONNECTED) {
				monitor_t *m = get_monitor_by_randr_id(outputs[i]);
				if (m != NULL) {
					m->wired = true;
				}
			}
		}
		free(info);
	}

	xcb_randr_get_output_primary_reply_t *gpo = xcb_randr_get_output_primary_reply(dpy, xcb_randr_get_output_primary(dpy, root), NULL);
	if (gpo != NULL) {
		pri_mon = get_monitor_by_randr_id(gpo->output);
	}
	free(gpo);

	/* handle overlapping monitors */
	if (merge_overlapping_monitors) {
		monitor_t *m = mon_head;
		while (m != NULL) {
			monitor_t *next = m->next;
			if (m->wired) {
				monitor_t *mb = mon_head;
				while (mb != NULL) {
					monitor_t *mb_next = mb->next;
					if (m != mb && mb->wired && contains(m->rectangle, mb->rectangle)) {
						if (last_wired == mb) {
							last_wired = m;
						}
						if (next == mb) {
							next = mb_next;
						}
						merge_monitors(mb, m);
						remove_monitor(mb);
					}
					mb = mb_next;
				}
			}
			m = next;
		}
	}

	/* merge and remove disconnected monitors */
	if (remove_unplugged_monitors) {
		monitor_t *m = mon_head;
		while (m != NULL) {
			monitor_t *next = m->next;
			if (!m->wired) {
				merge_monitors(m, last_wired);
				remove_monitor(m);
			}
			m = next;
		}
	}

	/* add one desktop to each new monitor */
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		if (m->desk == NULL) {
			add_desktop(m, make_desktop(NULL, XCB_NONE));
		}
	}

	if (!running && mon != NULL) {
		if (pri_mon != NULL) {
			mon = pri_mon;
		}
		center_pointer(mon->rectangle);
		ewmh_update_current_desktop();
	}

	free(sres);

	return (mon != NULL);
}
