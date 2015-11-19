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

#include <stdlib.h>
#include "bspwm.h"
#include "ewmh.h"
#include "monitor.h"
#include "query.h"
#include "settings.h"
#include "subscribe.h"
#include "tree.h"
#include "window.h"
#include "events.h"
#include "json.h"

void handle_event(xcb_generic_event_t *evt)
{
	uint8_t resp_type = XCB_EVENT_RESPONSE_TYPE(evt);
	switch (resp_type) {
		case XCB_MAP_REQUEST:
			map_request(evt);
			break;
		case XCB_DESTROY_NOTIFY:
			destroy_notify(evt);
			break;
		case XCB_UNMAP_NOTIFY:
			unmap_notify(evt);
			break;
		case XCB_CLIENT_MESSAGE:
			client_message(evt);
			break;
		case XCB_CONFIGURE_REQUEST:
			configure_request(evt);
			break;
		case XCB_PROPERTY_NOTIFY:
			property_notify(evt);
			break;
		case XCB_ENTER_NOTIFY:
			enter_notify(evt);
			break;
		case XCB_MOTION_NOTIFY:
			motion_notify(evt);
			break;
		case XCB_FOCUS_IN:
			focus_in(evt);
			break;
		case 0:
			process_error(evt);
			break;
		default:
			if (randr && resp_type == randr_base + XCB_RANDR_SCREEN_CHANGE_NOTIFY)
				update_monitors();
			break;
	}
}

void map_request(xcb_generic_event_t *evt)
{
	xcb_map_request_event_t *e = (xcb_map_request_event_t *) evt;

	schedule_window(e->window);
}

void configure_request(xcb_generic_event_t *evt)
{
	xcb_configure_request_event_t *e = (xcb_configure_request_event_t *) evt;

	coordinates_t loc;
	bool is_managed = locate_window(e->window, &loc);
	client_t *c = (is_managed ? loc.node->client : NULL);
	int w = 0, h = 0;

	if (is_managed && !IS_FLOATING(c)) {
		if (e->value_mask & XCB_CONFIG_WINDOW_X)
			c->floating_rectangle.x = e->x;
		if (e->value_mask & XCB_CONFIG_WINDOW_Y)
			c->floating_rectangle.y = e->y;
		if (e->value_mask & XCB_CONFIG_WINDOW_WIDTH)
			w = e->width;
		if (e->value_mask & XCB_CONFIG_WINDOW_HEIGHT)
			h = e->height;

		if (w != 0) {
			restrain_floating_width(c, &w);
			c->floating_rectangle.width = w;
		}

		if (h != 0) {
			restrain_floating_height(c, &h);
			c->floating_rectangle.height = h;
		}

		xcb_configure_notify_event_t evt;
		xcb_window_t win = c->window;
		unsigned int bw = c->border_width;

		xcb_rectangle_t rect = get_rectangle(loc.monitor, c);

		evt.response_type = XCB_CONFIGURE_NOTIFY;
		evt.event = win;
		evt.window = win;
		evt.above_sibling = XCB_NONE;
		evt.x = rect.x;
		evt.y = rect.y;
		evt.width = rect.width;
		evt.height = rect.height;
		evt.border_width = bw;
		evt.override_redirect = false;

		xcb_send_event(dpy, false, win, XCB_EVENT_MASK_STRUCTURE_NOTIFY, (const char *) &evt);

		if (c->state == STATE_PSEUDO_TILED) {
			arrange(loc.monitor, loc.desktop);
		}
	} else {
		uint16_t mask = 0;
		uint32_t values[7];
		unsigned short i = 0;

		if (e->value_mask & XCB_CONFIG_WINDOW_X) {
			mask |= XCB_CONFIG_WINDOW_X;
			values[i++] = e->x;
			if (is_managed)
				c->floating_rectangle.x = e->x;
		}

		if (e->value_mask & XCB_CONFIG_WINDOW_Y) {
			mask |= XCB_CONFIG_WINDOW_Y;
			values[i++] = e->y;
			if (is_managed)
				c->floating_rectangle.y = e->y;
		}

		if (e->value_mask & XCB_CONFIG_WINDOW_WIDTH) {
			mask |= XCB_CONFIG_WINDOW_WIDTH;
			w = e->width;
			if (is_managed) {
				restrain_floating_width(c, &w);
				c->floating_rectangle.width = w;
			}
			values[i++] = w;
		}

		if (e->value_mask & XCB_CONFIG_WINDOW_HEIGHT) {
			mask |= XCB_CONFIG_WINDOW_HEIGHT;
			h = e->height;
			if (is_managed) {
				restrain_floating_height(c, &h);
				c->floating_rectangle.height = h;
			}
			values[i++] = h;
		}

		if (!is_managed && e->value_mask & XCB_CONFIG_WINDOW_BORDER_WIDTH) {
			mask |= XCB_CONFIG_WINDOW_BORDER_WIDTH;
			values[i++] = e->border_width;
		}

		if (e->value_mask & XCB_CONFIG_WINDOW_SIBLING) {
			mask |= XCB_CONFIG_WINDOW_SIBLING;
			values[i++] = e->sibling;
		}

		if (e->value_mask & XCB_CONFIG_WINDOW_STACK_MODE) {
			mask |= XCB_CONFIG_WINDOW_STACK_MODE;
			values[i++] = e->stack_mode;
		}

		xcb_configure_window(dpy, e->window, mask, values);

		if (is_managed && (mask & (XCB_CONFIG_WINDOW_X_Y_WIDTH_HEIGHT))) {
			if (exists_subscriber(SBSC_MASK_WINDOW_GEOMETRY))
				put_status(SBSC_MASK_WINDOW_GEOMETRY, json_serialize_status_node(loc.monitor, loc.desktop, loc.node));
		}
	}

	if (is_managed) {
		translate_client(monitor_from_client(c), loc.monitor, c);
	}
}

