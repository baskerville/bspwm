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
#include <float.h>
#include <limits.h>
#include "bspwm.h"
#include "desktop.h"
#include "ewmh.h"
#include "history.h"
#include "monitor.h"
#include "query.h"
#include "subscribe.h"
#include "settings.h"
#include "stack.h"
#include "window.h"
#include "tree.h"

void arrange(monitor_t *m, desktop_t *d)
{
	if (d->root == NULL) {
		return;
	}

	layout_t set_layout = d->layout;

	if (single_monocle && tiled_count(d->root) == 1) {
		d->layout = LAYOUT_MONOCLE;
	}

	xcb_rectangle_t rect = m->rectangle;
	int wg = (gapless_monocle && d->layout == LAYOUT_MONOCLE ? 0 : d->window_gap);
	rect.x += m->left_padding + d->left_padding + wg;
	rect.y += m->top_padding + d->top_padding + wg;
	rect.width -= m->left_padding + d->left_padding + d->right_padding + m->right_padding + wg;
	rect.height -= m->top_padding + d->top_padding + d->bottom_padding + m->bottom_padding + wg;

	apply_layout(m, d, d->root, rect, rect);

	d->layout = set_layout;
}

void apply_layout(monitor_t *m, desktop_t *d, node_t *n, xcb_rectangle_t rect, xcb_rectangle_t root_rect)
{
	if (n == NULL) {
		return;
	}

	n->rectangle = rect;

	if (pointer_follows_focus && mon->desk->focus == n && frozen_pointer->action == ACTION_NONE) {
		xcb_rectangle_t r = rect;
		r.width -= d->window_gap;
		r.height -= d->window_gap;
		center_pointer(r);
	}

	if (n->presel != NULL) {
		draw_presel_feedback(m, d, n);
	}

	if (is_leaf(n)) {

		unsigned int bw;
		if ((borderless_monocle && n->client->state == STATE_TILED && d->layout == LAYOUT_MONOCLE)
		    || n->client->state == STATE_FULLSCREEN) {
			bw = 0;
		} else {
			bw = n->client->border_width;
		}

		xcb_rectangle_t r;
		client_state_t s = n->client->state;
		if (s == STATE_TILED || s == STATE_PSEUDO_TILED) {
			int wg = (gapless_monocle && d->layout == LAYOUT_MONOCLE ? 0 : d->window_gap);
			/* tiled clients */
			if (s == STATE_TILED) {
				r = rect;
				int bleed = wg + 2 * bw;
				r.width = (bleed < r.width ? r.width - bleed : 1);
				r.height = (bleed < r.height ? r.height - bleed : 1);
			/* pseudo-tiled clients */
			} else {
				r = n->client->floating_rectangle;
				if (center_pseudo_tiled) {
					r.x = rect.x - bw + (rect.width - wg - r.width) / 2;
					r.y = rect.y - bw + (rect.height - wg - r.height) / 2;
				} else {
					r.x = rect.x;
					r.y = rect.y;
				}
			}
			n->client->tiled_rectangle = r;
		/* floating clients */
		} else if (s == STATE_FLOATING) {
			r = n->client->floating_rectangle;
		/* fullscreen clients */
		} else {
			r = m->rectangle;
		}

		window_move_resize(n->id, r.x, r.y, r.width, r.height);
		window_border_width(n->id, bw);

		if (frozen_pointer->action == ACTION_NONE) {
			put_status(SBSC_MASK_NODE_GEOMETRY, "node_geometry %s %s 0x%X %ux%u+%i+%i\n", m->name, d->name, n->id, r.width, r.height, r.x, r.y);
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

presel_t *make_presel(void)
{
	presel_t *p = malloc(sizeof(presel_t));
	p->split_dir = DIR_EAST;
	p->split_ratio = split_ratio;
	p->feedback = XCB_NONE;
	return p;
}

void presel_dir(monitor_t *m, desktop_t *d, node_t *n, direction_t dir)
{
	if (n->presel == NULL) {
		n->presel = make_presel();
	}

	n->presel->split_dir = dir;

	put_status(SBSC_MASK_NODE_PRESEL, "node_presel %s %s 0x%X dir %s\n", m->name, d->name, n->id, SPLIT_DIR_STR(dir));
}

void presel_ratio(monitor_t *m, desktop_t *d, node_t *n, double ratio)
{
	if (n->presel == NULL) {
		n->presel = make_presel();
	}

	n->presel->split_ratio = ratio;

	put_status(SBSC_MASK_NODE_PRESEL, "node_presel %s %s 0x%X ratio %lf\n", m->name, d->name, n->id, ratio);
}

void cancel_presel(monitor_t *m, desktop_t *d, node_t *n)
{
	if (n->presel == NULL) {
		return;
	}

	if (n->presel->feedback != XCB_NONE) {
		xcb_destroy_window(dpy, n->presel->feedback);
	}

	free(n->presel);
	n->presel = NULL;

	put_status(SBSC_MASK_NODE_PRESEL, "node_presel %s %s 0x%X cancel\n", m->name, d->name, n->id);
}

node_t *find_public(desktop_t *d)
{
	unsigned int b_manual_area = 0;
	unsigned int b_automatic_area = 0;
	node_t *b_manual = NULL;
	node_t *b_automatic = NULL;
	for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root)) {
		if (n->vacant) {
			continue;
		}
		unsigned int n_area = node_area(NULL, d, n);
		if (n_area > b_manual_area && (n->presel != NULL || !n->private)) {
			b_manual = n;
			b_manual_area = n_area;
		}
		if (n_area > b_automatic_area &&
		    n->presel == NULL && !n->private && private_count(n->parent) == 0) {
			b_automatic = n;
			b_automatic_area = n_area;
		}
	}
	if (b_automatic != NULL) {
		return b_automatic;
	} else {
		return b_manual;
	}
}

node_t *insert_node(monitor_t *m, desktop_t *d, node_t *n, node_t *f)
{
	if (d == NULL || n == NULL) {
		return NULL;
	}

	/* n: inserted node */
	/* c: new internal node */
	/* f: focus or insertion anchor */
	/* p: parent of focus */
	/* g: grand parent of focus */

	if (f == NULL) {
		f = d->root;
	}

	if (f == NULL) {
		d->root = n;
	} else {
		node_t *c = make_node(XCB_NONE);
		node_t *p = f->parent;
		if (f->presel == NULL && (f->private || private_count(f->parent) > 0)) {
			node_t *k = find_public(d);
			if (k != NULL) {
				f = k;
				p = f->parent;
			}
			if (f->presel == NULL && (f->private || private_count(f->parent) > 0)) {
				xcb_rectangle_t rect = get_rectangle(m, d, f);
				presel_dir(m, d, f, (rect.width >= rect.height ? DIR_EAST : DIR_SOUTH));
			}
		}
		if (f->presel == NULL && p != NULL && p->vacant) {
			f = p;
			p = f->parent;
		}
		n->parent = c;
		c->birth_rotation = f->birth_rotation;
		if (f->presel == NULL) {
			if (p == NULL) {
				if (initial_polarity == FIRST_CHILD) {
					c->first_child = n;
					c->second_child = f;
				} else {
					c->first_child = f;
					c->second_child = n;
				}
				if (m->rectangle.width > m->rectangle.height) {
					c->split_type = TYPE_VERTICAL;
				} else {
					c->split_type = TYPE_HORIZONTAL;
				}
				f->parent = c;
				d->root = c;
			} else {
				node_t *g = p->parent;
				c->parent = g;
				if (g != NULL) {
					if (is_first_child(p)) {
						g->first_child = c;
					} else {
						g->second_child = c;
					}
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
				n->birth_rotation = rot;
				if (!n->vacant) {
					rotate_tree(p, rot);
				}
			}
		} else {
			if (p != NULL) {
				if (is_first_child(f)) {
					p->first_child = c;
				} else {
					p->second_child = c;
				}
			}
			c->split_ratio = f->presel->split_ratio;
			c->parent = p;
			f->parent = c;
			f->birth_rotation = 0;
			switch (f->presel->split_dir) {
				case DIR_WEST:
					c->split_type = TYPE_VERTICAL;
					c->first_child = n;
					c->second_child = f;
					break;
				case DIR_EAST:
					c->split_type = TYPE_VERTICAL;
					c->first_child = f;
					c->second_child = n;
					break;
				case DIR_NORTH:
					c->split_type = TYPE_HORIZONTAL;
					c->first_child = n;
					c->second_child = f;
					break;
				case DIR_SOUTH:
					c->split_type = TYPE_HORIZONTAL;
					c->first_child = f;
					c->second_child = n;
					break;
			}
			if (d->root == f) {
				d->root = c;
			}
			cancel_presel(m, d, f);
		}
		if (f->vacant) {
			propagate_vacant_state(m, d, f);
		}
	}

	if (d->focus == NULL) {
		d->focus = n;
	}

	return f;
}

void activate_node(monitor_t *m, desktop_t *d, node_t *n)
{
	if (n == NULL && d->root != NULL) {
		n = history_last_node(d, NULL);
		if (n == NULL) {
			n = first_extrema(d->root);
		}
	}

	if (n != NULL) {
		if (d->focus != NULL && n != d->focus) {
			neutralize_occluding_windows(m, d, n);
		}
		stack(d, n, true);
		if (d->focus != n) {
			for (node_t *f = first_extrema(d->focus); f != NULL; f = next_leaf(f, d->focus)) {
				if (!is_descendant(f, n)) {
					window_draw_border(f->id, get_border_color(false, (m == mon)));
				}
			}
		}
		draw_border(n, true, (m == mon));
	}

	d->focus = n;

	put_status(SBSC_MASK_NODE_ACTIVATE, "node_activate %s %s 0x%X\n", m->name, d->name, n!=NULL?n->id:0);
	put_status(SBSC_MASK_REPORT);
}

void transfer_sticky_nodes(monitor_t *m, desktop_t *ds, desktop_t *dd, node_t *n)
{
	if (n == NULL) {
		return;
	} else if (n->sticky) {
		transfer_node(m, ds, n, m, dd, dd->focus);
	} else {
		/* we need references to the children because n might be freed after
		 * the first recursive call */
		node_t *first_child = n->first_child;
		node_t *second_child = n->second_child;
		transfer_sticky_nodes(m, ds, dd, first_child);
		transfer_sticky_nodes(m, ds, dd, second_child);
	}
}

void focus_node(monitor_t *m, desktop_t *d, node_t *n)
{
	if (n == NULL && d->root != NULL) {
		n = history_last_node(d, NULL);
		if (n == NULL) {
			n = first_extrema(d->root);
		}
	}

	if (mon->desk != d || n == NULL || n->client == NULL) {
		clear_input_focus();
	}

	if (m->sticky_count > 0 && d != m->desk) {
		sticky_still = false;
		transfer_sticky_nodes(m, m->desk, d, m->desk->root);
		sticky_still = true;
		if (n == NULL && d->focus != NULL) {
			n = d->focus;
		}
	}

	if (d->focus != NULL && n != d->focus) {
		neutralize_occluding_windows(m, d, n);
	}

	if (n != NULL && n->client != NULL && n->client->urgent) {
		set_urgent(m, d, n, false);
	}

	if (mon != m) {
		for (desktop_t *e = mon->desk_head; e != NULL; e = e->next) {
			draw_border(e->focus, true, false);
		}
		for (desktop_t *e = m->desk_head; e != NULL; e = e->next) {
			if (e == d) {
				continue;
			}
			draw_border(e->focus, true, true);
		}
	}

	if (d->focus != n) {
		for (node_t *f = first_extrema(d->focus); f != NULL; f = next_leaf(f, d->focus)) {
			if (!is_descendant(f, n)) {
				window_draw_border(f->id, get_border_color(false, true));
			}
		}
	}

	draw_border(n, true, true);

	focus_desktop(m, d);

	d->focus = n;
	ewmh_update_active_window();
	history_add(m, d, n);

	put_status(SBSC_MASK_REPORT);

	if (n == NULL) {
		return;
	}

	put_status(SBSC_MASK_NODE_FOCUS, "node_focus %s %s 0x%X\n", m->name, d->name, n->id);

	stack(d, n, true);
	set_input_focus(n);

	if (focus_follows_pointer) {
		xcb_window_t win = XCB_NONE;
		query_pointer(&win, NULL);
		if (win != n->id) {
			enable_motion_recorder();
		} else {
			disable_motion_recorder();
		}
	}

	if (pointer_follows_focus) {
		center_pointer(get_rectangle(m, d, n));
	}
}

void update_focused(void)
{
	focus_node(mon, mon->desk, mon->desk->focus);
}

void hide_node(node_t *n)
{
	if (n == NULL) {
		return;
	} else {
		if (n->presel != NULL) {
			window_hide(n->presel->feedback);
		}
		if (n->client != NULL) {
			window_hide(n->id);
		}
		hide_node(n->first_child);
		hide_node(n->second_child);
	}
}

void show_node(node_t *n)
{
	if (n == NULL) {
		return;
	} else {
		if (n->client != NULL) {
			window_show(n->id);
		}
		if (n->presel != NULL) {
			window_show(n->presel->feedback);
		}
		show_node(n->first_child);
		show_node(n->second_child);
	}
}

node_t *make_node(uint32_t id)
{
	if (id == XCB_NONE) {
		id = xcb_generate_id(dpy);
	}
	node_t *n = malloc(sizeof(node_t));
	n->id = id;
	n->parent = n->first_child = n->second_child = NULL;
	n->vacant = n->sticky = n->private = n->locked = false;
	n->split_ratio = split_ratio;
	n->split_type = TYPE_VERTICAL;
	n->birth_rotation = 0;
	n->presel = NULL;
	n->client = NULL;
	return n;
}

client_t *make_client(void)
{
	client_t *c = malloc(sizeof(client_t));
	c->state = c->last_state = STATE_TILED;
	c->layer = c->last_layer = LAYER_NORMAL;
	snprintf(c->class_name, sizeof(c->class_name), "%s", MISSING_VALUE);
	snprintf(c->instance_name, sizeof(c->instance_name), "%s", MISSING_VALUE);
	c->border_width = border_width;
	c->urgent = c->icccm_focus = false;
	c->icccm_input = true;
	c->wm_states_count = 0;
	return c;
}

void initialize_client(node_t *n)
{
	xcb_window_t win = n->id;
	if (win != XCB_NONE) {
		client_t *c = n->client;
		xcb_icccm_get_wm_protocols_reply_t protocols;
		if (xcb_icccm_get_wm_protocols_reply(dpy, xcb_icccm_get_wm_protocols(dpy, win, ewmh->WM_PROTOCOLS), &protocols, NULL) == 1) {
			if (has_proto(WM_TAKE_FOCUS, &protocols)) {
				c->icccm_focus = true;
			}
			xcb_icccm_get_wm_protocols_reply_wipe(&protocols);
		}
		xcb_ewmh_get_atoms_reply_t wm_state;
		if (xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, win), &wm_state, NULL) == 1) {
			for (unsigned int i = 0; i < wm_state.atoms_len && i < MAX_WM_STATES; i++) {
				ewmh_wm_state_add(n, wm_state.atoms[i]);
			}
			xcb_ewmh_get_atoms_reply_wipe(&wm_state);
		}
		xcb_icccm_wm_hints_t hints;
		if (xcb_icccm_get_wm_hints_reply(dpy, xcb_icccm_get_wm_hints(dpy, win), &hints, NULL) == 1
		    && (hints.flags & XCB_ICCCM_WM_HINT_INPUT)) {
			c->icccm_input = hints.input;
		}
	}
}

