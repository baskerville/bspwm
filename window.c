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
#include "monitor.h"
#include "query.h"
#include "rule.h"
#include "settings.h"
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

	rule_consequence_t *csq = make_rule_conquence();
	apply_rules(win, csq);
	if (!schedule_rules(win, csq)) {
		manage_window(win, csq, -1);
		free(csq);
	}
}

void manage_window(xcb_window_t win, rule_consequence_t *csq, int fd)
{
	monitor_t *m = mon;
	desktop_t *d = mon->desk;
	node_t *f = mon->desk->focus;

	parse_rule_consequence(fd, csq);

	if (!csq->manage) {
		free(csq->layer);
		free(csq->state);
		window_show(win);
		return;
	}

	if (csq->node_desc[0] != '\0') {
		coordinates_t ref = {m, d, f};
		coordinates_t trg = {NULL, NULL, NULL};
		if (node_from_desc(csq->node_desc, &ref, &trg)) {
			m = trg.monitor;
			d = trg.desktop;
			f = trg.node;
		}
	} else if (csq->desktop_desc[0] != '\0') {
		coordinates_t ref = {m, d, NULL};
		coordinates_t trg = {NULL, NULL, NULL};
		if (desktop_from_desc(csq->desktop_desc, &ref, &trg)) {
			m = trg.monitor;
			d = trg.desktop;
			f = trg.desktop->focus;
		}
	} else if (csq->monitor_desc[0] != '\0') {
		coordinates_t ref = {m, NULL, NULL};
		coordinates_t trg = {NULL, NULL, NULL};
		if (monitor_from_desc(csq->monitor_desc, &ref, &trg)) {
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
	update_floating_rectangle(n);

	if (c->floating_rectangle.x == 0 && c->floating_rectangle.y == 0) {
		csq->center = true;
	}

	c->min_width = csq->min_width;
	c->max_width = csq->max_width;
	c->min_height = csq->min_height;
	c->max_height = csq->max_height;

	monitor_t *mm = monitor_from_client(c);
	embrace_client(mm, c);
	adapt_geometry(&mm->rectangle, &m->rectangle, n);

	if (csq->center) {
		window_center(m, c);
	}

	snprintf(c->class_name, sizeof(c->class_name), "%s", csq->class_name);
	snprintf(c->instance_name, sizeof(c->instance_name), "%s", csq->instance_name);

	f = insert_node(m, d, n, f);
	clients_count++;

	put_status(SBSC_MASK_NODE_MANAGE, "node_manage %s %s 0x%X 0x%X\n", m->name, d->name, win, f!=NULL?f->id:0);

	if (f != NULL && f->client != NULL && csq->state != NULL && *(csq->state) == STATE_FLOATING) {
		c->last_layer = c->layer = f->client->layer;
	}

	if (csq->layer != NULL) {
		c->last_layer = c->layer = *(csq->layer);
	}

	if (csq->state != NULL) {
		set_state(m, d, n, *(csq->state));
		c->last_state = c->state;
	}

	set_locked(m, d, n, csq->locked);
	set_sticky(m, d, n, csq->sticky);
	set_private(m, d, n, csq->private);

	arrange(m, d);

	bool give_focus = (csq->focus && (d == mon->desk || csq->follow));

	if (give_focus) {
		focus_node(m, d, n);
	} else if (csq->focus) {
		activate_node(m, d, n);
	} else {
		stack(d, n, false);
	}

	uint32_t values[] = {CLIENT_EVENT_MASK | (focus_follows_pointer ? XCB_EVENT_MASK_ENTER_WINDOW : 0)};
	xcb_change_window_attributes(dpy, win, XCB_CW_EVENT_MASK, values);

	if (d == m->desk) {
		window_show(n->id);
	} else {
		window_hide(n->id);
	}

	/* the same function is already called in `focus_node` but has no effects on unmapped windows */
	if (give_focus) {
		xcb_set_input_focus(dpy, XCB_INPUT_FOCUS_POINTER_ROOT, win, XCB_CURRENT_TIME);
	}

	ewmh_set_wm_desktop(n, d);
	ewmh_update_client_list(false);
	free(csq->layer);
	free(csq->state);
}

void unmanage_window(xcb_window_t win)
{
	coordinates_t loc;
	if (locate_window(win, &loc)) {
		put_status(SBSC_MASK_NODE_UNMANAGE, "node_unmanage %s %s 0x%X\n", loc.monitor, loc.desktop, win);
		remove_node(loc.monitor, loc.desktop, loc.node);
		if (frozen_pointer->window == win) {
			frozen_pointer->action = ACTION_NONE;
		}
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

void initialize_presel_feedback(node_t *n)
{
	if (n == NULL || n->presel == NULL || n->presel->feedback != XCB_NONE) {
		return;
	}

	xcb_window_t win = xcb_generate_id(dpy);
	xcb_create_window(dpy, XCB_COPY_FROM_PARENT, win, root, 0, 0, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
			          XCB_COPY_FROM_PARENT, XCB_CW_BACK_PIXEL, (uint32_t []) {get_color_pixel(presel_feedback_color)});

	xcb_icccm_set_wm_class(dpy, win, sizeof(PRESEL_FEEDBACK_IC), PRESEL_FEEDBACK_IC);
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
	if (n == NULL || n->presel == NULL) {
		return;
	}

	bool exists = (n->presel->feedback != XCB_NONE);
	if (!exists) {
		initialize_presel_feedback(n);
	}

	presel_t *p = n->presel;
	xcb_rectangle_t rect = n->rectangle;
	rect.x = rect.y = 0;
	rect.width -= d->window_gap;
	rect.height -= d->window_gap;
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

void refresh_presel_feebacks_in(node_t *n, desktop_t *d, monitor_t *m)
{
	if (n == NULL) {
		return;
	} else {
		if (n->presel != NULL) {
			draw_presel_feedback(m, d, n);
		}
		refresh_presel_feebacks_in(n->first_child, d, m);
		refresh_presel_feebacks_in(n->second_child, d, m);
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
		if (f->client->border_width > 0) {
			window_draw_border(f->id, border_color_pxl);
		}
	}
}

void window_draw_border(xcb_window_t win, uint32_t border_color_pxl)
{
	xcb_change_window_attributes(dpy, win, XCB_CW_BORDER_PIXEL, &border_color_pxl);
}

pointer_state_t *make_pointer_state(void)
{
	pointer_state_t *p = malloc(sizeof(pointer_state_t));
	p->monitor = NULL;
	p->desktop = NULL;
	p->node = p->vertical_fence = p->horizontal_fence = NULL;
	p->client = NULL;
	p->window = XCB_NONE;
	p->action = ACTION_NONE;
	return p;
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

void window_close(node_t *n)
{
	if (n == NULL) {
		return;
	} else if (n->client != NULL) {
		send_client_message(n->id, ewmh->WM_PROTOCOLS, WM_DELETE_WINDOW);
	} else {
		window_close(n->first_child);
		window_close(n->second_child);
	}
}

void window_kill(monitor_t *m, desktop_t *d, node_t *n)
{
	if (n == NULL) {
		return;
	}

	for (node_t *f = first_extrema(n); f != NULL; f = next_leaf(f, n)) {
		xcb_kill_client(dpy, f->id);
	}

	remove_node(m, d, n);
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

void update_floating_rectangle(node_t *n)
{
	client_t *c = n->client;

	xcb_get_geometry_reply_t *geo = xcb_get_geometry_reply(dpy, xcb_get_geometry(dpy, n->id), NULL);

	if (geo != NULL) {
		c->floating_rectangle = (xcb_rectangle_t) {geo->x, geo->y, geo->width, geo->height};
	}

	free(geo);
}

void restrain_floating_width(client_t *c, int *width)
{
	if (*width < 1) {
		*width = 1;
	}
	if (c->min_width > 0 && *width < c->min_width) {
		*width = c->min_width;
	} else if (c->max_width > 0 && *width > c->max_width) {
		*width = c->max_width;
	}
}

void restrain_floating_height(client_t *c, int *height)
{
	if (*height < 1) {
		*height = 1;
	}
	if (c->min_height > 0 && *height < c->min_height) {
		*height = c->min_height;
	} else if (c->max_height > 0 && *height > c->max_height) {
		*height = c->max_height;
	}
}

void restrain_floating_size(client_t *c, int *width, int *height)
{
	restrain_floating_width(c, width);
	restrain_floating_height(c, height);
}

void query_pointer(xcb_window_t *win, xcb_point_t *pt)
{
	window_lower(motion_recorder);

	xcb_query_pointer_reply_t *qpr = xcb_query_pointer_reply(dpy, xcb_query_pointer(dpy, root), NULL);

	if (qpr != NULL) {
		if (win != NULL) {
			*win = qpr->child;
		}
		if (pt != NULL) {
			*pt = (xcb_point_t) {qpr->root_x, qpr->root_y};
		}
	}

	free(qpr);

	window_raise(motion_recorder);
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

void window_raise(xcb_window_t win)
{
	uint32_t values[] = {XCB_STACK_MODE_ABOVE};
	xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_STACK_MODE, values);
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

void window_above(xcb_window_t w1, xcb_window_t w2)
{
	window_stack(w1, w2, XCB_STACK_MODE_ABOVE);
}

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
		xcb_map_window(dpy, win);
	} else {
		xcb_unmap_window(dpy, win);
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

void enable_motion_recorder(void)
{
	window_raise(motion_recorder);
	window_show(motion_recorder);
}

void disable_motion_recorder(void)
{
	window_hide(motion_recorder);
}

void update_motion_recorder(void)
{
	xcb_get_geometry_reply_t *geo = xcb_get_geometry_reply(dpy, xcb_get_geometry(dpy, root), NULL);

	if (geo != NULL) {
		window_resize(motion_recorder, geo->width, geo->height);
	}

	free(geo);
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
		if (n->client->icccm_input) {
			xcb_set_input_focus(dpy, XCB_INPUT_FOCUS_PARENT, n->id, XCB_CURRENT_TIME);
		} else if (n->client->icccm_focus) {
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
	int16_t cx = r.x + r.width / 2;
	int16_t cy = r.y + r.height / 2;
	window_lower(motion_recorder);
	xcb_warp_pointer(dpy, XCB_NONE, root, 0, 0, 0, 0, cx, cy);
	window_raise(motion_recorder);
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

bool has_proto(xcb_atom_t atom, xcb_icccm_get_wm_protocols_reply_t *protocols)
{
	for (uint32_t i = 0; i < protocols->atoms_len; i++) {
		if (protocols->atoms[i] == atom) {
			return true;
		}
	}
	return false;
}

void send_client_message(xcb_window_t win, xcb_atom_t property, xcb_atom_t value)
{
	xcb_client_message_event_t e;

	e.response_type = XCB_CLIENT_MESSAGE;
	e.window = win;
	e.format = 32;
	e.sequence = 0;
	e.type = property;
	e.data.data32[0] = value;
	e.data.data32[1] = XCB_CURRENT_TIME;

	xcb_send_event(dpy, false, win, XCB_EVENT_MASK_NO_EVENT, (char *) &e);
}