void destroy_notify(xcb_generic_event_t *evt)
{
	xcb_destroy_notify_event_t *e = (xcb_destroy_notify_event_t *) evt;

	unmanage_window(e->window);
}

void unmap_notify(xcb_generic_event_t *evt)
{
	xcb_unmap_notify_event_t *e = (xcb_unmap_notify_event_t *) evt;

	unmanage_window(e->window);
}

void property_notify(xcb_generic_event_t *evt)
{
	xcb_property_notify_event_t *e = (xcb_property_notify_event_t *) evt;

	if (e->atom != XCB_ATOM_WM_HINTS && e->atom != XCB_ATOM_WM_NORMAL_HINTS) {
		return;
	}

	coordinates_t loc;
	if (!locate_window(e->window, &loc))
			return;

	if (e->atom == XCB_ATOM_WM_HINTS) {
		xcb_icccm_wm_hints_t hints;
		if (xcb_icccm_get_wm_hints_reply(dpy, xcb_icccm_get_wm_hints(dpy, e->window), &hints, NULL) == 1 &&
		    (hints.flags & XCB_ICCCM_WM_HINT_X_URGENCY))
			set_urgency(loc.monitor, loc.desktop, loc.node, xcb_icccm_wm_hints_get_urgency(&hints));
	} else if (e->atom == XCB_ATOM_WM_NORMAL_HINTS) {
		client_t *c = loc.node->client;
		xcb_size_hints_t size_hints;
		if (xcb_icccm_get_wm_normal_hints_reply(dpy, xcb_icccm_get_wm_normal_hints(dpy, e->window), &size_hints, NULL) == 1 &&
		    (size_hints.flags & (XCB_ICCCM_SIZE_HINT_P_MIN_SIZE | XCB_ICCCM_SIZE_HINT_P_MAX_SIZE))) {
			c->min_width = size_hints.min_width;
			c->max_width = size_hints.max_width;
			c->min_height = size_hints.min_height;
			c->max_height = size_hints.max_height;
			int w = c->floating_rectangle.width;
			int h = c->floating_rectangle.height;
			restrain_floating_size(c, &w, &h);
			c->floating_rectangle.width = w;
			c->floating_rectangle.height = h;
			arrange(loc.monitor, loc.desktop);
		}
	}
}