bool is_leaf(node_t *n)
{
	return (n != NULL && n->first_child == NULL && n->second_child == NULL);
}

bool is_first_child(node_t *n)
{
	return (n != NULL && n->parent != NULL && n->parent->first_child == n);
}

bool is_second_child(node_t *n)
{
	return (n != NULL && n->parent != NULL && n->parent->second_child == n);
}

unsigned int clients_count_in(node_t *n)
{
	if (n == NULL) {
		return 0;
	} else {
		return (n->client != NULL ? 1 : 0) +
		        clients_count_in(n->first_child) +
		        clients_count_in(n->second_child);
	}
}

node_t *brother_tree(node_t *n)
{
	if (n == NULL || n->parent == NULL) {
		return NULL;
	}
	if (is_first_child(n)) {
		return n->parent->second_child;
	} else {
		return n->parent->first_child;
	}
}

node_t *first_extrema(node_t *n)
{
	if (n == NULL) {
		return NULL;
	} else if (n->first_child == NULL) {
		return n;
	} else {
		return first_extrema(n->first_child);
	}
}

node_t *second_extrema(node_t *n)
{
	if (n == NULL) {
		return NULL;
	} else if (n->second_child == NULL) {
		return n;
	} else {
		return second_extrema(n->second_child);
	}
}

