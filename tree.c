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

#include <float.h>
#include <limits.h>
#include "bspwm.h"
#include "desktop.h"
#include "ewmh.h"
#include "history.h"
#include "monitor.h"
#include "query.h"
#include "settings.h"
#include "stack.h"
#include "window.h"
#include "tree.h"

void arrange(monitor_t *m, desktop_t *d)
{
	if (d->root == NULL)
		return;

	PRINTF("arrange %s %s\n", m->name, d->name);

	xcb_rectangle_t rect = m->rectangle;
	int wg = (gapless_monocle && d->layout == LAYOUT_MONOCLE ? 0 : d->window_gap);
	rect.x += m->left_padding + d->left_padding + wg;
	rect.y += m->top_padding + d->top_padding + wg;
	rect.width -= m->left_padding + d->left_padding + d->right_padding + m->right_padding + wg;
	rect.height -= m->top_padding + d->top_padding + d->bottom_padding + m->bottom_padding + wg;
	apply_layout(m, d, d->root, rect, rect);
}

void apply_layout(monitor_t *m, desktop_t *d, node_t *n, xcb_rectangle_t rect, xcb_rectangle_t root_rect)
{
	if (n == NULL)
		return;

	n->rectangle = rect;

	if (is_leaf(n)) {

		unsigned int bw;
		if ((borderless_monocle && is_tiled(n->client) &&
		     !n->client->pseudo_tiled &&
		     d->layout == LAYOUT_MONOCLE) ||
		    n->client->fullscreen)
			bw = 0;
		else
			bw = n->client->border_width;

		xcb_rectangle_t r;
		if (!n->client->fullscreen) {
			if (!n->client->floating) {
				int wg = (gapless_monocle && d->layout == LAYOUT_MONOCLE ? 0 : d->window_gap);
				if (n->client->pseudo_tiled) {
				/* pseudo-tiled clients */
					r = n->client->floating_rectangle;
					r.x = rect.x - bw + (rect.width - wg - r.width) / 2;
					r.y = rect.y - bw + (rect.height - wg - r.height) / 2;
				} else {
					/* tiled clients */
					r = rect;
					int bleed = wg + 2 * bw;
					r.width = (bleed < r.width ? r.width - bleed : 1);
					r.height = (bleed < r.height ? r.height - bleed : 1);
				}
				n->client->tiled_rectangle = r;
			} else {
				/* floating clients */
				r = n->client->floating_rectangle;
			}
		} else {
			/* fullscreen clients */
			r = m->rectangle;
		}

		window_move_resize(n->client->window, r.x, r.y, r.width, r.height);
		window_border_width(n->client->window, bw);
		window_draw_border(n, d->focus == n, m == mon);

		if (pointer_follows_focus && mon->desk->focus == n && frozen_pointer->action == ACTION_NONE) {
			center_pointer(r);
		}

	} else {
		xcb_rectangle_t first_rect;
		xcb_rectangle_t second_rect;

		if (d->layout == LAYOUT_MONOCLE || n->first_child->vacant || n->second_child->vacant) {
			first_rect = second_rect = rect;
		} else {
			unsigned int fence;
			if (n->split_type == TYPE_VERTICAL) {
				fence = rect.width * n->split_ratio;
				first_rect = (xcb_rectangle_t) {rect.x, rect.y, fence, rect.height};
				second_rect = (xcb_rectangle_t) {rect.x + fence, rect.y, rect.width - fence, rect.height};
			} else {
				fence = rect.height * n->split_ratio;
				first_rect = (xcb_rectangle_t) {rect.x, rect.y, rect.width, fence};
				second_rect = (xcb_rectangle_t) {rect.x, rect.y + fence, rect.width, rect.height - fence};
			}
		}

		apply_layout(m, d, n->first_child, first_rect, root_rect);
		apply_layout(m, d, n->second_child, second_rect, root_rect);
	}
}

