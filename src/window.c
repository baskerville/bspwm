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
#include <xcb/shape.h>
#include "bspwm.h"
#include "ewmh.h"
#include "monitor.h"
#include "desktop.h"
#include "query.h"
#include "rule.h"
#include "settings.h"
#include "geometry.h"
#include "pointer.h"
#include "stack.h"
#include "tree.h"
#include "parse.h"
#include "window.h"

void schedule_window(xcb_window_t win)
{
	coordinates_t loc;
	uint8_t override_redirect = 0;
	xcb_get_window_attributes_reply_t *wa = xcb_get_window_attributes_reply(dpy, xcb_get_window_attributes(dpy, win), NULL);

	if (wa != NULL) {
		override_redirect = wa->override_redirect;
		free(wa);
	}

	if (override_redirect || locate_window(win, &loc)) {
		return;
	}

	/* ignore pending windows */
	for (pending_rule_t *pr = pending_rule_head; pr != NULL; pr = pr->next) {
		if (pr->win == win) {
			return;
		}
	}

	rule_consequence_t *csq = make_rule_consequence();
	apply_rules(win, csq);
	if (!schedule_rules(win, csq)) {
		manage_window(win, csq, -1);
		free(csq);
	}
}

bool manage_window(xcb_window_t win, rule_consequence_t *csq, int fd)
{
	monitor_t *m = mon;
	desktop_t *d = mon->desk;
	node_t *f = mon->desk->focus;

	parse_rule_consequence(fd, csq);

	if (!ignore_ewmh_struts && ewmh_handle_struts(win)) {
		for (monitor_t *m = mon_head; m != NULL; m = m->next) {
			arrange(m, m->desk);
		}
	}

	if (!csq->manage) {
		free(csq->layer);
		free(csq->state);
		window_show(win);
		return false;
	}

	if (csq->node_desc[0] != '\0') {
		coordinates_t ref = {m, d, f};
		coordinates_t trg = {NULL, NULL, NULL};
		if (node_from_desc(csq->node_desc, &ref, &trg) == SELECTOR_OK) {
			m = trg.monitor;
			d = trg.desktop;
			f = trg.node;
		}
	} else if (csq->desktop_desc[0] != '\0') {
		coordinates_t ref = {m, d, NULL};
		coordinates_t trg = {NULL, NULL, NULL};
		if (desktop_from_desc(csq->desktop_desc, &ref, &trg) == SELECTOR_OK) {
			m = trg.monitor;
			d = trg.desktop;
			f = trg.desktop->focus;
		}
	} else if (csq->monitor_desc[0] != '\0') {
		coordinates_t ref = {m, NULL, NULL};
		coordinates_t trg = {NULL, NULL, NULL};
		if (monitor_from_desc(csq->monitor_desc, &ref, &trg) == SELECTOR_OK) {
			m = trg.monitor;
			d = trg.monitor->desk;
			f = trg.monitor->desk->focus;
		}
	}

	if (csq->sticky) {
		m = mon;
		d = mon->desk;
		f = mon->desk->focus;
	}

	if (csq->split_dir[0] != '\0' && f != NULL) {
		direction_t dir;
		if (parse_direction(csq->split_dir, &dir)) {
			presel_dir(m, d, f, dir);
		}
	}

	if (csq->split_ratio != 0 && f != NULL) {
		presel_ratio(m, d, f, csq->split_ratio);
	}

	node_t *n = make_node(win);
	client_t *c = make_client();
	c->border_width = csq->border ? d->border_width : 0;
	n->client = c;
	initialize_client(n);
	initialize_floating_rectangle(n);

	if (csq->rect != NULL) {
		c->floating_rectangle = *csq->rect;
		free(csq->rect);
	} else if (c->floating_rectangle.x == 0 && c->floating_rectangle.y == 0) {
		csq->center = true;
	}

	monitor_t *mm = monitor_from_client(c);
	embrace_client(mm, c);
	adapt_geometry(&mm->rectangle, &m->rectangle, n);

	if (csq->center) {
		window_center(m, c);
	}

	snprintf(c->class_name, sizeof(c->class_name), "%s", csq->class_name);
	snprintf(c->instance_name, sizeof(c->instance_name), "%s", csq->instance_name);

	if ((csq->state != NULL && (*(csq->state) == STATE_FLOATING || *(csq->state) == STATE_FULLSCREEN)) || csq->hidden) {
		n->vacant = true;
	}

	f = insert_node(m, d, n, f);
	clients_count++;
	if (single_monocle && d->layout == LAYOUT_MONOCLE && tiled_count(d->root, true) > 1) {
		set_layout(m, d, d->user_layout, false);
	}

	n->vacant = false;

	put_status(SBSC_MASK_NODE_ADD, "node_add 0x%08X 0x%08X 0x%08X 0x%08X\n", m->id, d->id, f!=NULL?f->id:0, win);

	if (f != NULL && f->client != NULL && csq->state != NULL && *(csq->state) == STATE_FLOATING) {
		c->layer = f->client->layer;
	}

	if (csq->layer != NULL) {
		c->layer = *(csq->layer);
	}

	if (csq->state != NULL) {
		set_state(m, d, n, *(csq->state));
	}

	set_hidden(m, d, n, csq->hidden);
	set_sticky(m, d, n, csq->sticky);
	set_private(m, d, n, csq->private);
	set_locked(m, d, n, csq->locked);
	set_marked(m, d, n, csq->marked);

	arrange(m, d);

	uint32_t values[] = {CLIENT_EVENT_MASK | (focus_follows_pointer ? XCB_EVENT_MASK_ENTER_WINDOW : 0)};
	xcb_change_window_attributes(dpy, win, XCB_CW_EVENT_MASK, values);
	set_window_state(win, XCB_ICCCM_WM_STATE_NORMAL);
	window_grab_buttons(win);

	if (d == m->desk) {
		show_node(d, n);
	} else {
		hide_node(d, n);
	}

	ewmh_update_client_list(false);
	ewmh_set_wm_desktop(n, d);

	if (!csq->hidden && csq->focus) {
		if (d == mon->desk || csq->follow) {
			focus_node(m, d, n);
		} else {
			activate_node(m, d, n);
		}
	} else {
		stack(d, n, false);
		draw_border(n, false, (m == mon));
	}

	free(csq->layer);
	free(csq->state);

	return true;
}