node_t *next_leaf(node_t *n, node_t *r)
{
	if (n == NULL) {
		return NULL;
	}
	node_t *p = n;
	while (is_second_child(p) && p != r) {
		p = p->parent;
	}
	if (p == r) {
		return NULL;
	}
	return first_extrema(p->parent->second_child);
}

node_t *prev_leaf(node_t *n, node_t *r)
{
	if (n == NULL) {
		return NULL;
	}
	node_t *p = n;
	while (is_first_child(p) && p != r) {
		p = p->parent;
	}
	if (p == r) {
		return NULL;
	}
	return second_extrema(p->parent->first_child);
}

node_t *next_tiled_leaf(desktop_t *d, node_t *n, node_t *r)
{
	node_t *next = next_leaf(n, r);
	if (next == NULL || IS_TILED(next->client)) {
		return next;
	} else {
		return next_tiled_leaf(d, next, r);
	}
}

node_t *prev_tiled_leaf(desktop_t *d, node_t *n, node_t *r)
{
	node_t *prev = prev_leaf(n, r);
	if (prev == NULL || IS_TILED(prev->client)) {
		return prev;
	} else {
		return prev_tiled_leaf(d, prev, r);
	}
}

/* Returns true if *b* is adjacent to *a* in the direction *dir* */
bool is_adjacent(node_t *a, node_t *b, direction_t dir)
{
	switch (dir) {
		case DIR_EAST:
			return (a->rectangle.x + a->rectangle.width) == b->rectangle.x;
			break;
		case DIR_SOUTH:
			return (a->rectangle.y + a->rectangle.height) == b->rectangle.y;
			break;
		case DIR_WEST:
			return (b->rectangle.x + b->rectangle.width) == a->rectangle.x;
			break;
		case DIR_NORTH:
			return (b->rectangle.y + b->rectangle.height) == a->rectangle.y;
			break;
	}
	return false;
}

