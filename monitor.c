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

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "bspwm.h"
#include "desktop.h"
#include "ewmh.h"
#include "history.h"
#include "query.h"
#include "settings.h"
#include "tree.h"
#include "window.h"
#include "monitor.h"

monitor_t *make_monitor(xcb_rectangle_t rect)
{
	monitor_t *m = malloc(sizeof(monitor_t));
	snprintf(m->name, sizeof(m->name), "%s%02d", DEFAULT_MON_NAME, ++monitor_uid);
	m->prev = m->next = NULL;
	m->desk = m->desk_head = m->desk_tail = NULL;
	m->rectangle = rect;
	m->top_padding = m->right_padding = m->bottom_padding = m->left_padding = 0;
	m->wired = true;
	m->num_sticky = 0;
	uint32_t mask = XCB_CW_EVENT_MASK;
	uint32_t values[] = {XCB_EVENT_MASK_ENTER_WINDOW};
	m->root = xcb_generate_id(dpy);
	xcb_create_window(dpy, XCB_COPY_FROM_PARENT, m->root, root, rect.x, rect.y, rect.width, rect.height, 0, XCB_WINDOW_CLASS_INPUT_ONLY, XCB_COPY_FROM_PARENT, mask, values);
	window_lower(m->root);
	if (focus_follows_pointer)
		window_show(m->root);
	return m;
}

monitor_t *find_monitor(char *name)
{
	for (monitor_t *m = mon_head; m != NULL; m = m->next)
		if (streq(m->name, name))
			return m;
	return NULL;
}

monitor_t *get_monitor_by_id(xcb_randr_output_t id)
{
	for (monitor_t *m = mon_head; m != NULL; m = m->next)
		if (m->id == id)
			return m;
	return NULL;
}

void embrace_client(monitor_t *m, client_t *c)
{
	if ((c->floating_rectangle.x + c->floating_rectangle.width) <= m->rectangle.x)
		c->floating_rectangle.x = m->rectangle.x;
	else if (c->floating_rectangle.x >= (m->rectangle.x + m->rectangle.width))
		c->floating_rectangle.x = (m->rectangle.x + m->rectangle.width) - c->floating_rectangle.width;
	if ((c->floating_rectangle.y + c->floating_rectangle.height) <= m->rectangle.y)
		c->floating_rectangle.y = m->rectangle.y;
	else if (c->floating_rectangle.y >= (m->rectangle.y + m->rectangle.height))
		c->floating_rectangle.y = (m->rectangle.y + m->rectangle.height) - c->floating_rectangle.height;
}

void translate_client(monitor_t *ms, monitor_t *md, client_t *c)
{
	if (frozen_pointer->action != ACTION_NONE || ms == md)
		return;

	/* Clip the rectangle to fit into the monitor.	Without this, the fitting
	 * algorithm doesn't work as expected. This also conserves the
	 * out-of-bounds regions */
	int left_adjust = MAX((ms->rectangle.x - c->floating_rectangle.x), 0);
	int top_adjust = MAX((ms->rectangle.y - c->floating_rectangle.y), 0);
	int right_adjust = MAX((c->floating_rectangle.x + c->floating_rectangle.width) - (ms->rectangle.x + ms->rectangle.width), 0);
	int bottom_adjust = MAX((c->floating_rectangle.y + c->floating_rectangle.height) - (ms->rectangle.y + ms->rectangle.height), 0);
	c->floating_rectangle.x += left_adjust;
	c->floating_rectangle.y += top_adjust;
	c->floating_rectangle.width -= (left_adjust + right_adjust);
	c->floating_rectangle.height -= (top_adjust + bottom_adjust);

	int dx_s = c->floating_rectangle.x - ms->rectangle.x;
	int dy_s = c->floating_rectangle.y - ms->rectangle.y;

	int nume_x = dx_s * (md->rectangle.width - c->floating_rectangle.width);
	int nume_y = dy_s * (md->rectangle.height - c->floating_rectangle.height);

	int deno_x = ms->rectangle.width - c->floating_rectangle.width;
	int deno_y = ms->rectangle.height - c->floating_rectangle.height;

	int dx_d = (deno_x == 0 ? 0 : nume_x / deno_x);
	int dy_d = (deno_y == 0 ? 0 : nume_y / deno_y);

	/* Translate and undo clipping */
	c->floating_rectangle.width += left_adjust + right_adjust;
	c->floating_rectangle.height += top_adjust + bottom_adjust;
	c->floating_rectangle.x = md->rectangle.x + dx_d - left_adjust;
	c->floating_rectangle.y = md->rectangle.y + dy_d - top_adjust;
}

