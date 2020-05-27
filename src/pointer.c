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

#include <xcb/xcb_keysyms.h>
#include <stdlib.h>
#include <stdbool.h>
#include "bspwm.h"
#include "query.h"
#include "settings.h"
#include "stack.h"
#include "tree.h"
#include "monitor.h"
#include "subscribe.h"
#include "events.h"
#include "window.h"
#include "pointer.h"

uint16_t num_lock;
uint16_t caps_lock;
uint16_t scroll_lock;

bool grabbing;
node_t *grabbed_node;

void pointer_init(void)
{
	num_lock = modfield_from_keysym(XK_Num_Lock);
	caps_lock = modfield_from_keysym(XK_Caps_Lock);
	scroll_lock = modfield_from_keysym(XK_Scroll_Lock);
	if (caps_lock == XCB_NO_SYMBOL) {
		caps_lock = XCB_MOD_MASK_LOCK;
	}
	grabbing = false;
	grabbed_node = NULL;
}

void window_grab_buttons(xcb_window_t win)
{
	for (unsigned int i = 0; i < LENGTH(BUTTONS); i++) {
		if (click_to_focus == (int8_t) XCB_BUTTON_INDEX_ANY || click_to_focus == (int8_t) BUTTONS[i]) {
			window_grab_button(win, BUTTONS[i], XCB_NONE);
		}
		if (pointer_actions[i] != ACTION_NONE) {
			window_grab_button(win, BUTTONS[i], pointer_modifier);
		}
	}
}

void window_grab_button(xcb_window_t win, uint8_t button, uint16_t modifier)
{
#define GRAB(b, m) \
	xcb_grab_button(dpy, false, win, XCB_EVENT_MASK_BUTTON_PRESS, \
	                XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, b, m)
		GRAB(button, modifier);
		if (num_lock != XCB_NO_SYMBOL && caps_lock != XCB_NO_SYMBOL && scroll_lock != XCB_NO_SYMBOL) {
			GRAB(button, modifier | num_lock | caps_lock | scroll_lock);
		}
		if (num_lock != XCB_NO_SYMBOL && caps_lock != XCB_NO_SYMBOL) {
			GRAB(button, modifier | num_lock | caps_lock);
		}
		if (caps_lock != XCB_NO_SYMBOL && scroll_lock != XCB_NO_SYMBOL) {
			GRAB(button, modifier | caps_lock | scroll_lock);
		}
		if (num_lock != XCB_NO_SYMBOL && scroll_lock != XCB_NO_SYMBOL) {
			GRAB(button, modifier | num_lock | scroll_lock);
		}
		if (num_lock != XCB_NO_SYMBOL) {
			GRAB(button, modifier | num_lock);
		}
		if (caps_lock != XCB_NO_SYMBOL) {
			GRAB(button, modifier | caps_lock);
		}
		if (scroll_lock != XCB_NO_SYMBOL) {
			GRAB(button, modifier | scroll_lock);
		}
#undef GRAB
}

void grab_buttons(void)
{
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root)) {
				window_grab_buttons(n->id);
				if (n->presel != NULL) {
					window_grab_buttons(n->presel->feedback);
				}
			}
		}
	}
}

void ungrab_buttons(void)
{
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root)) {
				xcb_ungrab_button(dpy, XCB_BUTTON_INDEX_ANY, n->id, XCB_MOD_MASK_ANY);
			}
		}
	}
}