void set_window_state(xcb_window_t win, xcb_icccm_wm_state_t state)
{
	long data[] = {state, XCB_NONE};
	xcb_change_property(dpy, XCB_PROP_MODE_REPLACE, win, WM_STATE, WM_STATE, 32, 2, data);
}

void unmanage_window(xcb_window_t win)
{
	coordinates_t loc;
	if (locate_window(win, &loc)) {
		put_status(SBSC_MASK_NODE_REMOVE, "node_remove 0x%08X 0x%08X 0x%08X\n", loc.monitor->id, loc.desktop->id, win);
		remove_node(loc.monitor, loc.desktop, loc.node);
		arrange(loc.monitor, loc.desktop);
	} else {
		for (pending_rule_t *pr = pending_rule_head; pr != NULL; pr = pr->next) {
			if (pr->win == win) {
				remove_pending_rule(pr);
				return;
			}
		}
	}
}

bool is_presel_window(xcb_window_t win)
{
	xcb_icccm_get_wm_class_reply_t reply;
	bool ret = false;
	if (xcb_icccm_get_wm_class_reply(dpy, xcb_icccm_get_wm_class(dpy, win), &reply, NULL) == 1) {
		if (streq(BSPWM_CLASS_NAME, reply.class_name) && streq(PRESEL_FEEDBACK_I, reply.instance_name)) {
			ret = true;
		}
		xcb_icccm_get_wm_class_reply_wipe(&reply);
	}
	return ret;
}