void insert_node(monitor_t *m, desktop_t *d, node_t *n, node_t *f)
{
	if (d == NULL || n == NULL)
		return;

	PRINTF("insert node %X\n", n->client->window);

	/* n: new leaf node */
	/* c: new container node */
	/* f: focus or insertion anchor */
	/* p: parent of focus */
	/* g: grand parent of focus */

	if (f == NULL)
		f = d->root;

	if (f == NULL) {
		d->root = n;
	} else {
		node_t *c = make_node();
		node_t *p = f->parent;
		if (p != NULL && f->split_mode == MODE_AUTOMATIC &&
		    (p->first_child->vacant || p->second_child->vacant)) {
			f = p;
			p = f->parent;
		}
		if (((f->client != NULL && f->client->private) ||
		     (p != NULL && p->privacy_level > 0)) &&
		    f->split_mode == MODE_AUTOMATIC) {
			node_t *closest = NULL;
			node_t *public = NULL;
			closest_public(d, f, &closest, &public);
			if (public != NULL) {
				f = public;
				p = f->parent;
			} else {
				if (closest != NULL) {
					f = closest;
					p = f->parent;
				}
				f->split_mode = MODE_MANUAL;
				xcb_rectangle_t rect = f->client->tiled_rectangle;
				f->split_dir = (rect.width >= rect.height ? DIR_LEFT : DIR_UP);
				if (f->client->private) {
					get_opposite(f->split_dir, &f->split_dir);
					update_privacy_level(f, false);
				}
			}
		}
		n->parent = c;
		c->birth_rotation = f->birth_rotation;
		switch (f->split_mode) {
			case MODE_AUTOMATIC:
				if (p == NULL) {
					c->first_child = n;
					c->second_child = f;
					if (m->rectangle.width > m->rectangle.height)
						c->split_type = TYPE_VERTICAL;
					else
						c->split_type = TYPE_HORIZONTAL;
					f->parent = c;
					d->root = c;
				} else {
					node_t *g = p->parent;
					c->parent = g;
					if (g != NULL) {
						if (is_first_child(p))
							g->first_child = c;
						else
							g->second_child = c;
					} else {
						d->root = c;
					}
					c->split_type = p->split_type;
					c->split_ratio = p->split_ratio;
					p->parent = c;
					int rot;
					if (is_first_child(f)) {
						c->first_child = n;
						c->second_child = p;
						rot = 90;
					} else {
						c->first_child = p;
						c->second_child = n;
						rot = 270;
					}
					if (!is_floating(n->client))
						rotate_tree(p, rot);
					n->birth_rotation = rot;
				}
				break;
			case MODE_MANUAL:
				if (p != NULL) {
					if (is_first_child(f))
						p->first_child = c;
					else
						p->second_child = c;
				}
				c->split_ratio = f->split_ratio;
				c->parent = p;
				f->parent = c;
				f->birth_rotation = 0;
				switch (f->split_dir) {
					case DIR_LEFT:
						c->split_type = TYPE_VERTICAL;
						c->first_child = n;
						c->second_child = f;
						break;
					case DIR_RIGHT:
						c->split_type = TYPE_VERTICAL;
						c->first_child = f;
						c->second_child = n;
						break;
					case DIR_UP:
						c->split_type = TYPE_HORIZONTAL;
						c->first_child = n;
						c->second_child = f;
						break;
					case DIR_DOWN:
						c->split_type = TYPE_HORIZONTAL;
						c->first_child = f;
						c->second_child = n;
						break;
				}
				if (d->root == f)
					d->root = c;
				f->split_mode = MODE_AUTOMATIC;
				break;
		}
		if (f->vacant)
			update_vacant_state(f->parent);
		if (f->client != NULL && f->client->private)
			update_privacy_level(f, true);
	}
	if (n->client->private)
		update_privacy_level(n, true);
	if (d->focus == NULL)
		d->focus = n;
	if (n->client->sticky)
		m->num_sticky++;
	put_status();
}

void pseudo_focus(monitor_t *m, desktop_t *d, node_t *n)
{
	if (n != NULL) {
		stack(n, STACK_ABOVE);
		if (d->focus != n) {
			window_draw_border(d->focus, false, m == mon);
			window_draw_border(n, true, m == mon);
		}
	}
	d->focus = n;
}