void update_root(monitor_t *m)
{
	xcb_rectangle_t rect = m->rectangle;
	window_move_resize(m->root, rect.x, rect.y, rect.width, rect.height);
}

void focus_monitor(monitor_t *m)
{
	if (mon == m)
		return;

	PRINTF("focus monitor %s\n", m->name);

	mon = m;

	if (pointer_follows_monitor)
		center_pointer(m);

	ewmh_update_current_desktop();
	put_status();
}

monitor_t *add_monitor(xcb_rectangle_t rect)
{
	monitor_t *m = make_monitor(rect);
	if (mon == NULL) {
		mon = m;
		mon_head = m;
		mon_tail = m;
	} else {
		mon_tail->next = m;
		m->prev = mon_tail;
		mon_tail = m;
	}
	num_monitors++;
	return m;
}

void remove_monitor(monitor_t *m)
{
	PRINTF("remove monitor %s (0x%X)\n", m->name, m->id);

	while (m->desk_head != NULL)
		remove_desktop(m, m->desk_head);
	monitor_t *prev = m->prev;
	monitor_t *next = m->next;
	monitor_t *last_mon = history_get_monitor(m);
	if (prev != NULL)
		prev->next = next;
	if (next != NULL)
		next->prev = prev;
	if (mon_head == m)
		mon_head = next;
	if (mon_tail == m)
		mon_tail = prev;
	if (pri_mon == m)
		pri_mon = NULL;
	if (mon == m) {
		mon = (last_mon == NULL ? (prev == NULL ? next : prev) : last_mon);
		if (mon != NULL && mon->desk != NULL)
			update_current();
	}
	xcb_destroy_window(dpy, m->root);
	free(m);
	num_monitors--;
	put_status();
}

void merge_monitors(monitor_t *ms, monitor_t *md)
{
	PRINTF("merge %s into %s\n", ms->name, md->name);

	desktop_t *d = ms->desk_head;
	while (d != NULL) {
		desktop_t *next = d->next;
		if (d->root != NULL || strstr(d->name, DEFAULT_DESK_NAME) == NULL)
			transfer_desktop(ms, md, d);
		d = next;
	}
}

void swap_monitors(monitor_t *m1, monitor_t *m2)
{
	if (m1 == NULL || m2 == NULL || m1 == m2)
		return;

	if (mon_head == m1)
		mon_head = m2;
	else if (mon_head == m2)
		mon_head = m1;
	if (mon_tail == m1)
		mon_tail = m2;
	else if (mon_tail == m2)
		mon_tail = m1;

	monitor_t *p1 = m1->prev;
	monitor_t *n1 = m1->next;
	monitor_t *p2 = m2->prev;
	monitor_t *n2 = m2->next;

	if (p1 != NULL && p1 != m2)
		p1->next = m2;
	if (n1 != NULL && n1 != m2)
		n1->prev = m2;
	if (p2 != NULL && p2 != m1)
		p2->next = m1;
	if (n2 != NULL && n2 != m1)
		n2->prev = m1;

	m1->prev = p2 == m1 ? m2 : p2;
	m1->next = n2 == m1 ? m2 : n2;
	m2->prev = p1 == m2 ? m1 : p1;
	m2->next = n1 == m2 ? m1 : n1;

	ewmh_update_wm_desktops();
	ewmh_update_desktop_names();
	ewmh_update_current_desktop();
	put_status();
}