void initialize_presel_feedback(node_t *n)
{
	if (n == NULL || n->presel == NULL || n->presel->feedback != XCB_NONE) {
		return;
	}

	xcb_window_t win = xcb_generate_id(dpy);
	uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_SAVE_UNDER;
	uint32_t values[] = {get_color_pixel(presel_feedback_color), 1};
	xcb_create_window(dpy, XCB_COPY_FROM_PARENT, win, root, 0, 0, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
			          XCB_COPY_FROM_PARENT, mask, values);

	xcb_icccm_set_wm_class(dpy, win, sizeof(PRESEL_FEEDBACK_IC), PRESEL_FEEDBACK_IC);
	/* Make presel window's input shape NULL to pass any input to window below */
	xcb_shape_rectangles(dpy, XCB_SHAPE_SO_SET, XCB_SHAPE_SK_INPUT, XCB_CLIP_ORDERING_UNSORTED, win, 0, 0, 0, NULL);
	stacking_list_t *s = stack_tail;
	while (s != NULL && !IS_TILED(s->node->client)) {
		s = s->prev;
	}
	if (s != NULL) {
		window_above(win, s->node->id);
	}
	n->presel->feedback = win;
}

void draw_presel_feedback(monitor_t *m, desktop_t *d, node_t *n)
{
	if (n == NULL || n->presel == NULL || d->user_layout == LAYOUT_MONOCLE || !presel_feedback) {
		return;
	}

	bool exists = (n->presel->feedback != XCB_NONE);
	if (!exists) {
		initialize_presel_feedback(n);
	}

	int gap = gapless_monocle && d->layout == LAYOUT_MONOCLE ? 0 : d->window_gap;
	presel_t *p = n->presel;
	xcb_rectangle_t rect = n->rectangle;
	rect.x = rect.y = 0;
	rect.width -= gap;
	rect.height -= gap;
	xcb_rectangle_t presel_rect = rect;

	switch (p->split_dir) {
		case DIR_NORTH:
			presel_rect.height = p->split_ratio * rect.height;
			break;
		case DIR_EAST:
			presel_rect.width = (1 - p->split_ratio) * rect.width;
			presel_rect.x = rect.width - presel_rect.width;
			break;
		case DIR_SOUTH:
			presel_rect.height = (1 - p->split_ratio) * rect.height;
			presel_rect.y = rect.height - presel_rect.height;
			break;
		case DIR_WEST:
			presel_rect.width = p->split_ratio * rect.width;
			break;
	}

	window_move_resize(p->feedback, n->rectangle.x + presel_rect.x, n->rectangle.y + presel_rect.y,
	                   presel_rect.width, presel_rect.height);

	if (!exists && m->desk == d) {
		window_show(p->feedback);
	}
}

void refresh_presel_feedbacks(monitor_t *m, desktop_t *d, node_t *n)
{
	if (n == NULL) {
		return;
	} else {
		if (n->presel != NULL) {
			draw_presel_feedback(m, d, n);
		}
		refresh_presel_feedbacks(m, d, n->first_child);
		refresh_presel_feedbacks(m, d, n->second_child);
	}
}

void show_presel_feedbacks(monitor_t *m, desktop_t *d, node_t *n)
{
	if (n == NULL) {
		return;
	} else {
		if (n->presel != NULL) {
			window_show(n->presel->feedback);
		}
		show_presel_feedbacks(m, d, n->first_child);
		show_presel_feedbacks(m, d, n->second_child);
	}
}

void hide_presel_feedbacks(monitor_t *m, desktop_t *d, node_t *n)
{
	if (n == NULL) {
		return;
	} else {
		if (n->presel != NULL) {
			window_hide(n->presel->feedback);
		}
		hide_presel_feedbacks(m, d, n->first_child);
		hide_presel_feedbacks(m, d, n->second_child);
	}
}

void update_colors(void)
{
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			update_colors_in(d->root, d, m);
		}
	}
}

void update_colors_in(node_t *n, desktop_t *d, monitor_t *m)
{
	if (n == NULL) {
		return;
	} else {
		if (n->presel != NULL) {
			uint32_t pxl = get_color_pixel(presel_feedback_color);
			xcb_change_window_attributes(dpy, n->presel->feedback, XCB_CW_BACK_PIXEL, &pxl);
			if (d == m->desk) {
				/* hack to induce back pixel refresh */
				window_hide(n->presel->feedback);
				window_show(n->presel->feedback);
			}
		}
		if (n == d->focus) {
			draw_border(n, true, (m == mon));
		} else if (n->client != NULL) {
			draw_border(n, false, (m == mon));
		} else {
			update_colors_in(n->first_child, d, m);
			update_colors_in(n->second_child, d, m);
		}
	}
}