void focus_node(monitor_t *m, desktop_t *d, node_t *n)
{
	if (mon->desk != d || n == NULL)
		clear_input_focus();

	if (m->num_sticky > 0 && d != m->desk) {
		node_t *a = first_extrema(m->desk->root);
		sticky_still = false;
		while (a != NULL) {
			node_t *b = next_leaf(a, m->desk->root);
			if (a->client->sticky)
				transfer_node(m, m->desk, a, m, d, d->focus);
			a = b;
		}
		sticky_still = true;
		if (n == NULL && d->focus != NULL)
			n = d->focus;
	}

	if (n != NULL) {
		if (d->focus != NULL && n != d->focus && d->focus->client->fullscreen) {
			set_fullscreen(d->focus, false);
			arrange(m, d);
		}
		if (n->client->urgent) {
			n->client->urgent = false;
			put_status();
		}
	}

	if (mon != m) {
		for (desktop_t *cd = mon->desk_head; cd != NULL; cd = cd->next)
			window_draw_border(cd->focus, true, false);
		for (desktop_t *cd = m->desk_head; cd != NULL; cd = cd->next)
			if (cd != d)
				window_draw_border(cd->focus, true, true);
		if (d->focus == n)
			window_draw_border(n, true, true);
	}

	if (d->focus != n) {
		window_draw_border(d->focus, false, true);
		window_draw_border(n, true, true);
	}

	focus_desktop(m, d);

	d->focus = n;

	if (n == NULL) {
		history_add(m, d, NULL);
		ewmh_update_active_window();
		return;
	} else {
		stack(n, STACK_ABOVE);
	}

	PRINTF("focus node %X\n", n->client->window);

	history_add(m, d, n);
	set_input_focus(n);

	if (focus_follows_pointer) {
		xcb_window_t win = XCB_NONE;
		query_pointer(&win, NULL);
		if (win != n->client->window)
			enable_motion_recorder();
		else
			disable_motion_recorder();
	}

	if (pointer_follows_focus) {
		center_pointer(get_rectangle(n->client));
	}

	ewmh_update_active_window();
}

void update_current(void)
{
	focus_node(mon, mon->desk, mon->desk->focus);
}

node_t *make_node(void)
{
	node_t *n = malloc(sizeof(node_t));
	n->parent = n->first_child = n->second_child = NULL;
	n->split_ratio = split_ratio;
	n->split_mode = MODE_AUTOMATIC;
	n->split_type = TYPE_VERTICAL;
	n->birth_rotation = 0;
	n->privacy_level = 0;
	n->client = NULL;
	n->vacant = false;
	return n;
}

client_t *make_client(xcb_window_t win, unsigned int border_width)
{
	client_t *c = malloc(sizeof(client_t));
	c->window = win;
	snprintf(c->class_name, sizeof(c->class_name), "%s", MISSING_VALUE);
	snprintf(c->instance_name, sizeof(c->instance_name), "%s", MISSING_VALUE);
	c->border_width = border_width;
	c->pseudo_tiled = c->floating = c->fullscreen = false;
	c->locked = c->sticky = c->urgent = c->private = c->icccm_focus = false;
	xcb_icccm_get_wm_protocols_reply_t protocols;
	if (xcb_icccm_get_wm_protocols_reply(dpy, xcb_icccm_get_wm_protocols(dpy, win, ewmh->WM_PROTOCOLS), &protocols, NULL) == 1) {
		if (has_proto(WM_TAKE_FOCUS, &protocols))
			c->icccm_focus = true;
		xcb_icccm_get_wm_protocols_reply_wipe(&protocols);
	}
	c->num_states = 0;
	xcb_ewmh_get_atoms_reply_t wm_state;
	if (xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, win), &wm_state, NULL) == 1) {
		for (unsigned int i = 0; i < wm_state.atoms_len && i < MAX_STATE; i++)
			ewmh_wm_state_add(c, wm_state.atoms[i]);
		xcb_ewmh_get_atoms_reply_wipe(&wm_state);
	}
	return c;
}

bool is_leaf(node_t *n)
{
	return (n != NULL && n->first_child == NULL && n->second_child == NULL);
}

bool is_tiled(client_t *c)
{
	if (c == NULL)
		return false;
	return (!c->floating && !c->fullscreen);
}

bool is_floating(client_t *c)
{
	if (c == NULL)
		return false;
	return (c->floating && !c->fullscreen);
}

bool is_first_child(node_t *n)
{
	return (n != NULL && n->parent != NULL && n->parent->first_child == n);
}