node_t *find_fence(node_t *n, direction_t dir)
{
	node_t *p;

	if (n == NULL) {
		return NULL;
	}

	p = n->parent;

	while (p != NULL) {
		if ((dir == DIR_NORTH && p->split_type == TYPE_HORIZONTAL && p->rectangle.y < n->rectangle.y) ||
		    (dir == DIR_WEST && p->split_type == TYPE_VERTICAL && p->rectangle.x < n->rectangle.x) ||
		    (dir == DIR_SOUTH && p->split_type == TYPE_HORIZONTAL && (p->rectangle.y + p->rectangle.height) > (n->rectangle.y + n->rectangle.height)) ||
		    (dir == DIR_EAST && p->split_type == TYPE_VERTICAL && (p->rectangle.x + p->rectangle.width) > (n->rectangle.x + n->rectangle.width)))
			return p;
		p = p->parent;
	}

	return NULL;
}

node_t *nearest_neighbor(monitor_t *m, desktop_t *d, node_t *n, direction_t dir, node_select_t sel)
{
	if (n == NULL || (n->client != NULL && IS_FULLSCREEN(n->client)) ||
	    (d->layout == LAYOUT_MONOCLE && (n->client != NULL && IS_TILED(n->client)))) {
		return NULL;
	}

	node_t *nearest = NULL;
	if (history_aware_focus) {
		nearest = nearest_from_history(m, d, n, dir, sel);
	}
    if (nearest == NULL) {
        if (focus_by_distance) {
            nearest = nearest_from_distance(m, d, n, dir, sel);
        } else {
            nearest = nearest_from_tree(m, d, n, dir, sel);
        }
    }
    return nearest;
}

/* returns *true* if *a* is a child of *b* */
bool is_child(node_t *a, node_t *b)
{
	if (a == NULL || b == NULL) {
		return false;
	}
	return (a->parent != NULL && a->parent == b);
}

/* returns *true* if *a* is a descendant of *b* */
bool is_descendant(node_t *a, node_t *b)
{
	if (a == NULL || b == NULL) {
		return false;
	}
	while (a != b && a != NULL) {
		a = a->parent;
	}
	return a == b;
}

bool find_by_id(uint32_t id, coordinates_t *loc)
{
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			node_t *n = find_by_id_in(d->root, id);
			if (n != NULL) {
				loc->monitor = m;
				loc->desktop = d;
				loc->node = n;
				return true;
			}
		}
	}
	return false;
}

node_t *find_by_id_in(node_t *r, uint32_t id)
{
	if (r == NULL) {
		return NULL;
	} else if (r->id == id) {
		return r;
	} else {
		node_t *f = find_by_id_in(r->first_child, id);
		if (f != NULL) {
			return f;
		} else {
			return find_by_id_in(r->second_child, id);
		}
	}
}

node_t *nearest_from_tree(monitor_t *m, desktop_t *d, node_t *n, direction_t dir, node_select_t sel)
{
    if (n == NULL) {
        return NULL;
    }

    node_t *fence = find_fence(n, dir);

    if (fence == NULL) {
        return NULL;
    }

    node_t *nearest = NULL;

    if (dir == DIR_NORTH || dir == DIR_WEST) {
        nearest = second_extrema(fence->first_child);
    } else if (dir == DIR_SOUTH || dir == DIR_EAST) {
        nearest = first_extrema(fence->second_child);
    }

	coordinates_t ref = {m, d, n};
	coordinates_t loc = {m, d, nearest};

	if (node_matches(&loc, &ref, sel)) {
		return nearest;
	} else {
		return NULL;
	}
}

node_t *nearest_from_history(monitor_t *m, desktop_t *d, node_t *n, direction_t dir, node_select_t sel)
{
	if (n == NULL || (n->client != NULL && !IS_TILED(n->client))) {
		return NULL;
	}

	node_t *target = find_fence(n, dir);
	if (target == NULL) {
		return NULL;
	}
	if (dir == DIR_NORTH || dir == DIR_WEST) {
		target = target->first_child;
	} else if (dir == DIR_SOUTH || dir == DIR_EAST) {
		target = target->second_child;
	}

	node_t *nearest = NULL;
	int min_rank = INT_MAX;
	coordinates_t ref = {m, d, n};

	for (node_t *a = first_extrema(target); a != NULL; a = next_leaf(a, target)) {
		if (a->vacant || !is_adjacent(n, a, dir) || a == n) {
			continue;
		}
		coordinates_t loc = {m, d, a};
		if (!node_matches(&loc, &ref, sel)) {
			continue;
		}

		int rank = history_rank(d, a);
		if (rank >= 0 && rank < min_rank) {
			nearest = a;
			min_rank = rank;
		}
	}

	return nearest;
}