void draw_border(node_t *n, bool focused_node, bool focused_monitor)
{
	if (n == NULL) {
		return;
	}

	uint32_t border_color_pxl = get_border_color(focused_node, focused_monitor);
	for (node_t *f = first_extrema(n); f != NULL; f = next_leaf(f, n)) {
		if (f->client != NULL) {
			window_draw_border(f->id, border_color_pxl);
		}
	}
}

void window_draw_border(xcb_window_t win, uint32_t border_color_pxl)
{
	xcb_change_window_attributes(dpy, win, XCB_CW_BORDER_PIXEL, &border_color_pxl);
}

void adopt_orphans(void)
{
	xcb_query_tree_reply_t *qtr = xcb_query_tree_reply(dpy, xcb_query_tree(dpy, root), NULL);
	if (qtr == NULL) {
		return;
	}

	int len = xcb_query_tree_children_length(qtr);
	xcb_window_t *wins = xcb_query_tree_children(qtr);

	for (int i = 0; i < len; i++) {
		uint32_t idx;
		xcb_window_t win = wins[i];
		if (xcb_ewmh_get_wm_desktop_reply(ewmh, xcb_ewmh_get_wm_desktop(ewmh, win), &idx, NULL) == 1) {
			schedule_window(win);
		}
	}

	free(qtr);
}

uint32_t get_border_color(bool focused_node, bool focused_monitor)
{
	if (focused_monitor && focused_node) {
		return get_color_pixel(focused_border_color);
	} else if (focused_node) {
		return get_color_pixel(active_border_color);
	} else {
		return get_color_pixel(normal_border_color);
	}
}

void initialize_floating_rectangle(node_t *n)
{
	client_t *c = n->client;

	xcb_get_geometry_reply_t *geo = xcb_get_geometry_reply(dpy, xcb_get_geometry(dpy, n->id), NULL);

	if (geo != NULL) {
		c->floating_rectangle = (xcb_rectangle_t) {geo->x, geo->y, geo->width, geo->height};
	}

	free(geo);
}

xcb_rectangle_t get_window_rectangle(node_t *n)
{
	client_t *c = n->client;
	if (c != NULL) {
		xcb_get_geometry_reply_t *g = xcb_get_geometry_reply(dpy, xcb_get_geometry(dpy, n->id), NULL);
		if (g != NULL) {
			xcb_rectangle_t rect = (xcb_rectangle_t) {g->x, g->y, g->width, g->height};
			free(g);
			return rect;
		}
	}
	return (xcb_rectangle_t) {0, 0, screen_width, screen_height};
}

bool move_client(coordinates_t *loc, int dx, int dy)
{
	node_t *n = loc->node;

	if (n == NULL || n->client == NULL) {
		return false;
	}

	monitor_t *pm = NULL;

	if (IS_TILED(n->client)) {
		if (!grabbing) {
			return false;
		}
		xcb_window_t pwin = XCB_NONE;
		query_pointer(&pwin, NULL);
		if (pwin == n->id) {
			return false;
		}
		coordinates_t dst;
		bool is_managed = (pwin != XCB_NONE && locate_window(pwin, &dst));
		if (is_managed && dst.monitor == loc->monitor && IS_TILED(dst.node->client)) {
			swap_nodes(loc->monitor, loc->desktop, n, loc->monitor, loc->desktop, dst.node, false);
			return true;
		} else {
			if (is_managed && dst.monitor == loc->monitor) {
				return false;
			} else {
				xcb_point_t pt = {0, 0};
				query_pointer(NULL, &pt);
				pm = monitor_from_point(pt);
			}
		}
	} else {
		client_t *c = n->client;
		xcb_rectangle_t rect = c->floating_rectangle;
		int16_t x = rect.x + dx;
		int16_t y = rect.y + dy;

		window_move(n->id, x, y);

		c->floating_rectangle.x = x;
		c->floating_rectangle.y = y;
		if (!grabbing) {
			put_status(SBSC_MASK_NODE_GEOMETRY, "node_geometry 0x%08X 0x%08X 0x%08X %ux%u+%i+%i\n", loc->monitor->id, loc->desktop->id, loc->node->id, rect.width, rect.height, x, y);
		}
		pm = monitor_from_client(c);
	}

	if (pm == NULL || pm == loc->monitor) {
		return true;
	}

	transfer_node(loc->monitor, loc->desktop, n, pm, pm->desk, pm->desk->focus, true);
	loc->monitor = pm;
	loc->desktop = pm->desk;

	return true;
}

