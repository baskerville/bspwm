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

#include "bspwm.h"
#include "query.h"
#include "settings.h"
#include "stack.h"
#include "tree.h"
#include "monitor.h"
#include "window.h"
#include "pointer.h"

void grab_pointer(pointer_action_t pac)
{
	PRINTF("grab pointer %u\n", pac);

	xcb_window_t win = XCB_NONE;
	xcb_point_t pos;

	query_pointer(&win, &pos);

	coordinates_t loc;
	if (locate_window(win, &loc)) {
		client_t *c = NULL;
		frozen_pointer->position = pos;
		frozen_pointer->action = pac;
		c = loc.node->client;
		frozen_pointer->monitor = loc.monitor;
		frozen_pointer->desktop = loc.desktop;
		frozen_pointer->node = loc.node;
		frozen_pointer->client = c;
		frozen_pointer->window = c->window;
		frozen_pointer->horizontal_fence = NULL;
		frozen_pointer->vertical_fence = NULL;

		switch (pac)  {
			case ACTION_FOCUS:
				if (loc.node != mon->desk->focus) {
					bool backup = pointer_follows_monitor;
					pointer_follows_monitor = false;
					focus_node(loc.monitor, loc.desktop, loc.node);
					pointer_follows_monitor = backup;
				} else if (focus_follows_pointer) {
					stack(loc.node);
				}
				frozen_pointer->action = ACTION_NONE;
				break;
			case ACTION_MOVE:
			case ACTION_RESIZE_SIDE:
			case ACTION_RESIZE_CORNER:
				if (c->floating) {
					frozen_pointer->rectangle = c->floating_rectangle;
					frozen_pointer->is_tiled = false;
				} else if (!c->floating) {
					frozen_pointer->rectangle = c->tiled_rectangle;
					frozen_pointer->is_tiled = (pac == ACTION_MOVE || !c->pseudo_tiled);
				} else {
					frozen_pointer->action = ACTION_NONE;
					return;
				}
				if (pac == ACTION_RESIZE_SIDE) {
					float W = frozen_pointer->rectangle.width;
					float H = frozen_pointer->rectangle.height;
					float ratio = W / H;
					float x = pos.x - frozen_pointer->rectangle.x;
					float y = pos.y - frozen_pointer->rectangle.y;
					float diag_a = ratio * y;
					float diag_b = W - diag_a;
					if (x < diag_a) {
						if (x < diag_b)
							frozen_pointer->side = SIDE_LEFT;
						else
							frozen_pointer->side = SIDE_BOTTOM;
					} else {
						if (x < diag_b)
							frozen_pointer->side = SIDE_TOP;
						else
							frozen_pointer->side = SIDE_RIGHT;
					}
				} else if (pac == ACTION_RESIZE_CORNER) {
					int16_t mid_x = frozen_pointer->rectangle.x + (frozen_pointer->rectangle.width / 2);
					int16_t mid_y = frozen_pointer->rectangle.y + (frozen_pointer->rectangle.height / 2);
					if (pos.x > mid_x) {
						if (pos.y > mid_y)
							frozen_pointer->corner = CORNER_BOTTOM_RIGHT;
						else
							frozen_pointer->corner = CORNER_TOP_RIGHT;
					} else {
						if (pos.y > mid_y)
							frozen_pointer->corner = CORNER_BOTTOM_LEFT;
						else
							frozen_pointer->corner = CORNER_TOP_LEFT;
					}
				}
				if (frozen_pointer->is_tiled) {
					if (pac == ACTION_RESIZE_SIDE) {
						switch (frozen_pointer->side) {
							case SIDE_TOP:
								frozen_pointer->horizontal_fence = find_fence(loc.node, DIR_UP);
								break;
							case SIDE_RIGHT:
								frozen_pointer->vertical_fence = find_fence(loc.node, DIR_RIGHT);
								break;
							case SIDE_BOTTOM:
								frozen_pointer->horizontal_fence = find_fence(loc.node, DIR_DOWN);
								break;
							case SIDE_LEFT:
								frozen_pointer->vertical_fence = find_fence(loc.node, DIR_LEFT);
								break;
						}
					} else if (pac == ACTION_RESIZE_CORNER) {
						switch (frozen_pointer->corner) {
							case CORNER_TOP_LEFT:
								frozen_pointer->horizontal_fence = find_fence(loc.node, DIR_UP);
								frozen_pointer->vertical_fence = find_fence(loc.node, DIR_LEFT);
								break;
							case CORNER_TOP_RIGHT:
								frozen_pointer->horizontal_fence = find_fence(loc.node, DIR_UP);
								frozen_pointer->vertical_fence = find_fence(loc.node, DIR_RIGHT);
								break;
							case CORNER_BOTTOM_RIGHT:
								frozen_pointer->horizontal_fence = find_fence(loc.node, DIR_DOWN);
								frozen_pointer->vertical_fence = find_fence(loc.node, DIR_RIGHT);
								break;
							case CORNER_BOTTOM_LEFT:
								frozen_pointer->horizontal_fence = find_fence(loc.node, DIR_DOWN);
								frozen_pointer->vertical_fence = find_fence(loc.node, DIR_LEFT);
								break;
						}
					}
					if (frozen_pointer->horizontal_fence != NULL)
						frozen_pointer->horizontal_ratio = frozen_pointer->horizontal_fence->split_ratio;
					if (frozen_pointer->vertical_fence != NULL)
						frozen_pointer->vertical_ratio = frozen_pointer->vertical_fence->split_ratio;
				}
				break;
			case ACTION_NONE:
				break;
		}
	} else {
		if (pac == ACTION_FOCUS) {
			monitor_t *m = monitor_from_point(pos);
			if (m != NULL && m != mon)
				focus_node(m, m->desk, m->desk->focus);
		}
		frozen_pointer->action = ACTION_NONE;
	}
}

