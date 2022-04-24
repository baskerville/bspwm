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

#include <stdbool.h>
#include "bspwm.h"
#include "ewmh.h"
#include "monitor.h"
#include "query.h"
#include "settings.h"
#include "subscribe.h"
#include "tree.h"
#include "window.h"
#include "pointer.h"
#include "rule.h"
#include "events.h"

uint8_t randr_base;

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
		case XCB_CONFIGURE_NOTIFY:
			configure_notify(evt);
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
		case XCB_BUTTON_PRESS:
			button_press(evt);
			break;
		case XCB_FOCUS_IN:
			focus_in(evt);
			break;
		case XCB_MAPPING_NOTIFY:
			mapping_notify(evt);
			break;
		case 0:
			process_error(evt);
			break;
		default:
			if (randr && resp_type == randr_base + XCB_RANDR_SCREEN_CHANGE_NOTIFY) {
				update_monitors();
			}
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
	uint16_t width, height;

	if (!is_managed) {
		uint16_t mask = 0;
		uint32_t values[7];
		unsigned short i = 0;

		if (e->value_mask & XCB_CONFIG_WINDOW_X) {
			mask |= XCB_CONFIG_WINDOW_X;
			values[i++] = e->x;
		}

		if (e->value_mask & XCB_CONFIG_WINDOW_Y) {
			mask |= XCB_CONFIG_WINDOW_Y;
			values[i++] = e->y;
		}

		if (e->value_mask & XCB_CONFIG_WINDOW_WIDTH) {
			mask |= XCB_CONFIG_WINDOW_WIDTH;
			values[i++] = e->width;
		}

		if (e->value_mask & XCB_CONFIG_WINDOW_HEIGHT) {
			mask |= XCB_CONFIG_WINDOW_HEIGHT;
			values[i++] = e->height;
		}

		if (e->value_mask & XCB_CONFIG_WINDOW_BORDER_WIDTH) {
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

	} else if (IS_FLOATING(c)) {
		width = c->floating_rectangle.width;
		height = c->floating_rectangle.height;

		if (e->value_mask & XCB_CONFIG_WINDOW_X) {
			c->floating_rectangle.x = e->x;
		}

		if (e->value_mask & XCB_CONFIG_WINDOW_Y) {
			c->floating_rectangle.y = e->y;
		}

		if (e->value_mask & XCB_CONFIG_WINDOW_WIDTH) {
			width = e->width;
		}

		if (e->value_mask & XCB_CONFIG_WINDOW_HEIGHT) {
			height = e->height;
		}

		apply_size_hints(c, &width, &height);
		c->floating_rectangle.width = width;
		c->floating_rectangle.height = height;
		xcb_rectangle_t r = c->floating_rectangle;

		window_move_resize(e->window, r.x, r.y, r.width, r.height);

		put_status(SBSC_MASK_NODE_GEOMETRY, "node_geometry 0x%08X 0x%08X 0x%08X %ux%u+%i+%i\n", loc.monitor->id, loc.desktop->id, e->window, r.width, r.height, r.x, r.y);

		monitor_t *m = monitor_from_client(c);
		if (m != loc.monitor) {
			transfer_node(loc.monitor, loc.desktop, loc.node, m, m->desk, m->desk->focus, false);
		}
	} else {
		if (c->state == STATE_PSEUDO_TILED) {
			width = c->floating_rectangle.width;
			height = c->floating_rectangle.height;
			if (e->value_mask & XCB_CONFIG_WINDOW_WIDTH) {
				width = e->width;
			}
			if (e->value_mask & XCB_CONFIG_WINDOW_HEIGHT) {
				height = e->height;
			}
			apply_size_hints(c, &width, &height);
			if (width != c->floating_rectangle.width || height != c->floating_rectangle.height) {
				c->floating_rectangle.width = width;
				c->floating_rectangle.height = height;
				arrange(loc.monitor, loc.desktop);
			}
		}


		xcb_configure_notify_event_t evt;
		unsigned int bw = c->border_width;

		xcb_rectangle_t r = IS_FULLSCREEN(c) ? loc.monitor->rectangle : c->tiled_rectangle;

		evt.response_type = XCB_CONFIGURE_NOTIFY;
		evt.event = e->window;
		evt.window = e->window;
		evt.above_sibling = XCB_NONE;
		evt.x = r.x;
		evt.y = r.y;
		evt.width = r.width;
		evt.height = r.height;
		evt.border_width = bw;
		evt.override_redirect = false;

		xcb_send_event(dpy, false, e->window, XCB_EVENT_MASK_STRUCTURE_NOTIFY, (const char *) &evt);
	}
}