node_t *nearest_from_distance(monitor_t *m, desktop_t *d, node_t *n, direction_t dir, node_select_t sel)
{
	if (n == NULL) {
		return NULL;
	}

	node_t *target = NULL;

	if (n->client != NULL && IS_TILED(n->client)) {
		target = find_fence(n, dir);
		if (target == NULL) {
			return NULL;
		}
		if (dir == DIR_NORTH || dir == DIR_WEST) {
			target = target->first_child;
		} else if (dir == DIR_SOUTH || dir == DIR_EAST) {
			target = target->second_child;
		}
	} else {
		target = d->root;
	}

	node_t *nearest = NULL;
	direction_t dir2;
	xcb_point_t pt;
	xcb_point_t pt2;
	get_side_handle(m, d, n, dir, &pt);
	get_opposite(dir, &dir2);
	double ds = DBL_MAX;
	coordinates_t ref = {m, d, n};

	for (node_t *a = first_extrema(target); a != NULL; a = next_leaf(a, target)) {
		coordinates_t loc = {m, d, a};
		if (a == n ||
		    !node_matches(&loc, &ref, sel) ||
		    IS_TILED(a->client) != IS_TILED(n->client) ||
		    (IS_TILED(a->client) && !is_adjacent(n, a, dir))) {
			continue;
		}

		get_side_handle(m, d, a, dir2, &pt2);
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
		case DIR_EAST:
			*dst = DIR_WEST;
			break;
		case DIR_SOUTH:
			*dst = DIR_NORTH;
			break;
		case DIR_WEST:
			*dst = DIR_EAST;
			break;
		case DIR_NORTH:
			*dst = DIR_SOUTH;
			break;
	}
}

unsigned int node_area(monitor_t *m, desktop_t *d, node_t *n)
{
	if (n == NULL) {
		return 0;
	}
	xcb_rectangle_t rect = get_rectangle(m, d, n);
	return rect.width * rect.height;
}

int tiled_count(node_t *n)
{
	if (n == NULL) {
		return 0;
	}
	int cnt = 0;
	for (node_t *f = first_extrema(n); f != NULL; f = next_leaf(f, n)) {
		if (IS_TILED(f->client)) {
			cnt++;
		}
	}
	return cnt;
}

node_t *find_biggest(monitor_t *m, desktop_t *d, node_t *n, node_select_t sel)
{
	if (d == NULL) {
		return NULL;
	}

	node_t *b = NULL;
	unsigned int b_area = 0;
	coordinates_t ref = {m, d, n};

	for (node_t *f = first_extrema(d->root); f != NULL; f = next_leaf(f, d->root)) {
		coordinates_t loc = {m, d, f};
		if (f->vacant || !node_matches(&loc, &ref, sel)) {
			continue;
		}
		unsigned int f_area = node_area(m, d, f);
		if (f_area > b_area) {
			b = f;
			b_area = f_area;
		}
	}

	return b;
}

void rotate_tree(node_t *n, int deg)
{
	if (n == NULL || is_leaf(n) || deg == 0) {
		return;
	}

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
		if (n->split_type == TYPE_HORIZONTAL) {
			n->split_type = TYPE_VERTICAL;
		} else if (n->split_type == TYPE_VERTICAL) {
			n->split_type = TYPE_HORIZONTAL;
		}
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
	if (rot == 0) {
		return;
	}
	rotate_tree(n, 360 - rot);
}

void unrotate_brother(node_t *n)
{
	unrotate_tree(brother_tree(n), n->birth_rotation);
}

void flip_tree(node_t *n, flip_t flp)
{
	if (n == NULL || is_leaf(n)) {
		return;
	}

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
		if (b1 > 0 && b2 > 0) {
			n->split_ratio = (double) b1 / b;
		}
		return b;
	}
}

void unlink_node(monitor_t *m, desktop_t *d, node_t *n)
{
	if (d == NULL || n == NULL) {
		return;
	}

	node_t *p = n->parent;

	if (p == NULL) {
		d->root = NULL;
		d->focus = NULL;
	} else {
		if (d->focus == p || is_descendant(d->focus, n)) {
			d->focus = NULL;
		}

		node_t *b = brother_tree(n);
		node_t *g = p->parent;

		if (!n->vacant) {
			unrotate_tree(b, n->birth_rotation);
		}

		b->parent = g;

		if (g != NULL) {
			if (is_first_child(p)) {
				g->first_child = b;
			} else {
				g->second_child = b;
			}
		} else {
			d->root = b;
		}

		b->birth_rotation = p->birth_rotation;
		n->parent = NULL;

		history_remove(d, p, false);
		cancel_presel(m, d, p);
		if (p->sticky) {
			m->sticky_count--;
		}
		free(p);

		propagate_vacant_state(m, d, b);
	}
}

void remove_node(monitor_t *m, desktop_t *d, node_t *n)
{
	if (n == NULL) {
		return;
	}

	node_t *last_focus = d->focus;

	unlink_node(m, d, n);
	history_remove(d, n, true);
	remove_stack_node(n);
	cancel_presel(m, d, n);
	if (m->sticky_count > 0) {
		m->sticky_count -= sticky_count(n);
	}
	clients_count -= clients_count_in(n);
	free(n->client);
	free(n);

	ewmh_update_client_list(false);

	if (d->focus != last_focus) {
		if (d == mon->desk) {
			focus_node(m, d, d->focus);
		} else {
			activate_node(m, d, d->focus);
		}
	}
}