void track_pointer(int root_x, int root_y)
{
	if (frozen_pointer->action == ACTION_NONE)
		return;

	int delta_x, delta_y, x = 0, y = 0, w = 1, h = 1;

	pointer_action_t pac = frozen_pointer->action;
	monitor_t *m = frozen_pointer->monitor;
	desktop_t *d = frozen_pointer->desktop;
	node_t *n = frozen_pointer->node;
	client_t *c = frozen_pointer->client;
	xcb_window_t win = frozen_pointer->window;
	xcb_rectangle_t rect = frozen_pointer->rectangle;
	node_t *vertical_fence = frozen_pointer->vertical_fence;
	node_t *horizontal_fence = frozen_pointer->horizontal_fence;

	delta_x = root_x - frozen_pointer->position.x;
	delta_y = root_y - frozen_pointer->position.y;

	switch (pac) {
		case ACTION_MOVE:
			if (frozen_pointer->is_tiled) {
				xcb_window_t pwin = XCB_NONE;
				query_pointer(&pwin, NULL);
				if (pwin == win)
					return;
				coordinates_t loc;
				bool is_managed = (pwin == XCB_NONE ? false : locate_window(pwin, &loc));
				if (is_managed && !loc.node->client->floating && loc.monitor == m) {
					swap_nodes(m, d, n, m, d, loc.node);
					arrange(m, d);
				} else {
					if (is_managed && loc.monitor == m) {
						return;
					} else if (!is_managed) {
						xcb_point_t pt = (xcb_point_t) {root_x, root_y};
						monitor_t *pmon = monitor_from_point(pt);
						if (pmon == NULL || pmon == m) {
							return;
						} else {
							loc.monitor = pmon;
							loc.desktop = pmon->desk;
						}
					}
					bool focused = (n == mon->desk->focus);
					transfer_node(m, d, n, loc.monitor, loc.desktop, loc.desktop->focus);
					if (focused)
						focus_node(loc.monitor, loc.desktop, n);
					frozen_pointer->monitor = loc.monitor;
					frozen_pointer->desktop = loc.desktop;
				}
			} else {
				x = rect.x + delta_x;
				y = rect.y + delta_y;
				window_move(win, x, y);
				c->floating_rectangle.x = x;
				c->floating_rectangle.y = y;
				xcb_point_t pt = (xcb_point_t) {root_x, root_y};
				monitor_t *pmon = monitor_from_point(pt);
				if (pmon == NULL || pmon == m)
					return;
				bool focused = (n == mon->desk->focus);
				transfer_node(m, d, n, pmon, pmon->desk, pmon->desk->focus);
				if (focused)
					focus_node(pmon, pmon->desk, n);
				frozen_pointer->monitor = pmon;
				frozen_pointer->desktop = pmon->desk;
			}
			break;
		case ACTION_RESIZE_SIDE:
		case ACTION_RESIZE_CORNER:
			if (frozen_pointer->is_tiled) {
				if (vertical_fence != NULL) {
					double sr = frozen_pointer->vertical_ratio + (double) delta_x / vertical_fence->rectangle.width;
					sr = MAX(0, sr);
					sr = MIN(1, sr);
					vertical_fence->split_ratio = sr;
				}
				if (horizontal_fence != NULL) {
					double sr = frozen_pointer->horizontal_ratio + (double) delta_y / horizontal_fence->rectangle.height;
					sr = MAX(0, sr);
					sr = MIN(1, sr);
					horizontal_fence->split_ratio = sr;
				}
				arrange(m, d);
			} else {
				if (pac == ACTION_RESIZE_SIDE) {
					switch (frozen_pointer->side) {
						case SIDE_TOP:
							x = rect.x;
							y = rect.y + delta_y;
							w = rect.width;
							h = rect.height - delta_y;
							break;
						case SIDE_RIGHT:
							x = rect.x;
							y = rect.y;
							w = rect.width + delta_x;
							h = rect.height;
							break;
						case SIDE_BOTTOM:
							x = rect.x;
							y = rect.y;
							w = rect.width;
							h = rect.height + delta_y;
							break;
						case SIDE_LEFT:
							x = rect.x + delta_x;
							y = rect.y;
							w = rect.width - delta_x;
							h = rect.height;
							break;
					}
				} else if (pac == ACTION_RESIZE_CORNER) {
					switch (frozen_pointer->corner) {
						case CORNER_TOP_LEFT:
							x = rect.x + delta_x;
							y = rect.y + delta_y;
							w = rect.width - delta_x;
							h = rect.height - delta_y;
							break;
						case CORNER_TOP_RIGHT:
							x = rect.x;
							y = rect.y + delta_y;
							w = rect.width + delta_x;
							h = rect.height - delta_y;
							break;
						case CORNER_BOTTOM_LEFT:
							x = rect.x + delta_x;
							y = rect.y;
							w = rect.width - delta_x;
							h = rect.height + delta_y;
							break;
						case CORNER_BOTTOM_RIGHT:
							x = rect.x;
							y = rect.y;
							w = rect.width + delta_x;
							h = rect.height + delta_y;
							break;
					}
				}

				int oldw = w, oldh = h;
				restrain_floating_size(c, &w, &h);

				if (c->pseudo_tiled) {
					c->floating_rectangle.width = w;
					c->floating_rectangle.height = h;
					arrange(m, d);
				} else {
					if (oldw == w) {
						c->floating_rectangle.x = x;
						c->floating_rectangle.width = w;
					}
					if (oldh == h) {
						c->floating_rectangle.y = y;
						c->floating_rectangle.height = h;
					}
					window_move_resize(win, c->floating_rectangle.x,
					                        c->floating_rectangle.y,
					                        c->floating_rectangle.width,
					                        c->floating_rectangle.height);
				}
			}
			break;
		case ACTION_FOCUS:
		case ACTION_NONE:
			break;
	}
}

void ungrab_pointer(void)
{
	frozen_pointer->action = ACTION_NONE;
}