void configure_notify(xcb_generic_event_t *evt)
{
	xcb_configure_notify_event_t *e = (xcb_configure_notify_event_t *) evt;

	if (e->window == root) {
		screen_width = e->width;
		screen_height = e->height;
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

	if (e->window == motion_recorder.id) {
		/* Unmapping the motion recorder in `query_pointer` will produce
		 * unwanted enter notify events. We can filter those events because
		 * their sequence number is the same as the sequence number of the
		 * related unmap notify event. This is a technique used by i3-wm to
		 * filter enter notify events. */
		motion_recorder.sequence = e->sequence;
		return;
	}

	/* Filter out destroyed windows */
	if (!window_exists(e->window)) {
		return;
	}

	set_window_state(e->window, XCB_ICCCM_WM_STATE_WITHDRAWN);
	unmanage_window(e->window);
}

void property_notify(xcb_generic_event_t *evt)
{
	xcb_property_notify_event_t *e = (xcb_property_notify_event_t *) evt;

	if (!ignore_ewmh_struts && e->atom == ewmh->_NET_WM_STRUT_PARTIAL && ewmh_handle_struts(e->window)) {
		for (monitor_t *m = mon_head; m != NULL; m = m->next) {
			for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
				arrange(m, d);
			}
		}
	}

	if (e->atom != XCB_ATOM_WM_HINTS && e->atom != XCB_ATOM_WM_NORMAL_HINTS) {
		return;
	}

	coordinates_t loc;
	if (!locate_window(e->window, &loc)) {
		for (pending_rule_t *pr = pending_rule_head; pr != NULL; pr = pr->next) {
			if (pr->win == e->window) {
				postpone_event(pr, evt);
				break;
			}
		}
		return;
	}

	if (e->atom == XCB_ATOM_WM_HINTS) {
		xcb_icccm_wm_hints_t hints;
		if (xcb_icccm_get_wm_hints_reply(dpy, xcb_icccm_get_wm_hints(dpy, e->window), &hints, NULL) == 1 &&
		    (hints.flags & XCB_ICCCM_WM_HINT_X_URGENCY))
			set_urgent(loc.monitor, loc.desktop, loc.node, xcb_icccm_wm_hints_get_urgency(&hints));
	} else if (e->atom == XCB_ATOM_WM_NORMAL_HINTS) {
		client_t *c = loc.node->client;
		if (xcb_icccm_get_wm_normal_hints_reply(dpy, xcb_icccm_get_wm_normal_hints(dpy, e->window), &c->size_hints, NULL) == 1) {
			arrange(loc.monitor, loc.desktop);
		}
	}
}