bool is_second_child(node_t *n)
{
	return (n != NULL && n->parent != NULL && n->parent->second_child == n);
}

void reset_mode(coordinates_t *loc)
{
	if (loc->node != NULL) {
		loc->node->split_mode = MODE_AUTOMATIC;
		window_draw_border(loc->node, loc->desktop->focus == loc->node, mon == loc->monitor);
	} else if (loc->desktop != NULL) {
		for (node_t *a = first_extrema(loc->desktop->root); a != NULL; a = next_leaf(a, loc->desktop->root)) {
			a->split_mode = MODE_AUTOMATIC;
			window_draw_border(a, loc->desktop->focus == a, mon == loc->monitor);
		}
	}
}

node_t *brother_tree(node_t *n)
{
	if (n == NULL || n->parent == NULL)
		return NULL;
	if (is_first_child(n))
		return n->parent->second_child;
	else
		return n->parent->first_child;
}

void closest_public(desktop_t *d, node_t *n, node_t **closest, node_t **public)
{
	if (n == NULL)
		return;
	node_t *prev = prev_leaf(n, d->root);
	node_t *next = next_leaf(n, d->root);
	while (prev != NULL || next != NULL) {
#define TESTLOOP(n) \
		if (n != NULL) { \
			if (is_tiled(n->client)) { \
				if (n->privacy_level == 0) { \
					if (n->parent == NULL || n->parent->privacy_level == 0) { \
						*public = n; \
						return; \
					} else if (*closest == NULL) { \
						*closest = n; \
					} \
				} \
			} \
			n = n##_leaf(n, d->root); \
		}
		TESTLOOP(prev)
		TESTLOOP(next)
#undef TESTLOOP
	}
}

node_t *first_extrema(node_t *n)
{
	if (n == NULL)
		return NULL;
	else if (n->first_child == NULL)
		return n;
	else
		return first_extrema(n->first_child);
}

node_t *second_extrema(node_t *n)
{
	if (n == NULL)
		return NULL;
	else if (n->second_child == NULL)
		return n;
	else
		return second_extrema(n->second_child);
}

node_t *next_leaf(node_t *n, node_t *r)
{
	if (n == NULL)
		return NULL;
	node_t *p = n;
	while (is_second_child(p) && p != r)
		p = p->parent;
	if (p == r)
		return NULL;
	return first_extrema(p->parent->second_child);
}

node_t *prev_leaf(node_t *n, node_t *r)
{
	if (n == NULL)
		return NULL;
	node_t *p = n;
	while (is_first_child(p) && p != r)
		p = p->parent;
	if (p == r)
		return NULL;
	return second_extrema(p->parent->first_child);
}

node_t *next_tiled_leaf(desktop_t *d, node_t *n, node_t *r)
{
	node_t *next = next_leaf(n, r);
	if (next == NULL || is_tiled(next->client))
		return next;
	else
		return next_tiled_leaf(d, next, r);
}

node_t *prev_tiled_leaf(desktop_t *d, node_t *n, node_t *r)
{
	node_t *prev = prev_leaf(n, r);
	if (prev == NULL || is_tiled(prev->client))
		return prev;
	else
		return prev_tiled_leaf(d, prev, r);
}

/* bool is_adjacent(node_t *a, node_t *r) */
/* { */
/*	   node_t *f = r->parent; */
/*	   node_t *p = a; */
/*	   bool first_child = is_first_child(r); */
/*	   while (p != r) { */
/*		   if (p->parent->split_type == f->split_type && is_first_child(p) == first_child) */
/*			   return false; */
/*		   p = p->parent; */
/*	   } */
/*	   return true; */
/* } */

/* Returns true if *b* is adjacent to *a* in the direction *dir* */
bool is_adjacent(node_t *a, node_t *b, direction_t dir)
{
	switch (dir) {
		case DIR_RIGHT:
			return (a->rectangle.x + a->rectangle.width) == b->rectangle.x;
			break;
		case DIR_DOWN:
			return (a->rectangle.y + a->rectangle.height) == b->rectangle.y;
			break;
		case DIR_LEFT:
			return (b->rectangle.x + b->rectangle.width) == a->rectangle.x;
			break;
		case DIR_UP:
			return (b->rectangle.y + b->rectangle.height) == a->rectangle.y;
			break;
	}
	return false;
}