monitor_t *closest_monitor(monitor_t *m, cycle_dir_t dir, desktop_select_t sel)
{
	monitor_t *f = (dir == CYCLE_PREV ? m->prev : m->next);
	if (f == NULL)
		f = (dir == CYCLE_PREV ? mon_tail : mon_head);

	while (f != m) {
		coordinates_t loc = {m, m->desk, NULL};
		if (desktop_matches(&loc, &loc, sel))
			return f;
		f = (dir == CYCLE_PREV ? m->prev : m->next);
		if (f == NULL)
			f = (dir == CYCLE_PREV ? mon_tail : mon_head);
	}

	return NULL;
}

bool is_inside_monitor(monitor_t *m, xcb_point_t pt)
{
	xcb_rectangle_t r = m->rectangle;
	return (r.x <= pt.x && pt.x < (r.x + r.width)
			&& r.y <= pt.y && pt.y < (r.y + r.height));
}

monitor_t *monitor_from_point(xcb_point_t pt)
{
	for (monitor_t *m = mon_head; m != NULL; m = m->next)
		if (is_inside_monitor(m, pt))
			return m;
	return NULL;
}

monitor_t *monitor_from_client(client_t *c)
{
	xcb_point_t pt = {c->floating_rectangle.x, c->floating_rectangle.y};
	monitor_t *nearest = monitor_from_point(pt);
	if (nearest == NULL) {
		int x = (c->floating_rectangle.x + c->floating_rectangle.width) / 2;
		int y = (c->floating_rectangle.y + c->floating_rectangle.height) / 2;
		int dmin = INT_MAX;
		for (monitor_t *m = mon_head; m != NULL; m = m->next) {
			xcb_rectangle_t r = m->rectangle;
			int d = abs((r.x + r.width / 2) - x) + abs((r.y + r.height / 2) - y);
			if (d < dmin) {
				dmin = d;
				nearest = m;
			}
		}
	}
	return nearest;
}

monitor_t *nearest_monitor(monitor_t *m, direction_t dir, desktop_select_t sel)
{
	int dmin = INT_MAX;
	monitor_t *nearest = NULL;
	xcb_rectangle_t rect = m->rectangle;
	for (monitor_t *f = mon_head; f != NULL; f = f->next) {
		if (f == m)
			continue;
		coordinates_t loc = {f, f->desk, NULL};
		if (!desktop_matches(&loc, &loc, sel))
			continue;
		xcb_rectangle_t r = f->rectangle;
		if ((dir == DIR_LEFT && r.x < rect.x) ||
		    (dir == DIR_RIGHT && r.x >= (rect.x + rect.width)) ||
		    (dir == DIR_UP && r.y < rect.y) ||
		    (dir == DIR_DOWN && r.y >= (rect.y + rect.height))) {
			int d = abs((r.x + r.width / 2) - (rect.x + rect.width / 2)) +
			        abs((r.y + r.height / 2) - (rect.y + rect.height / 2));
			if (d < dmin) {
				dmin = d;
				nearest = f;
			}
		}
	}
	return nearest;
}