bool resize_client(coordinates_t *loc, resize_handle_t rh, int dx, int dy, bool relative)
{
	node_t *n = loc->node;
	if (n == NULL || n->client == NULL || n->client->state == STATE_FULLSCREEN) {
		return false;
	}
	node_t *horizontal_fence = NULL, *vertical_fence = NULL;
	xcb_rectangle_t rect = get_rectangle(NULL, NULL, n);
	uint16_t width = rect.width, height = rect.height;
	int16_t x = rect.x, y = rect.y;
	if (n->client->state == STATE_TILED) {
		if (rh & HANDLE_LEFT) {
			vertical_fence = find_fence(n, DIR_WEST);
		} else if (rh & HANDLE_RIGHT) {
			vertical_fence = find_fence(n, DIR_EAST);
		}
		if (rh & HANDLE_TOP) {
			horizontal_fence = find_fence(n, DIR_NORTH);
		} else if (rh & HANDLE_BOTTOM) {
			horizontal_fence = find_fence(n, DIR_SOUTH);
		}
		if (vertical_fence == NULL && horizontal_fence == NULL) {
			return false;
		}
		if (vertical_fence != NULL) {
			double sr = 0.0;
			if (relative) {
				sr = vertical_fence->split_ratio + (double) dx / (double) vertical_fence->rectangle.width;
			} else {
				sr = (double) (dx - vertical_fence->rectangle.x) / (double) vertical_fence->rectangle.width;
			}
			sr = MAX(0, sr);
			sr = MIN(1, sr);
			vertical_fence->split_ratio = sr;
		}
		if (horizontal_fence != NULL) {
			double sr = 0.0;
			if (relative) {
				sr = horizontal_fence->split_ratio + (double) dy / (double) horizontal_fence->rectangle.height;
			} else {
				sr = (double) (dy - horizontal_fence->rectangle.y) / (double) horizontal_fence->rectangle.height;
			}
			sr = MAX(0, sr);
			sr = MIN(1, sr);
			horizontal_fence->split_ratio = sr;
		}
		node_t *target_fence = horizontal_fence != NULL ? horizontal_fence : vertical_fence;
		adjust_ratios(target_fence, target_fence->rectangle);
		arrange(loc->monitor, loc->desktop);
	} else {
		int w = width, h = height;
		if (relative) {
			w += dx * (rh & HANDLE_LEFT ? -1 : (rh & HANDLE_RIGHT ? 1 : 0));
			h += dy * (rh & HANDLE_TOP ? -1 : (rh & HANDLE_BOTTOM ? 1 : 0));
		} else {
			if (rh & HANDLE_LEFT) {
				w = x + width - dx;
			} else if (rh & HANDLE_RIGHT) {
				w = dx - x;
			}
			if (rh & HANDLE_TOP) {
				h = y + height - dy;
			} else if (rh & HANDLE_BOTTOM) {
				h = dy - y;
			}
		}
		width = MAX(1, w);
		height = MAX(1, h);
		apply_size_hints(n->client, &width, &height);
		if (rh & HANDLE_LEFT) {
			x += rect.width - width;
		}
		if (rh & HANDLE_TOP) {
			y += rect.height - height;
		}
		n->client->floating_rectangle = (xcb_rectangle_t) {x, y, width, height};
		if (n->client->state == STATE_FLOATING) {
			window_move_resize(n->id, x, y, width, height);

			if (!grabbing) {
				put_status(SBSC_MASK_NODE_GEOMETRY, "node_geometry 0x%08X 0x%08X 0x%08X %ux%u+%i+%i\n", loc->monitor->id, loc->desktop->id, loc->node->id, width, height, x, y);
			}
		} else {
			arrange(loc->monitor, loc->desktop);
		}
	}
	return true;
}