void destroy_tree(monitor_t *m, desktop_t *d, node_t *n)
{
	if (n == NULL) {
		return;
	}
	node_t *first_child = n->first_child;
	node_t *second_child = n->second_child;
	if (n->client != NULL) {
		remove_stack_node(n);
		clients_count--;
	}
	if (n->sticky) {
		m->sticky_count--;
	}
	cancel_presel(m, d, n);
	free(n->client);
	free(n);
	if (first_child != NULL && second_child != NULL) {
		first_child->parent = second_child->parent = NULL;
		destroy_tree(m, d, first_child);
		destroy_tree(m, d, second_child);
	}
}

bool swap_nodes(monitor_t *m1, desktop_t *d1, node_t *n1, monitor_t *m2, desktop_t *d2, node_t *n2)
{
	if (n1 == NULL || n2 == NULL || n1 == n2 || is_descendant(n1, n2) || is_descendant(n2, n1) ||
	    (d1 != d2 && ((m1->sticky_count > 0 && sticky_count(n1) > 0) ||
	                  (m2->sticky_count > 0 && sticky_count(n2) > 0)))) {
		return false;
	}

	put_status(SBSC_MASK_NODE_SWAP, "node_swap %s %s 0x%X %s %s 0x%X\n", m1->name, d1->name, n1->id, m2->name, d2->name, n2->id);

	node_t *pn1 = n1->parent;
	node_t *pn2 = n2->parent;
	bool n1_first_child = is_first_child(n1);
	bool n2_first_child = is_first_child(n2);
	int br1 = n1->birth_rotation;
	int br2 = n2->birth_rotation;
	bool n1_held_focus = is_descendant(d1->focus, n1);
	bool n2_held_focus = is_descendant(d2->focus, n2);
	node_t *last_d1_focus = d1->focus;
	node_t *last_d2_focus = d2->focus;

	if (pn1 != NULL) {
		if (n1_first_child) {
			pn1->first_child = n2;
		} else {
			pn1->second_child = n2;
		}
	}

	if (pn2 != NULL) {
		if (n2_first_child) {
			pn2->first_child = n1;
		} else {
			pn2->second_child = n1;
		}
	}

	n1->parent = pn2;
	n2->parent = pn1;
	n1->birth_rotation = br2;
	n2->birth_rotation = br1;

	if (n1->vacant != n2->vacant) {
		propagate_vacant_state(m2, d2, n1);
		propagate_vacant_state(m1, d1, n2);
	}

	if (d1 != d2) {
		if (d1->root == n1) {
			d1->root = n2;
		}

		if (d2->root == n2) {
			d2->root = n1;
		}

		if (n1_held_focus) {
			d1->focus = n2_held_focus ? last_d2_focus : n2;
		}

		if (n2_held_focus) {
			d2->focus = n1_held_focus ? last_d1_focus : n1;
		}

		if (m1 != m2) {
			adapt_geometry(&m2->rectangle, &m1->rectangle, n2);
			adapt_geometry(&m1->rectangle, &m2->rectangle, n1);
		}

		ewmh_set_wm_desktop(n1, d2);
		ewmh_set_wm_desktop(n2, d1);

		history_swap_nodes(m1, d1, n1, m2, d2, n2);

		if (m1->desk != d1 && m2->desk == d2) {
			show_node(n1);
			hide_node(n2);
		} else if (m1->desk == d1 && m2->desk != d2) {
			hide_node(n1);
			show_node(n2);
		}

		if (n1_held_focus) {
			if (d1 == mon->desk) {
				focus_node(m1, d1, d1->focus);
			} else {
				activate_node(m1, d1, d1->focus);
			}
		} else {
			draw_border(n2, is_descendant(n2, d1->focus), (m1 == mon));
		}

		if (n2_held_focus) {
			if (d2 == mon->desk) {
				focus_node(m2, d2, d2->focus);
			} else {
				activate_node(m2, d2, d2->focus);
			}
		} else {
			draw_border(n1, is_descendant(n1, d2->focus), (m2 == mon));
		}
	}

	arrange(m1, d1);

	if (d1 != d2) {
		arrange(m2, d2);
	}

	return true;
}

bool transfer_node(monitor_t *ms, desktop_t *ds, node_t *ns, monitor_t *md, desktop_t *dd, node_t *nd)
{
	if (ns == NULL || ns == nd || is_child(ns, nd) || is_descendant(nd, ns)) {
		return false;
	}

	unsigned int sc = 0;

	if (sticky_still && ms->sticky_count > 0 && ds != dd && (sc = sticky_count(ns)) > 0) {
		return false;
	}

	put_status(SBSC_MASK_NODE_TRANSFER, "node_transfer %s %s 0x%X %s %s 0x%X\n", ms->name, ds->name, ns->id, md->name, dd->name, nd!=NULL?nd->id:0);

	bool held_focus = is_descendant(ds->focus, ns);
	/* avoid ending up with a dangling pointer (because of unlink_node) */
	node_t *last_ds_focus = is_child(ns, ds->focus) ? NULL : ds->focus;

	if (held_focus && ds == mon->desk) {
		clear_input_focus();
	}

	unlink_node(ms, ds, ns);
	insert_node(md, dd, ns, nd);

	if (md != ms) {
		adapt_geometry(&ms->rectangle, &md->rectangle, ns);
	}

	if (ds != dd) {
		ewmh_set_wm_desktop(ns, dd);
		if (sc == 0) {
			if (ds == ms->desk && dd != md->desk) {
				hide_node(ns);
			} else if (ds != ms->desk && dd == md->desk) {
				show_node(ns);
			}
		}
	}

	history_transfer_node(md, dd, ns);
	stack(dd, ns, false);

	if (ds == dd) {
		if (held_focus) {
			if (ds == mon->desk) {
				focus_node(ms, ds, last_ds_focus);
			} else {
				activate_node(ms, ds, last_ds_focus);
			}
		} else {
			draw_border(ns, is_descendant(ns, ds->focus), (ms == mon));
		}
	} else {
		if (held_focus) {
			if (ds == mon->desk) {
				focus_node(ms, ds, ds->focus);
			} else {
				activate_node(ms, ds, ds->focus);
			}
		}
		if (dd->focus == ns) {
			if (dd == mon->desk) {
				focus_node(md, dd, held_focus ? last_ds_focus : dd->focus);
			} else {
				activate_node(md, dd, held_focus ? last_ds_focus : dd->focus);
			}
		} else {
			draw_border(ns, is_descendant(ns, dd->focus), (md == mon));
		}
	}

	arrange(ms, ds);

	if (ds != dd) {
		arrange(md, dd);
	}

	return true;
}