node_t *find_fence(node_t *n, direction_t dir)
{
	node_t *p;

	if (n == NULL)
		return NULL;

	p = n->parent;

	while (p != NULL) {
		if ((dir == DIR_UP && p->split_type == TYPE_HORIZONTAL && p->rectangle.y < n->rectangle.y) ||
		    (dir == DIR_LEFT && p->split_type == TYPE_VERTICAL && p->rectangle.x < n->rectangle.x) ||
		    (dir == DIR_DOWN && p->split_type == TYPE_HORIZONTAL && (p->rectangle.y + p->rectangle.height) > (n->rectangle.y + n->rectangle.height)) ||
		    (dir == DIR_RIGHT && p->split_type == TYPE_VERTICAL && (p->rectangle.x + p->rectangle.width) > (n->rectangle.x + n->rectangle.width)))
			return p;
		p = p->parent;
	}

	return NULL;
}

node_t *nearest_neighbor(monitor_t *m, desktop_t *d, node_t *n, direction_t dir, client_select_t sel)
{
	if (n == NULL || n->client->fullscreen ||
	    (d->layout == LAYOUT_MONOCLE && is_tiled(n->client)))
		return NULL;

	node_t *nearest = NULL;
	if (history_aware_focus)
		nearest = nearest_from_history(m, d, n, dir, sel);
    if (nearest == NULL) {
        if (focus_by_distance) {
            nearest = nearest_from_distance(m, d, n, dir, sel);
        } else {
            nearest = nearest_from_tree(m, d, n, dir, sel);
        }
    }
    return nearest;
}

node_t *nearest_from_tree(monitor_t *m, desktop_t *d, node_t *n, direction_t dir, client_select_t sel)
{
    if (n == NULL)
        return NULL;

    node_t *fence = find_fence(n, dir);

    if (fence == NULL)
        return NULL;

    node_t *nearest = NULL;

    if (dir == DIR_UP || dir == DIR_LEFT)
        nearest = second_extrema(fence->first_child);
    else if (dir == DIR_DOWN || dir == DIR_RIGHT)
        nearest = first_extrema(fence->second_child);

	coordinates_t ref = {m, d, n};
	coordinates_t loc = {m, d, nearest};

	if (node_matches(&loc, &ref, sel))
		return nearest;
	else
		return NULL;
}

node_t *nearest_from_history(monitor_t *m, desktop_t *d, node_t *n, direction_t dir, client_select_t sel)
{
	if (n == NULL || !is_tiled(n->client))
		return NULL;

	node_t *target = find_fence(n, dir);
	if (target == NULL)
		return NULL;
	if (dir == DIR_UP || dir == DIR_LEFT)
		target = target->first_child;
	else if (dir == DIR_DOWN || dir == DIR_RIGHT)
		target = target->second_child;

	node_t *nearest = NULL;
	int min_rank = INT_MAX;
	coordinates_t ref = {m, d, n};

	for (node_t *a = first_extrema(target); a != NULL; a = next_leaf(a, target)) {
		if (a->vacant || !is_adjacent(n, a, dir) || a == n)
			continue;
		coordinates_t loc = {m, d, a};
		if (!node_matches(&loc, &ref, sel))
			continue;

		int rank = history_rank(d, a);
		if (rank >= 0 && rank < min_rank) {
			nearest = a;
			min_rank = rank;
		}
	}

	return nearest;
}

node_t *nearest_from_distance(monitor_t *m, desktop_t *d, node_t *n, direction_t dir, client_select_t sel)
{
	if (n == NULL)
		return NULL;

	node_t *target = NULL;

	if (is_tiled(n->client)) {
		target = find_fence(n, dir);
		if (target == NULL)
			return NULL;
		if (dir == DIR_UP || dir == DIR_LEFT)
			target = target->first_child;
		else if (dir == DIR_DOWN || dir == DIR_RIGHT)
			target = target->second_child;
	} else {
		target = d->root;
	}

	node_t *nearest = NULL;
	direction_t dir2;
	xcb_point_t pt;
	xcb_point_t pt2;
	get_side_handle(n->client, dir, &pt);
	get_opposite(dir, &dir2);
	double ds = DBL_MAX;
	coordinates_t ref = {m, d, n};

	for (node_t *a = first_extrema(target); a != NULL; a = next_leaf(a, target)) {
		coordinates_t loc = {m, d, a};
		if (a == n ||
		    !node_matches(&loc, &ref, sel) ||
		    is_tiled(a->client) != is_tiled(n->client) ||
		    (is_tiled(a->client) && !is_adjacent(n, a, dir)))
			continue;

		get_side_handle(a->client, dir2, &pt2);
		double ds2 = distance(pt, pt2);
		if (ds2 < ds) {
			ds = ds2;
			nearest = a;
		}
	}

	return nearest;
}