/* taken from awesomeWM */
void apply_size_hints(client_t *c, uint16_t *width, uint16_t *height)
{
	if (!honor_size_hints) {
		return;
	}

	int32_t minw = 0, minh = 0;
	int32_t basew = 0, baseh = 0, real_basew = 0, real_baseh = 0;

	if (c->state == STATE_FULLSCREEN) {
		return;
	}

	if (c->size_hints.flags & XCB_ICCCM_SIZE_HINT_BASE_SIZE) {
		basew = c->size_hints.base_width;
		baseh = c->size_hints.base_height;
		real_basew = basew;
		real_baseh = baseh;
	} else if (c->size_hints.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE) {
		/* base size is substituted with min size if not specified */
		basew = c->size_hints.min_width;
		baseh = c->size_hints.min_height;
	}

	if (c->size_hints.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE) {
		minw = c->size_hints.min_width;
		minh = c->size_hints.min_height;
	} else if (c->size_hints.flags & XCB_ICCCM_SIZE_HINT_BASE_SIZE) {
		/* min size is substituted with base size if not specified */
		minw = c->size_hints.base_width;
		minh = c->size_hints.base_height;
	}

	/* Handle the size aspect ratio */
	if (c->size_hints.flags & XCB_ICCCM_SIZE_HINT_P_ASPECT &&
	    c->size_hints.min_aspect_den > 0 &&
	    c->size_hints.max_aspect_den > 0 &&
	    *height > real_baseh &&
	    *width > real_basew) {
		/* ICCCM mandates:
		 * If a base size is provided along with the aspect ratio fields, the base size should be subtracted from the
		 * window size prior to checking that the aspect ratio falls in range. If a base size is not provided, nothing
		 * should be subtracted from the window size. (The minimum size is not to be used in place of the base size for
		 * this purpose.)
		 */
		double dx = *width - real_basew;
		double dy = *height - real_baseh;
		double ratio = dx / dy;
		double min = c->size_hints.min_aspect_num / (double) c->size_hints.min_aspect_den;
		double max = c->size_hints.max_aspect_num / (double) c->size_hints.max_aspect_den;

		if (max > 0 && min > 0 && ratio > 0) {
			if (ratio < min) {
				/* dx is lower than allowed, make dy lower to compensate this (+ 0.5 to force proper rounding). */
				dy = dx / min + 0.5;
				*width  = dx + real_basew;
				*height = dy + real_baseh;
			} else if (ratio > max) {
				/* dx is too high, lower it (+0.5 for proper rounding) */
				dx = dy * max + 0.5;
				*width  = dx + real_basew;
				*height = dy + real_baseh;
			}
		}
	}

	/* Handle the minimum size */
	*width = MAX(*width, minw);
	*height = MAX(*height, minh);

	/* Handle the maximum size */
	if (c->size_hints.flags & XCB_ICCCM_SIZE_HINT_P_MAX_SIZE)
	{
		if (c->size_hints.max_width > 0) {
			*width = MIN(*width, c->size_hints.max_width);
		}
		if (c->size_hints.max_height > 0) {
			*height = MIN(*height, c->size_hints.max_height);
		}
	}

	/* Handle the size increment */
	if (c->size_hints.flags & (XCB_ICCCM_SIZE_HINT_P_RESIZE_INC | XCB_ICCCM_SIZE_HINT_BASE_SIZE) &&
	    c->size_hints.width_inc > 0 && c->size_hints.height_inc > 0) {
		uint16_t t1 = *width, t2 = *height;
		unsigned_subtract(t1, basew);
		unsigned_subtract(t2, baseh);
		*width -= t1 % c->size_hints.width_inc;
		*height -= t2 % c->size_hints.height_inc;
	}
}