void client_message(xcb_generic_event_t *evt)
{
	xcb_client_message_event_t *e = (xcb_client_message_event_t *) evt;

	if (e->type == ewmh->_NET_CURRENT_DESKTOP) {
		coordinates_t loc;
		if (ewmh_locate_desktop(e->data.data32[0], &loc))
			focus_node(loc.monitor, loc.desktop, loc.desktop->focus);
		return;
	}

	coordinates_t loc;
	if (!locate_window(e->window, &loc))
		return;

	if (e->type == ewmh->_NET_WM_STATE) {
		handle_state(loc.monitor, loc.desktop, loc.node, e->data.data32[1], e->data.data32[0]);
		handle_state(loc.monitor, loc.desktop, loc.node, e->data.data32[2], e->data.data32[0]);
	} else if (e->type == ewmh->_NET_ACTIVE_WINDOW) {
		if ((ignore_ewmh_focus && e->data.data32[0] == XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL) ||
		    loc.node == mon->desk->focus)
			return;
		focus_node(loc.monitor, loc.desktop, loc.node);
	} else if (e->type == ewmh->_NET_WM_DESKTOP) {
		coordinates_t dloc;
		if (ewmh_locate_desktop(e->data.data32[0], &dloc))
			transfer_node(loc.monitor, loc.desktop, loc.node, dloc.monitor, dloc.desktop, dloc.desktop->focus);
	} else if (e->type == ewmh->_NET_CLOSE_WINDOW) {
		window_close(loc.node);
	}
}

void focus_in(xcb_generic_event_t *evt)
{
	xcb_focus_in_event_t *e = (xcb_focus_in_event_t *) evt;

	if (e->mode == XCB_NOTIFY_MODE_GRAB || e->mode == XCB_NOTIFY_MODE_UNGRAB
	    || e->detail == XCB_NOTIFY_DETAIL_POINTER || e->detail == XCB_NOTIFY_DETAIL_POINTER_ROOT
	    || e->detail == XCB_NOTIFY_DETAIL_NONE) {
		return;
	}

	if (mon->desk->focus != NULL && e->event == mon->desk->focus->client->window) {
		return;
	}

	coordinates_t loc;
	if (locate_window(e->event, &loc)) {
		// prevent input focus stealing
		update_input_focus();
	}
}

void enter_notify(xcb_generic_event_t *evt)
{
	xcb_enter_notify_event_t *e = (xcb_enter_notify_event_t *) evt;
	xcb_window_t win = e->event;

	if (e->mode != XCB_NOTIFY_MODE_NORMAL ||
	    (mon->desk->focus != NULL &&
	     mon->desk->focus->client->window == win)) {
		return;
	}

	xcb_get_window_attributes_reply_t *wa = xcb_get_window_attributes_reply(dpy, xcb_get_window_attributes(dpy, motion_recorder), NULL);

	if (wa == NULL) {
		return;
	}

	if (wa->map_state == XCB_MAP_STATE_UNMAPPED) {
		enable_motion_recorder();
	} else {
		disable_motion_recorder();
	}
}