void get_opposite(direction_t src, direction_t *dst)
{
	switch (src) {
		case DIR_RIGHT:
			*dst = DIR_LEFT;
			break;
		case DIR_DOWN:
			*dst = DIR_UP;
			break;
		case DIR_LEFT:
			*dst = DIR_RIGHT;
			break;
		case DIR_UP:
			*dst = DIR_DOWN;
			break;
	}
}

int tiled_area(node_t *n)
{
	if (n == NULL)
		return -1;
	xcb_rectangle_t rect = n->client->tiled_rectangle;
	return rect.width * rect.height;
}

node_t *find_biggest(monitor_t *m, desktop_t *d, node_t *n, client_select_t sel)
{
	if (d == NULL)
		return NULL;

	node_t *r = NULL;
	int r_area = tiled_area(r);
	coordinates_t ref = {m, d, n};

	for (node_t *f = first_extrema(d->root); f != NULL; f = next_leaf(f, d->root)) {
		coordinates_t loc = {m, d, f};
		if (!is_tiled(f->client) || !node_matches(&loc, &ref, sel))
			continue;
		int f_area = tiled_area(f);
		if (r == NULL) {
			r = f;
			r_area = f_area;
		} else if (f_area > r_area) {
			r = f;
			r_area = f_area;
		}
	}

	return r;
}

void rotate_tree(node_t *n, int deg)
{
	if (n == NULL || is_leaf(n) || deg == 0)
		return;

	node_t *tmp;

	if ((deg == 90 && n->split_type == TYPE_HORIZONTAL) ||
	    (deg == 270 && n->split_type == TYPE_VERTICAL) ||
	    deg == 180) {
		tmp = n->first_child;
		n->first_child = n->second_child;
		n->second_child = tmp;
		n->split_ratio = 1.0 - n->split_ratio;
	}

	if (deg != 180) {
		if (n->split_type == TYPE_HORIZONTAL)
			n->split_type = TYPE_VERTICAL;
		else if (n->split_type == TYPE_VERTICAL)
			n->split_type = TYPE_HORIZONTAL;
	}

	rotate_tree(n->first_child, deg);
	rotate_tree(n->second_child, deg);
}

void rotate_brother(node_t *n)
{
	rotate_tree(brother_tree(n), n->birth_rotation);
}

void unrotate_tree(node_t *n, int rot)
{
	if (rot == 0)
		return;
	rotate_tree(n, 360 - rot);
}

void unrotate_brother(node_t *n)
{
	unrotate_tree(brother_tree(n), n->birth_rotation);
}

void flip_tree(node_t *n, flip_t flp)
{
	if (n == NULL || is_leaf(n))
		return;

	node_t *tmp;

	if ((flp == FLIP_HORIZONTAL && n->split_type == TYPE_HORIZONTAL) ||
	    (flp == FLIP_VERTICAL && n->split_type == TYPE_VERTICAL)) {
		tmp = n->first_child;
		n->first_child = n->second_child;
		n->second_child = tmp;
		n->split_ratio = 1.0 - n->split_ratio;
	}

	flip_tree(n->first_child, flp);
	flip_tree(n->second_child, flp);
}

void equalize_tree(node_t *n)
{
	if (n == NULL || n->vacant) {
		return;
	} else {
		n->split_ratio = split_ratio;
		equalize_tree(n->first_child);
		equalize_tree(n->second_child);
	}
}

int balance_tree(node_t *n)
{
	if (n == NULL || n->vacant) {
		return 0;
	} else if (is_leaf(n)) {
		return 1;
	} else {
		int b1 = balance_tree(n->first_child);
		int b2 = balance_tree(n->second_child);
		int b = b1 + b2;
		if (b1 > 0 && b2 > 0)
			n->split_ratio = (double) b1 / b;
		return b;
	}
}