void query_pointer(xcb_window_t *win, xcb_point_t *pt)
{
	if (motion_recorder.enabled) {
		window_hide(motion_recorder.id);
	}

	xcb_query_pointer_reply_t *qpr = xcb_query_pointer_reply(dpy, xcb_query_pointer(dpy, root), NULL);

	if (qpr != NULL) {
		if (win != NULL) {
			*win = qpr->child;
			xcb_point_t pt = {qpr->root_x, qpr->root_y};
			for (stacking_list_t *s = stack_tail; s != NULL; s = s->prev) {
				if (!s->node->client->shown || s->node->hidden) {
					continue;
				}
				xcb_rectangle_t rect = get_rectangle(NULL, NULL, s->node);
				if (is_inside(pt, rect)) {
					if (s->node->id == qpr->child || is_presel_window(qpr->child)) {
						*win = s->node->id;
					}
					break;
				}
			}
		}
		if (pt != NULL) {
			*pt = (xcb_point_t) {qpr->root_x, qpr->root_y};
		}
	}

	free(qpr);

	if (motion_recorder.enabled) {
		window_show(motion_recorder.id);
	}
}

void update_motion_recorder(void)
{
	xcb_point_t pt;
	xcb_window_t win = XCB_NONE;
	query_pointer(&win, &pt);
	if (win == XCB_NONE) {
		return;
	}
	monitor_t *m = monitor_from_point(pt);
	if (m == NULL) {
		return;
	}
	desktop_t *d = m->desk;
	node_t *n = NULL;
	for (n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root)) {
		if (n->id == win || (n->presel != NULL && n->presel->feedback == win)) {
			break;
		}
	}
	if ((n != NULL && n != mon->desk->focus) || (n == NULL && m != mon)) {
		enable_motion_recorder(win);
	} else {
		disable_motion_recorder();
	}
}

void enable_motion_recorder(xcb_window_t win)
{
	xcb_get_geometry_reply_t *geo = xcb_get_geometry_reply(dpy, xcb_get_geometry(dpy, win), NULL);
	if (geo != NULL) {
		uint16_t width = geo->width + 2 * geo->border_width;
		uint16_t height = geo->height + 2 * geo->border_width;
		window_move_resize(motion_recorder.id, geo->x, geo->y, width, height);
		window_above(motion_recorder.id, win);
		window_show(motion_recorder.id);
		motion_recorder.enabled = true;
	}
	free(geo);
}

void disable_motion_recorder(void)
{
	if (!motion_recorder.enabled) {
		return;
	}
	window_hide(motion_recorder.id);
	motion_recorder.enabled = false;
}

void window_border_width(xcb_window_t win, uint32_t bw)
{
	uint32_t values[] = {bw};
	xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_BORDER_WIDTH, values);
}

void window_move(xcb_window_t win, int16_t x, int16_t y)
{
	uint32_t values[] = {x, y};
	xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_X_Y, values);
}

void window_resize(xcb_window_t win, uint16_t w, uint16_t h)
{
	uint32_t values[] = {w, h};
	xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_WIDTH_HEIGHT, values);
}

void window_move_resize(xcb_window_t win, int16_t x, int16_t y, uint16_t w, uint16_t h)
{
	uint32_t values[] = {x, y, w, h};
	xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_X_Y_WIDTH_HEIGHT, values);
}

void window_center(monitor_t *m, client_t *c)
{
	xcb_rectangle_t *r = &c->floating_rectangle;
	xcb_rectangle_t a = m->rectangle;
	if (r->width >= a.width) {
		r->x = a.x;
	} else {
		r->x = a.x + (a.width - r->width) / 2;
	}
	if (r->height >= a.height) {
		r->y = a.y;
	} else {
		r->y = a.y + (a.height - r->height) / 2;
	}
	r->x -= c->border_width;
	r->y -= c->border_width;
}