int16_t modfield_from_keysym(xcb_keysym_t keysym)
{
	uint16_t modfield = 0;
	xcb_keycode_t *keycodes = NULL, *mod_keycodes = NULL;
	xcb_get_modifier_mapping_reply_t *reply = NULL;
	xcb_key_symbols_t *symbols = xcb_key_symbols_alloc(dpy);

	if ((keycodes = xcb_key_symbols_get_keycode(symbols, keysym)) == NULL ||
	    (reply = xcb_get_modifier_mapping_reply(dpy, xcb_get_modifier_mapping(dpy), NULL)) == NULL ||
	    reply->keycodes_per_modifier < 1 ||
	    (mod_keycodes = xcb_get_modifier_mapping_keycodes(reply)) == NULL) {
		goto end;
	}

	unsigned int num_mod = xcb_get_modifier_mapping_keycodes_length(reply) / reply->keycodes_per_modifier;
	for (unsigned int i = 0; i < num_mod; i++) {
		for (unsigned int j = 0; j < reply->keycodes_per_modifier; j++) {
			xcb_keycode_t mk = mod_keycodes[i * reply->keycodes_per_modifier + j];
			if (mk == XCB_NO_SYMBOL) {
				continue;
			}
			for (xcb_keycode_t *k = keycodes; *k != XCB_NO_SYMBOL; k++) {
				if (*k == mk) {
					modfield |= (1 << i);
				}
			}
		}
	}

end:
	xcb_key_symbols_free(symbols);
	free(keycodes);
	free(reply);
	return modfield;
}

resize_handle_t get_handle(node_t *n, xcb_point_t pos, pointer_action_t pac)
{
	resize_handle_t rh = HANDLE_BOTTOM_RIGHT;
	xcb_rectangle_t rect = get_rectangle(NULL, NULL, n);
	if (pac == ACTION_RESIZE_SIDE) {
		float W = rect.width;
		float H = rect.height;
		float ratio = W / H;
		float x = pos.x - rect.x;
		float y = pos.y - rect.y;
		float diag_a = ratio * y;
		float diag_b = W - diag_a;
		if (x < diag_a) {
			if (x < diag_b) {
				rh = HANDLE_LEFT;
			} else {
				rh = HANDLE_BOTTOM;
			}
		} else {
			if (x < diag_b) {
				rh = HANDLE_TOP;
			} else {
				rh = HANDLE_RIGHT;
			}
		}
	} else if (pac == ACTION_RESIZE_CORNER) {
		int16_t mid_x = rect.x + (rect.width / 2);
		int16_t mid_y = rect.y + (rect.height / 2);
		if (pos.x > mid_x) {
			if (pos.y > mid_y) {
				rh = HANDLE_BOTTOM_RIGHT;
			} else {
				rh = HANDLE_TOP_RIGHT;
			}
		} else {
			if (pos.y > mid_y) {
				rh = HANDLE_BOTTOM_LEFT;
			} else {
				rh = HANDLE_TOP_LEFT;
			}
		}
	}
	return rh;
}

bool grab_pointer(pointer_action_t pac)
{
	xcb_window_t win = XCB_NONE;
	xcb_point_t pos;

	query_pointer(&win, &pos);

	coordinates_t loc;

	if (!locate_window(win, &loc)) {
		if (pac == ACTION_FOCUS) {
			monitor_t *m = monitor_from_point(pos);
			if (m != NULL && m != mon && (win == XCB_NONE || win == m->root)) {
				focus_node(m, m->desk, m->desk->focus);
				return true;
			}
		}
		return false;
	}

	if (pac == ACTION_FOCUS) {
		if (loc.node != mon->desk->focus) {
			focus_node(loc.monitor, loc.desktop, loc.node);
			return true;
		} else if (focus_follows_pointer) {
			stack(loc.desktop, loc.node, true);
		}
		return false;
	}

	if (loc.node->client->state == STATE_FULLSCREEN) {
		return true;
	}

	xcb_grab_pointer_reply_t *reply = xcb_grab_pointer_reply(dpy, xcb_grab_pointer(dpy, 0, root, XCB_EVENT_MASK_BUTTON_RELEASE|XCB_EVENT_MASK_BUTTON_MOTION, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_CURRENT_TIME), NULL);

	if (reply == NULL || reply->status != XCB_GRAB_STATUS_SUCCESS) {
		free(reply);
		return true;
	}
	free(reply);

	if (pac == ACTION_MOVE) {
		put_status(SBSC_MASK_POINTER_ACTION, "pointer_action 0x%08X 0x%08X 0x%08X move begin\n", loc.monitor->id, loc.desktop->id, loc.node->id);
	} else if (pac == ACTION_RESIZE_CORNER) {
		put_status(SBSC_MASK_POINTER_ACTION, "pointer_action 0x%08X 0x%08X 0x%08X resize_corner begin\n", loc.monitor->id, loc.desktop->id, loc.node->id);
	} else if (pac == ACTION_RESIZE_SIDE) {
		put_status(SBSC_MASK_POINTER_ACTION, "pointer_action 0x%08X 0x%08X 0x%08X resize_side begin\n", loc.monitor->id, loc.desktop->id, loc.node->id);
	}

	track_pointer(loc, pac, pos);

	return true;
}