void unlink_node(monitor_t *m, desktop_t *d, node_t *n)
{
	if (d == NULL || n == NULL)
		return;

	PRINTF("unlink node %X\n", n->client->window);

	node_t *p = n->parent;

	if (p == NULL) {
		d->root = NULL;
		d->focus = NULL;
	} else {
		if (n->client->private)
			update_privacy_level(n, false);

		node_t *b;
		node_t *g = p->parent;

		if (is_first_child(n)) {
			b = p->second_child;
			if (!n->vacant)
				unrotate_tree(b, n->birth_rotation);
		} else {
			b = p->first_child;
			if (!n->vacant)
				unrotate_tree(b, n->birth_rotation);
		}

		b->parent = g;

		if (g != NULL) {
			if (is_first_child(p))
				g->first_child = b;
			else
				g->second_child = b;
		} else {
			d->root = b;
		}

		b->birth_rotation = p->birth_rotation;
		n->parent = NULL;
		free(p);
		update_vacant_state(b->parent);

		if (n == d->focus) {
			d->focus = history_get_node(d, n);
			// fallback to the first extrema (`n` is not reachable)
			if (d->focus == NULL)
				d->focus = first_extrema(d->root);
		}
	}
	if (n->client->sticky)
		m->num_sticky--;
	put_status();
}

void remove_node(monitor_t *m, desktop_t *d, node_t *n)
{
	if (n == NULL)
		return;

	PRINTF("remove node %X\n", n->client->window);

	bool focused = (n == mon->desk->focus);
	unlink_node(m, d, n);
	history_remove(d, n);
	remove_stack_node(n);
	free(n->client);
	free(n);

	num_clients--;
	ewmh_update_client_list();

	if (focused)
		update_current();
}

void destroy_tree(node_t *n)
{
	if (n == NULL)
		return;
	node_t *first_tree = n->first_child;
	node_t *second_tree = n->second_child;
	if (n->client != NULL) {
		free(n->client);
		num_clients--;
	}
	free(n);
	destroy_tree(first_tree);
	destroy_tree(second_tree);
}

bool swap_nodes(monitor_t *m1, desktop_t *d1, node_t *n1, monitor_t *m2, desktop_t *d2, node_t *n2)
{
	if (n1 == NULL || n2 == NULL ||n1 == n2 ||
	    (d1 != d2 && (n1->client->sticky || n2->client->sticky)))
		return false;

	PRINTF("swap nodes %X %X\n", n1->client->window, n2->client->window);

	node_t *pn1 = n1->parent;
	node_t *pn2 = n2->parent;
	bool n1_first_child = is_first_child(n1);
	bool n2_first_child = is_first_child(n2);
	int br1 = n1->birth_rotation;
	int br2 = n2->birth_rotation;
	int pl1 = n1->privacy_level;
	int pl2 = n2->privacy_level;

	if (pn1 != NULL) {
		if (n1_first_child)
			pn1->first_child = n2;
		else
			pn1->second_child = n2;
	}

	if (pn2 != NULL) {
		if (n2_first_child)
			pn2->first_child = n1;
		else
			pn2->second_child = n1;
	}

	n1->parent = pn2;
	n2->parent = pn1;
	n1->birth_rotation = br2;
	n2->birth_rotation = br1;
	n1->privacy_level = pl2;
	n2->privacy_level = pl1;

	if (n1->vacant != n2->vacant) {
		update_vacant_state(n1->parent);
		update_vacant_state(n2->parent);
	}

	if (n1->client->private != n2->client->private) {
		n1->client->private = !n1->client->private;
		n2->client->private = !n2->client->private;
	}

	if (d1 != d2) {
		if (d1->root == n1)
			d1->root = n2;
		if (d1->focus == n1)
			d1->focus = n2;
		if (d2->root == n2)
			d2->root = n1;
		if (d2->focus == n2)
			d2->focus = n1;

		if (m1 != m2) {
			translate_client(m2, m1, n2->client);
			translate_client(m1, m2, n1->client);
		}

		ewmh_set_wm_desktop(n1, d2);
		ewmh_set_wm_desktop(n2, d1);
		history_swap_nodes(m1, d1, n1, m2, d2, n2);

		if (m1->desk != d1 && m2->desk == d2) {
			window_show(n1->client->window);
			window_hide(n2->client->window);
		} else if (m1->desk == d1 && m2->desk != d2) {
			window_hide(n1->client->window);
			window_show(n2->client->window);
		}

		update_input_focus();
	}

	return true;
}