node_t *closest_node(monitor_t *m, desktop_t *d, node_t *n, cycle_dir_t dir, node_select_t sel)
{
	if (n == NULL) {
		return NULL;
	}

	node_t *f = (dir == CYCLE_PREV ? prev_leaf(n, d->root) : next_leaf(n, d->root));
	if (f == NULL) {
		f = (dir == CYCLE_PREV ? second_extrema(d->root) : first_extrema(d->root));
	}

	coordinates_t ref = {m, d, n};
	while (f != n) {
		coordinates_t loc = {m, d, f};
		if (node_matches(&loc, &ref, sel)) {
			return f;
		}
		f = (dir == CYCLE_PREV ? prev_leaf(f, d->root) : next_leaf(f, d->root));
		if (f == NULL) {
			f = (dir == CYCLE_PREV ? second_extrema(d->root) : first_extrema(d->root));
		}
	}
	return NULL;
}

void circulate_leaves(monitor_t *m, desktop_t *d, node_t *n, circulate_dir_t dir)
{
	if (d == NULL || d->root == NULL || d->focus == NULL || is_leaf(n)) {
		return;
	}
	node_t *p = d->focus->parent;
	bool focus_first_child = is_first_child(d->focus);
	if (dir == CIRCULATE_FORWARD) {
		for (node_t *s = second_extrema(n), *f = prev_tiled_leaf(d, s, n); f != NULL; s = prev_tiled_leaf(d, f, n), f = prev_tiled_leaf(d, s, n)) {
			swap_nodes(m, d, f, m, d, s);
		}
	} else {
		for (node_t *f = first_extrema(n), *s = next_tiled_leaf(d, f, n); s != NULL; f = next_tiled_leaf(d, s, n), s = next_tiled_leaf(d, f, n)) {
			swap_nodes(m, d, f, m, d, s);
		}
	}
	if (focus_first_child) {
		focus_node(m, d, p->first_child);
	} else {
		focus_node(m, d, p->second_child);
	}
}

void set_vacant_state(monitor_t *m, desktop_t *d, node_t *n, bool value)
{
	n->vacant = value;

	if (value) {
		unrotate_brother(n);
	} else {
		rotate_brother(n);
	}

	if (value) {
		cancel_presel(m, d, n);
	}

	propagate_vacant_state(m, d, n);
}

void propagate_vacant_state(monitor_t *m, desktop_t *d, node_t *n)
{
	if (n == NULL) {
		return;
	}

	node_t *p = n->parent;

	while (p != NULL) {
		p->vacant = (p->first_child->vacant && p->second_child->vacant);
		if (p->vacant) {
			cancel_presel(m, d, p);
		}
		p = p->parent;
	}
}

void set_layer(monitor_t *m, desktop_t *d, node_t *n, stack_layer_t l)
{
	if (n == NULL || n->client->layer == l) {
		return;
	}

	n->client->last_layer = n->client->layer;
	n->client->layer = l;

	put_status(SBSC_MASK_NODE_LAYER, "node_layer %s %s 0x%X %s\n", m->name, d->name, n->id, LAYER_STR(l));

	if (d->focus == n) {
		neutralize_occluding_windows(m, d, n);
	}

	stack(d, n, (d->focus == n));
}

void set_state(monitor_t *m, desktop_t *d, node_t *n, client_state_t s)
{
	if (n == NULL || n->client->state == s) {
		return;
	}

	client_t *c = n->client;

	c->last_state = c->state;
	c->state = s;

	switch (c->last_state) {
		case STATE_TILED:
		case STATE_PSEUDO_TILED:
			break;
		case STATE_FLOATING:
			set_floating(m, d, n, false);
			break;
		case STATE_FULLSCREEN:
			set_fullscreen(m, d, n, false);
			break;
	}

	put_status(SBSC_MASK_NODE_STATE, "node_state %s %s 0x%X %s off\n", m->name, d->name, n->id, STATE_STR(c->last_state));

	switch (c->state) {
		case STATE_TILED:
		case STATE_PSEUDO_TILED:
			break;
		case STATE_FLOATING:
			set_floating(m, d, n, true);
			break;
		case STATE_FULLSCREEN:
			set_fullscreen(m, d, n, true);
			break;
	}

	put_status(SBSC_MASK_NODE_STATE, "node_state %s %s 0x%X %s on\n", m->name, d->name, n->id, STATE_STR(c->state));

	if (n == m->desk->focus) {
		put_status(SBSC_MASK_REPORT);
	}
}

void set_floating(monitor_t *m, desktop_t *d, node_t *n, bool value)
{
	if (n == NULL) {
		return;
	}

	cancel_presel(m, d, n);
	set_vacant_state(m, d, n, value);

	if (!value && d->focus == n) {
		neutralize_occluding_windows(m, d, n);
	}

	stack(d, n, (d->focus == n));
}