void track_pointer(coordinates_t loc, pointer_action_t pac, xcb_point_t pos)
{
	node_t *n = loc.node;
	resize_handle_t rh = get_handle(loc.node, pos, pac);

	uint16_t last_motion_x = pos.x, last_motion_y = pos.y;
	xcb_timestamp_t last_motion_time = 0;

	xcb_generic_event_t *evt = NULL;

	grabbing = true;
	grabbed_node = n;

	do {
		free(evt);
		while ((evt = xcb_wait_for_event(dpy)) == NULL) {
			xcb_flush(dpy);
		}
		uint8_t resp_type = XCB_EVENT_RESPONSE_TYPE(evt);
		if (resp_type == XCB_MOTION_NOTIFY) {
			xcb_motion_notify_event_t *e = (xcb_motion_notify_event_t*) evt;
			uint32_t dtime = e->time - last_motion_time;
			if (dtime < pointer_motion_interval) {
				continue;
			}
			last_motion_time = e->time;
			int16_t dx = e->root_x - last_motion_x;
			int16_t dy = e->root_y - last_motion_y;
			if (pac == ACTION_MOVE) {
				move_client(&loc, dx, dy);
			} else {
				if (honor_size_hints) {
					resize_client(&loc, rh, e->root_x, e->root_y, false);
				} else {
					resize_client(&loc, rh, dx, dy, true);
				}
			}
			last_motion_x = e->root_x;
			last_motion_y = e->root_y;
			xcb_flush(dpy);
		} else if (resp_type == XCB_BUTTON_RELEASE) {
			grabbing = false;
		} else {
			handle_event(evt);
		}
	} while (grabbing && grabbed_node != NULL);
	free(evt);

	xcb_ungrab_pointer(dpy, XCB_CURRENT_TIME);

	if (grabbed_node == NULL) {
		grabbing = false;
		return;
	}

	if (pac == ACTION_MOVE) {
		put_status(SBSC_MASK_POINTER_ACTION, "pointer_action 0x%08X 0x%08X 0x%08X move end\n", loc.monitor->id, loc.desktop->id, n->id);
	} else if (pac == ACTION_RESIZE_CORNER) {
		put_status(SBSC_MASK_POINTER_ACTION, "pointer_action 0x%08X 0x%08X 0x%08X resize_corner end\n", loc.monitor->id, loc.desktop->id, n->id);
	} else if (pac == ACTION_RESIZE_SIDE) {
		put_status(SBSC_MASK_POINTER_ACTION, "pointer_action 0x%08X 0x%08X 0x%08X resize_side end\n", loc.monitor->id, loc.desktop->id, n->id);
	}

	xcb_rectangle_t r = get_rectangle(NULL, NULL, n);

	put_status(SBSC_MASK_NODE_GEOMETRY, "node_geometry 0x%08X 0x%08X 0x%08X %ux%u+%i+%i\n", loc.monitor->id, loc.desktop->id, loc.node->id, r.width, r.height, r.x, r.y);

	if ((pac == ACTION_MOVE && IS_TILED(n->client)) ||
	    ((pac == ACTION_RESIZE_CORNER || pac == ACTION_RESIZE_SIDE) &&
	     n->client->state == STATE_TILED)) {
		for (node_t *f = first_extrema(loc.desktop->root); f != NULL; f = next_leaf(f, loc.desktop->root)) {
			if (f == n || f->client == NULL || !IS_TILED(f->client)) {
				continue;
			}
			xcb_rectangle_t r = f->client->tiled_rectangle;
			put_status(SBSC_MASK_NODE_GEOMETRY, "node_geometry 0x%08X 0x%08X 0x%08X %ux%u+%i+%i\n", loc.monitor->id, loc.desktop->id, f->id, r.width, r.height, r.x, r.y);
		}
	}
}