void window_stack(xcb_window_t w1, xcb_window_t w2, uint32_t mode)
{
	if (w2 == XCB_NONE) {
		return;
	}
	uint16_t mask = XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE;
	uint32_t values[] = {w2, mode};
	xcb_configure_window(dpy, w1, mask, values);
}

/* Stack w1 above w2 */
void window_above(xcb_window_t w1, xcb_window_t w2)
{
	window_stack(w1, w2, XCB_STACK_MODE_ABOVE);
}

/* Stack w1 below w2 */
void window_below(xcb_window_t w1, xcb_window_t w2)
{
	window_stack(w1, w2, XCB_STACK_MODE_BELOW);
}

void window_lower(xcb_window_t win)
{
	uint32_t values[] = {XCB_STACK_MODE_BELOW};
	xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_STACK_MODE, values);
}

void window_set_visibility(xcb_window_t win, bool visible)
{
	uint32_t values_off[] = {ROOT_EVENT_MASK & ~XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY};
	uint32_t values_on[] = {ROOT_EVENT_MASK};
	xcb_change_window_attributes(dpy, root, XCB_CW_EVENT_MASK, values_off);
	if (visible) {
		set_window_state(win, XCB_ICCCM_WM_STATE_NORMAL);
		xcb_map_window(dpy, win);
	} else {
		xcb_unmap_window(dpy, win);
		set_window_state(win, XCB_ICCCM_WM_STATE_ICONIC);
	}
	xcb_change_window_attributes(dpy, root, XCB_CW_EVENT_MASK, values_on);
}

void window_hide(xcb_window_t win)
{
	window_set_visibility(win, false);
}

void window_show(xcb_window_t win)
{
	window_set_visibility(win, true);
}

void update_input_focus(void)
{
	set_input_focus(mon->desk->focus);
}

void set_input_focus(node_t *n)
{
	if (n == NULL || n->client == NULL) {
		clear_input_focus();
	} else {
		if (n->client->icccm_props.input_hint) {
			xcb_set_input_focus(dpy, XCB_INPUT_FOCUS_PARENT, n->id, XCB_CURRENT_TIME);
		} else if (n->client->icccm_props.take_focus) {
			send_client_message(n->id, ewmh->WM_PROTOCOLS, WM_TAKE_FOCUS);
		}
	}
}

void clear_input_focus(void)
{
	xcb_set_input_focus(dpy, XCB_INPUT_FOCUS_POINTER_ROOT, root, XCB_CURRENT_TIME);
}

void center_pointer(xcb_rectangle_t r)
{
	if (grabbing) {
		return;
	}
	int16_t cx = r.x + r.width / 2;
	int16_t cy = r.y + r.height / 2;
	xcb_warp_pointer(dpy, XCB_NONE, root, 0, 0, 0, 0, cx, cy);
}

void get_atom(char *name, xcb_atom_t *atom)
{
	xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(dpy, xcb_intern_atom(dpy, 0, strlen(name), name), NULL);
	if (reply != NULL) {
		*atom = reply->atom;
	} else {
		*atom = XCB_NONE;
	}
	free(reply);
}

void set_atom(xcb_window_t win, xcb_atom_t atom, uint32_t value)
{
	xcb_change_property(dpy, XCB_PROP_MODE_REPLACE, win, atom, XCB_ATOM_CARDINAL, 32, 1, &value);
}

void send_client_message(xcb_window_t win, xcb_atom_t property, xcb_atom_t value)
{
	xcb_client_message_event_t *e = calloc(32, 1);

	e->response_type = XCB_CLIENT_MESSAGE;
	e->window = win;
	e->type = property;
	e->format = 32;
	e->data.data32[0] = value;
	e->data.data32[1] = XCB_CURRENT_TIME;

	xcb_send_event(dpy, false, win, XCB_EVENT_MASK_NO_EVENT, (char *) e);
	xcb_flush(dpy);
	free(e);
}

bool window_exists(xcb_window_t win)
{
	xcb_generic_error_t *err;
	free(xcb_query_tree_reply(dpy, xcb_query_tree(dpy, win), &err));

	if (err != NULL) {
		free(err);
		return false;
	}

	return true;
}