void motion_notify(xcb_generic_event_t *evt)
{
	xcb_motion_notify_event_t *e = (xcb_motion_notify_event_t *) evt;

	int dtime = e->time - last_motion_time;
	if (dtime > 1000) {
		last_motion_time = e->time;
		last_motion_x = e->event_x;
		last_motion_y = e->event_y;
		return;
	}

	int mdist = abs(e->event_x - last_motion_x) + abs(e->event_y - last_motion_y);
	if (mdist < 10) {
		return;
	}

	xcb_window_t win = XCB_NONE;
	xcb_point_t pt = {e->root_x, e->root_y};
	query_pointer(&win, NULL);

	bool pfm_backup = pointer_follows_monitor;
	bool pff_backup = pointer_follows_focus;
	auto_raise = false;
	pointer_follows_monitor = false;
	pointer_follows_focus = false;
	coordinates_t loc;
	if (locate_window(win, &loc)) {
		if (loc.node != mon->desk->focus) {
			focus_node(loc.monitor, loc.desktop, loc.node);
		}
	} else {
		monitor_t *m = monitor_from_point(pt);
		if (m != NULL && m != mon) {
			focus_node(m, m->desk, m->desk->focus);
		}
	}
	pointer_follows_monitor = pfm_backup;
	pointer_follows_focus = pff_backup;
	auto_raise = true;

	disable_motion_recorder();
}

void handle_state(monitor_t *m, desktop_t *d, node_t *n, xcb_atom_t state, unsigned int action)
{
	if (state == ewmh->_NET_WM_STATE_FULLSCREEN) {
		if (action == XCB_EWMH_WM_STATE_ADD) {
			set_state(m, d, n, STATE_FULLSCREEN);
		} else if (action == XCB_EWMH_WM_STATE_REMOVE) {
			set_state(m, d, n, STATE_TILED);
		} else if (action == XCB_EWMH_WM_STATE_TOGGLE) {
			set_state(m, d, n, IS_FULLSCREEN(n->client) ? STATE_TILED : STATE_FULLSCREEN);
		}
		arrange(m, d);
	} else if (state == ewmh->_NET_WM_STATE_BELOW) {
		if (action == XCB_EWMH_WM_STATE_ADD) {
			set_layer(m, d, n, LAYER_BELOW);
		} else if (action == XCB_EWMH_WM_STATE_REMOVE) {
			set_layer(m, d, n, LAYER_NORMAL);
		} else if (action == XCB_EWMH_WM_STATE_TOGGLE) {
			set_layer(m, d, n, n->client->layer == LAYER_BELOW ? LAYER_NORMAL : LAYER_BELOW);
		}
	} else if (state == ewmh->_NET_WM_STATE_ABOVE) {
		if (action == XCB_EWMH_WM_STATE_ADD) {
			set_layer(m, d, n, LAYER_ABOVE);
		} else if (action == XCB_EWMH_WM_STATE_REMOVE) {
			set_layer(m, d, n, LAYER_NORMAL);
		} else if (action == XCB_EWMH_WM_STATE_TOGGLE) {
			set_layer(m, d, n, n->client->layer == LAYER_ABOVE ? LAYER_NORMAL : LAYER_ABOVE);
		}
	} else if (state == ewmh->_NET_WM_STATE_STICKY) {
		if (action == XCB_EWMH_WM_STATE_ADD) {
			set_sticky(m, d, n, true);
		} else if (action == XCB_EWMH_WM_STATE_REMOVE) {
			set_sticky(m, d, n, false);
		} else if (action == XCB_EWMH_WM_STATE_TOGGLE) {
			set_sticky(m, d, n, !n->client->sticky);
		}
	} else if (state == ewmh->_NET_WM_STATE_DEMANDS_ATTENTION) {
		if (action == XCB_EWMH_WM_STATE_ADD) {
			set_urgency(m, d, n, true);
		} else if (action == XCB_EWMH_WM_STATE_REMOVE) {
			set_urgency(m, d, n, false);
		} else if (action == XCB_EWMH_WM_STATE_TOGGLE) {
			set_urgency(m, d, n, !n->client->urgent);
		}
	}
}

void process_error(xcb_generic_event_t *evt)
{
	xcb_request_error_t *e = (xcb_request_error_t *) evt;
	warn("Failed request: %s, %s: %d.\n", xcb_event_get_request_label(e->major_opcode), xcb_event_get_error_label(e->error_code), e->bad_value);
}