void set_fullscreen(monitor_t *m, desktop_t *d, node_t *n, bool value)
{
	if (n == NULL) {
		return;
	}

	client_t *c = n->client;

	cancel_presel(m, d, n);
	set_vacant_state(m, d, n, value);

	if (value) {
		ewmh_wm_state_add(n, ewmh->_NET_WM_STATE_FULLSCREEN);
		c->last_layer = c->layer;
		c->layer = LAYER_ABOVE;
	} else {
		ewmh_wm_state_remove(n, ewmh->_NET_WM_STATE_FULLSCREEN);
		c->layer = c->last_layer;
		if (d->focus == n) {
			neutralize_occluding_windows(m, d, n);
		}
	}

	stack(d, n, (d->focus == n));
}

void neutralize_occluding_windows(monitor_t *m, desktop_t *d, node_t *n)
{
	bool dirty = false;
	for (node_t *f = first_extrema(n); f != NULL; f = next_leaf(f, n)) {
		for (node_t *a = first_extrema(d->root); a != NULL; a = next_leaf(a, d->root)) {
			if (a != f && IS_FULLSCREEN(a->client) && stack_cmp(f->client, a->client) < 0) {
				set_state(m, d, a, a->client->last_state);
				dirty = true;
			}
		}
	}
	if (dirty) {
		arrange(m, d);
	}
}

void set_locked(monitor_t *m, desktop_t *d, node_t *n, bool value)
{
	if (n == NULL || n->locked == value) {
		return;
	}

	n->locked = value;

	put_status(SBSC_MASK_NODE_FLAG, "node_flag %s %s 0x%X locked %s\n", m->name, d->name, n->id, ON_OFF_STR(value));

	if (n == m->desk->focus) {
		put_status(SBSC_MASK_REPORT);
	}
}

void set_sticky(monitor_t *m, desktop_t *d, node_t *n, bool value)
{
	if (n == NULL || n->sticky == value) {
		return;
	}

	if (d != m->desk) {
		transfer_node(m, d, n, m, m->desk, m->desk->focus);
	}

	n->sticky = value;

	if (value) {
		ewmh_wm_state_add(n, ewmh->_NET_WM_STATE_STICKY);
		m->sticky_count++;
	} else {
		ewmh_wm_state_remove(n, ewmh->_NET_WM_STATE_STICKY);
		m->sticky_count--;
	}

	put_status(SBSC_MASK_NODE_FLAG, "node_flag %s %s 0x%X sticky %s\n", m->name, d->name, n->id, ON_OFF_STR(value));

	if (n == m->desk->focus) {
		put_status(SBSC_MASK_REPORT);
	}

}

void set_private(monitor_t *m, desktop_t *d, node_t *n, bool value)
{
	if (n == NULL || n->private == value) {
		return;
	}

	n->private = value;

	put_status(SBSC_MASK_NODE_FLAG, "node_flag %s %s 0x%X private %s\n", m->name, d->name, n->id, ON_OFF_STR(value));

	if (n == m->desk->focus) {
		put_status(SBSC_MASK_REPORT);
	}
}

void set_urgent(monitor_t *m, desktop_t *d, node_t *n, bool value)
{
	if (value && mon->desk->focus == n) {
		return;
	}

	n->client->urgent = value;

	put_status(SBSC_MASK_NODE_FLAG, "node_flag %s %s 0x%X urgent %s\n", m->name, d->name, n->id, ON_OFF_STR(value));
	put_status(SBSC_MASK_REPORT);
}

/* Returns true if a contains b */
bool contains(xcb_rectangle_t a, xcb_rectangle_t b)
{
	return (a.x <= b.x && (a.x + a.width) >= (b.x + b.width) &&
	        a.y <= b.y && (a.y + a.height) >= (b.y + b.height));
}

xcb_rectangle_t get_rectangle(monitor_t *m, desktop_t *d, node_t *n)
{
	client_t *c = n->client;
	if (c != NULL) {
		switch (c->state) {
			case STATE_TILED:
			case STATE_PSEUDO_TILED:
				return c->tiled_rectangle;
				break;
			case STATE_FLOATING:
				return c->floating_rectangle;
				break;
			case STATE_FULLSCREEN:
			default:
				return m != NULL ? m->rectangle : (xcb_rectangle_t) {0, 0, screen_width, screen_height};
				break;
		 }
	} else {
		int wg = (gapless_monocle && d->layout == LAYOUT_MONOCLE ? 0 : d->window_gap);
		xcb_rectangle_t rect = n->rectangle;
		rect.width -= wg;
		rect.height -= wg;
		return rect;
	}
}

void get_side_handle(monitor_t *m, desktop_t *d, node_t *n, direction_t dir, xcb_point_t *pt)
{
	xcb_rectangle_t rect = get_rectangle(m, d, n);

	switch (dir) {
		case DIR_EAST:
			pt->x = rect.x + rect.width;
			pt->y = rect.y + (rect.height / 2);
			break;
		case DIR_SOUTH:
			pt->x = rect.x + (rect.width / 2);
			pt->y = rect.y + rect.height;
			break;
		case DIR_WEST:
			pt->x = rect.x;
			pt->y = rect.y + (rect.height / 2);
			break;
		case DIR_NORTH:
			pt->x = rect.x + (rect.width / 2);
			pt->y = rect.y;
			break;
	}
}

#define DEF_FLAG_COUNT(flag) \
	unsigned int flag##_count(node_t *n) \
	{ \
		if (n == NULL) { \
			return 0; \
		} else { \
			return ((n->flag ? 1 : 0) + \
			        flag##_count(n->first_child) + \
			        flag##_count(n->second_child)); \
		} \
	}
	DEF_FLAG_COUNT(sticky)
	DEF_FLAG_COUNT(private)
	DEF_FLAG_COUNT(locked)
#undef DEF_FLAG_COUNT