bool transfer_node(monitor_t *ms, desktop_t *ds, node_t *ns, monitor_t *md, desktop_t *dd, node_t *nd)
{
	if (ns == NULL || ns == nd || (sticky_still && ns->client->sticky))
		return false;

	PRINTF("transfer node %X\n", ns->client->window);

	bool focused = (ns == mon->desk->focus);
	bool active = (ns == ds->focus);

	if (focused)
		clear_input_focus();

	unlink_node(ms, ds, ns);
	insert_node(md, dd, ns, nd);

	if (md != ms)
		translate_client(ms, md, ns->client);

	if (ds != dd) {
		ewmh_set_wm_desktop(ns, dd);
		if (!ns->client->sticky) {
			if (ds == ms->desk && dd != md->desk)
				window_hide(ns->client->window);
			else if (ds != ms->desk && dd == md->desk)
				window_show(ns->client->window);
		}
		if (ns->client->fullscreen && dd->focus != ns)
			set_fullscreen(ns, false);
	}

	history_transfer_node(md, dd, ns);
	stack(ns, STACK_BELOW);

	if (ds == dd) {
		if (focused)
			focus_node(md, dd, ns);
		else if (active)
			pseudo_focus(md, dd, ns);
	} else {
		if (focused)
			update_current();
		else if (ns == mon->desk->focus)
			update_input_focus();
	}

	arrange(ms, ds);
	if (ds != dd)
		arrange(md, dd);

	return true;
}

node_t *closest_node(monitor_t *m, desktop_t *d, node_t *n, cycle_dir_t dir, client_select_t sel)
{
	if (n == NULL)
		return NULL;

	node_t *f = (dir == CYCLE_PREV ? prev_leaf(n, d->root) : next_leaf(n, d->root));
	if (f == NULL)
		f = (dir == CYCLE_PREV ? second_extrema(d->root) : first_extrema(d->root));

	coordinates_t ref = {m, d, n};
	while (f != n) {
		coordinates_t loc = {m, d, f};
		if (node_matches(&loc, &ref, sel))
			return f;
		f = (dir == CYCLE_PREV ? prev_leaf(f, d->root) : next_leaf(f, d->root));
		if (f == NULL)
			f = (dir == CYCLE_PREV ? second_extrema(d->root) : first_extrema(d->root));
	}
	return NULL;
}

void circulate_leaves(monitor_t *m, desktop_t *d, circulate_dir_t dir)
{
	if (d == NULL || d->root == NULL || d->focus == NULL || is_leaf(d->root))
		return;
	node_t *p = d->focus->parent;
	bool focus_first_child = is_first_child(d->focus);
	if (dir == CIRCULATE_FORWARD)
		for (node_t *s = second_extrema(d->root), *f = prev_tiled_leaf(d, s, d->root); f != NULL; s = prev_tiled_leaf(d, f, d->root), f = prev_tiled_leaf(d, s, d->root))
			swap_nodes(m, d, f, m, d, s);
	else
		for (node_t *f = first_extrema(d->root), *s = next_tiled_leaf(d, f, d->root); s != NULL; f = next_tiled_leaf(d, s, d->root), s = next_tiled_leaf(d, f, d->root))
			swap_nodes(m, d, f, m, d, s);
	if (focus_first_child)
		focus_node(m, d, p->first_child);
	else
		focus_node(m, d, p->second_child);
}

void update_vacant_state(node_t *n)
{
	if (n == NULL)
		return;

	PUTS("update vacant state");

	/* n is not a leaf */
	node_t *p = n;

	while (p != NULL) {
		p->vacant = (p->first_child->vacant && p->second_child->vacant);
		p = p->parent;
	}
}

void update_privacy_level(node_t *n, bool value)
{
	int v = (value ? 1 : -1);
	for (node_t *p = n; p != NULL; p = p->parent)
		p->privacy_level += v;
}