bool update_monitors(void)
{
	PUTS("update monitors");
	xcb_randr_get_screen_resources_current_reply_t *sres = xcb_randr_get_screen_resources_current_reply(dpy, xcb_randr_get_screen_resources_current(dpy, root), NULL);
	if (sres == NULL)
		return false;

	monitor_t *m, *mm = NULL;

	int len = xcb_randr_get_screen_resources_current_outputs_length(sres);
	xcb_randr_output_t *outputs = xcb_randr_get_screen_resources_current_outputs(sres);

	xcb_randr_get_output_info_cookie_t cookies[len];
	for (int i = 0; i < len; i++)
		cookies[i] = xcb_randr_get_output_info(dpy, outputs[i], XCB_CURRENT_TIME);

	for (m = mon_head; m != NULL; m = m->next)
		m->wired = false;

	for (int i = 0; i < len; i++) {
		xcb_randr_get_output_info_reply_t *info = xcb_randr_get_output_info_reply(dpy, cookies[i], NULL);
		if (info != NULL) {
			if (info->crtc != XCB_NONE) {
				xcb_randr_get_crtc_info_reply_t *cir = xcb_randr_get_crtc_info_reply(dpy, xcb_randr_get_crtc_info(dpy, info->crtc, XCB_CURRENT_TIME), NULL);
				if (cir != NULL) {
					xcb_rectangle_t rect = (xcb_rectangle_t) {cir->x, cir->y, cir->width, cir->height};
					mm = get_monitor_by_id(outputs[i]);
					if (mm != NULL) {
						mm->rectangle = rect;
						update_root(mm);
						for (desktop_t *d = mm->desk_head; d != NULL; d = d->next)
							for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root))
								translate_client(mm, mm, n->client);
						arrange(mm, mm->desk);
						mm->wired = true;
						PRINTF("update monitor %s (0x%X)\n", mm->name, mm->id);
					} else {
						mm = add_monitor(rect);
						char *name = (char *)xcb_randr_get_output_info_name(info);
						size_t name_len = MIN(sizeof(mm->name), (size_t)xcb_randr_get_output_info_name_length(info) + 1);
						snprintf(mm->name, name_len, "%s", name);
						mm->id = outputs[i];
						PRINTF("add monitor %s (0x%X)\n", mm->name, mm->id);
					}
				}
				free(cir);
			} else if (!remove_disabled_monitor && info->connection != XCB_RANDR_CONNECTION_DISCONNECTED) {
				m = get_monitor_by_id(outputs[i]);
				if (m != NULL)
					m->wired = true;
			}
		}
		free(info);
	}

	/* initially focus the primary monitor and add the first desktop to it */
	xcb_randr_get_output_primary_reply_t *gpo = xcb_randr_get_output_primary_reply(dpy, xcb_randr_get_output_primary(dpy, root), NULL);
	if (gpo != NULL) {
		pri_mon = get_monitor_by_id(gpo->output);
		if (!running && pri_mon != NULL) {
			if (mon != pri_mon)
				mon = pri_mon;
			add_desktop(pri_mon, make_desktop(NULL));
			ewmh_update_current_desktop();
		}
	}
	free(gpo);

	/* handle overlapping monitors */
	if (merge_overlapping_monitors) {
		m = mon_head;
		while (m != NULL) {
			monitor_t *next = m->next;
			if (m->wired) {
				for (monitor_t *mb = mon_head; mb != NULL; mb = mb->next)
					if (mb != m && mb->wired &&
							(m->desk == NULL || mb->desk == NULL) &&
							contains(mb->rectangle, m->rectangle)) {
						if (mm == m)
							mm = mb;
						merge_monitors(m, mb);
						remove_monitor(m);
						break;
					}
			}
			m = next;
		}
	}

	/* merge and remove disconnected monitors */
	if (remove_unplugged_monitors) {
		m = mon_head;
		while (m != NULL) {
			monitor_t *next = m->next;
			if (!m->wired) {
				merge_monitors(m, mm);
				remove_monitor(m);
			}
			m = next;
		}
	}

	/* add one desktop to each new monitor */
	for (m = mon_head; m != NULL; m = m->next)
		if (m->desk == NULL && (running || pri_mon == NULL || m != pri_mon))
			add_desktop(m, make_desktop(NULL));

	if (!running && pri_mon != NULL && mon_head != pri_mon)
		swap_monitors(mon_head, pri_mon);

	free(sres);
	update_motion_recorder();
	return (num_monitors > 0);
}