void client_message(xcb_generic_event_t *evt)
{
	xcb_client_message_event_t *e = (xcb_client_message_event_t *) evt;

	if (e->type == ewmh->_NET_CURRENT_DESKTOP) {
		coordinates_t loc;
		if (ewmh_locate_desktop(e->data.data32[0], &loc)) {
			focus_node(loc.monitor, loc.desktop, loc.desktop->focus);
		}
		return;
	}

	coordinates_t loc;
	if (!locate_window(e->window, &loc)) {
		for (pending_rule_t *pr = pending_rule_head; pr != NULL; pr = pr->next) {
			if (pr->win == e->window) {
				postpone_event(pr, evt);
				break;
			}
		}
		return;
	}

	if (e->type == ewmh->_NET_WM_STATE) {
		handle_state(loc.monitor, loc.desktop, loc.node, e->data.data32[1], e->data.data32[0]);
		handle_state(loc.monitor, loc.desktop, loc.node, e->data.data32[2], e->data.data32[0]);
	} else if (e->type == ewmh->_NET_ACTIVE_WINDOW) {
		if ((ignore_ewmh_focus && e->data.data32[0] == XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL) ||
		    loc.node == mon->desk->focus) {
			return;
		}
		focus_node(loc.monitor, loc.desktop, loc.node);
	} else if (e->type == ewmh->_NET_WM_DESKTOP) {
		coordinates_t dloc;
		if (ewmh_locate_desktop(e->data.data32[0], &dloc)) {
			transfer_node(loc.monitor, loc.desktop, loc.node, dloc.monitor, dloc.desktop, dloc.desktop->focus, false);
		}
	} else if (e->type == ewmh->_NET_CLOSE_WINDOW) {
		close_node(loc.node);
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

	if (mon->desk->focus != NULL) {
		if (e->event == mon->desk->focus->id) {
			return;
		}
		if (e->event == root) {
			/* Some clients expect the window manager to refocus the
			   focused window in this case */
			focus_node(mon, mon->desk, mon->desk->focus);
			return;
		}
	}

	coordinates_t loc;
	if (locate_window(e->event, &loc)) {
		// prevent input focus stealing
		update_input_focus();
	}
}

void button_press(xcb_generic_event_t *evt)
{
	xcb_button_press_event_t *e = (xcb_button_press_event_t *) evt;
	bool replay = false;
	for (unsigned int i = 0; i < LENGTH(BUTTONS); i++) {
		if (e->detail != BUTTONS[i]) {
			continue;
		}
		if ((click_to_focus == (int8_t) XCB_BUTTON_INDEX_ANY || click_to_focus == (int8_t) BUTTONS[i]) &&
			cleaned_mask(e->state) == XCB_NONE) {
			bool pff = pointer_follows_focus;
			bool pfm = pointer_follows_monitor;
			pointer_follows_focus = false;
			pointer_follows_monitor = false;
			replay = !grab_pointer(ACTION_FOCUS) || !swallow_first_click;
			pointer_follows_focus = pff;
			pointer_follows_monitor = pfm;
		} else {
			grab_pointer(pointer_actions[i]);
		}
	}
	xcb_allow_events(dpy, replay ? XCB_ALLOW_REPLAY_POINTER : XCB_ALLOW_SYNC_POINTER, e->time);
	xcb_flush(dpy);
}

void enter_notify(xcb_generic_event_t *evt)
{
	xcb_enter_notify_event_t *e = (xcb_enter_notify_event_t *) evt;
	xcb_window_t win = e->event;

	if (e->mode != XCB_NOTIFY_MODE_NORMAL || e->detail == XCB_NOTIFY_DETAIL_INFERIOR) {
		return;
	}

	/* Ignore the enter notify events that we generated by unmapping the motion
	 * recorder window in `query_pointer`. */
	if (motion_recorder.enabled && motion_recorder.sequence == e->sequence) {
		return;
	}

	if (win == mon->root || (mon->desk->focus != NULL &&
	                         (win == mon->desk->focus->id ||
	                          (mon->desk->focus->presel != NULL &&
	                           win == mon->desk->focus->presel->feedback)))) {
		return;
	}

	update_motion_recorder();
}

void motion_notify(xcb_generic_event_t *evt)
{
	xcb_motion_notify_event_t *e = (xcb_motion_notify_event_t *) evt;

	static uint16_t last_motion_x = 0, last_motion_y = 0;
	static xcb_timestamp_t last_motion_time = 0;

	int64_t dtime = e->time - last_motion_time;

	/* Ignore unintentional pointer motions. */
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

	disable_motion_recorder();

	xcb_window_t win = XCB_NONE;
	query_pointer(&win, NULL);
	coordinates_t loc;
	bool pff = pointer_follows_focus;
	bool pfm = pointer_follows_monitor;
	pointer_follows_focus = false;
	pointer_follows_monitor = false;
	auto_raise = false;

	if (locate_window(win, &loc)) {
		if (loc.monitor->desk == loc.desktop && loc.node != mon->desk->focus) {
			focus_node(loc.monitor, loc.desktop, loc.node);
		}
	} else {
		xcb_point_t pt = {e->root_x, e->root_y};
		monitor_t *m = monitor_from_point(pt);
		if (m != NULL && m != mon) {
			focus_node(m, m->desk, m->desk->focus);
		}
	}

	pointer_follows_focus = pff;
	pointer_follows_monitor = pfm;
	auto_raise = true;
}

void handle_state(monitor_t *m, desktop_t *d, node_t *n, xcb_atom_t state, unsigned int action)
{
	if (state == ewmh->_NET_WM_STATE_FULLSCREEN) {
		if (action == XCB_EWMH_WM_STATE_ADD && (ignore_ewmh_fullscreen & STATE_TRANSITION_ENTER) == 0) {
			set_state(m, d, n, STATE_FULLSCREEN);
		} else if (action == XCB_EWMH_WM_STATE_REMOVE && (ignore_ewmh_fullscreen & STATE_TRANSITION_EXIT) == 0) {
			if (n->client->state == STATE_FULLSCREEN) {
				set_state(m, d, n, n->client->last_state);
			}
		} else if (action == XCB_EWMH_WM_STATE_TOGGLE) {
			client_state_t next_state = IS_FULLSCREEN(n->client) ? n->client->last_state : STATE_FULLSCREEN;
			if ((next_state == STATE_FULLSCREEN && (ignore_ewmh_fullscreen & STATE_TRANSITION_ENTER) == 0) ||
			    (next_state != STATE_FULLSCREEN && (ignore_ewmh_fullscreen & STATE_TRANSITION_EXIT) == 0)) {
				set_state(m, d, n, next_state);
			}
		}
		arrange(m, d);
	} else if (state == ewmh->_NET_WM_STATE_BELOW) {
		if (action == XCB_EWMH_WM_STATE_ADD) {
			set_layer(m, d, n, LAYER_BELOW);
		} else if (action == XCB_EWMH_WM_STATE_REMOVE) {
			if (n->client->layer == LAYER_BELOW) {
				set_layer(m, d, n, n->client->last_layer);
			}
		} else if (action == XCB_EWMH_WM_STATE_TOGGLE) {
			set_layer(m, d, n, n->client->layer == LAYER_BELOW ? n->client->last_layer : LAYER_BELOW);
		}
	} else if (state == ewmh->_NET_WM_STATE_ABOVE) {
		if (action == XCB_EWMH_WM_STATE_ADD) {
			set_layer(m, d, n, LAYER_ABOVE);
		} else if (action == XCB_EWMH_WM_STATE_REMOVE) {
			if (n->client->layer == LAYER_ABOVE) {
				set_layer(m, d, n, n->client->last_layer);
			}
		} else if (action == XCB_EWMH_WM_STATE_TOGGLE) {
			set_layer(m, d, n, n->client->layer == LAYER_ABOVE ? n->client->last_layer : LAYER_ABOVE);
		}
	} else if (state == ewmh->_NET_WM_STATE_HIDDEN) {
		if (action == XCB_EWMH_WM_STATE_ADD) {
			set_hidden(m, d, n, true);
		} else if (action == XCB_EWMH_WM_STATE_REMOVE) {
			set_hidden(m, d, n, false);
		} else if (action == XCB_EWMH_WM_STATE_TOGGLE) {
			set_hidden(m, d, n, !n->hidden);
		}
	} else if (state == ewmh->_NET_WM_STATE_STICKY) {
		if (action == XCB_EWMH_WM_STATE_ADD) {
			set_sticky(m, d, n, true);
		} else if (action == XCB_EWMH_WM_STATE_REMOVE) {
			set_sticky(m, d, n, false);
		} else if (action == XCB_EWMH_WM_STATE_TOGGLE) {
			set_sticky(m, d, n, !n->sticky);
		}
	} else if (state == ewmh->_NET_WM_STATE_DEMANDS_ATTENTION) {
		if (action == XCB_EWMH_WM_STATE_ADD) {
			set_urgent(m, d, n, true);
		} else if (action == XCB_EWMH_WM_STATE_REMOVE) {
			set_urgent(m, d, n, false);
		} else if (action == XCB_EWMH_WM_STATE_TOGGLE) {
			set_urgent(m, d, n, !n->client->urgent);
		}
#define HANDLE_WM_STATE(s)  \
	} else if (state == ewmh->_NET_WM_STATE_##s) { \
		if (action == XCB_EWMH_WM_STATE_ADD) { \
			n->client->wm_flags |= WM_FLAG_##s; \
		} else if (action == XCB_EWMH_WM_STATE_REMOVE) { \
			n->client->wm_flags &= ~WM_FLAG_##s; \
		} else if (action == XCB_EWMH_WM_STATE_TOGGLE) { \
			n->client->wm_flags ^= WM_FLAG_##s; \
		} \
		ewmh_wm_state_update(n);
	HANDLE_WM_STATE(MODAL)
	HANDLE_WM_STATE(MAXIMIZED_VERT)
	HANDLE_WM_STATE(MAXIMIZED_HORZ)
	HANDLE_WM_STATE(SHADED)
	HANDLE_WM_STATE(SKIP_TASKBAR)
	HANDLE_WM_STATE(SKIP_PAGER)
	}
#undef HANDLE_WM_STATE
}

void mapping_notify(xcb_generic_event_t *evt)
{
	if (mapping_events_count == 0) {
		return;
	}

	xcb_mapping_notify_event_t *e = (xcb_mapping_notify_event_t *) evt;

	if (e->request == XCB_MAPPING_POINTER) {
		return;
	}

	if (mapping_events_count > 0) {
		mapping_events_count--;
	}

	ungrab_buttons();
	grab_buttons();
}

void process_error(xcb_generic_event_t *evt)
{
	xcb_request_error_t *e = (xcb_request_error_t *) evt;
	/* Ignore unavoidable failed requests */
	if (e->error_code == ERROR_CODE_BAD_WINDOW) {
		return;
	}
	warn("Failed request: %s, %s: 0x%08X.\n", xcb_event_get_request_label(e->major_opcode), xcb_event_get_error_label(e->error_code), e->bad_value);
}
