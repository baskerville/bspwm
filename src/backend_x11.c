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

/*
 * X11/XCB backend implementation.
 *
 * This file implements the backend_* interface from backend.h using XCB.
 * It holds the X11-specific global state (connection, screen, atoms)
 * and delegates to the existing window.c/events.c/ewmh.c/pointer.c.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/randr.h>
#include <xcb/xinerama.h>
#include <xcb/shape.h>
#include <xkbcommon/xkbcommon.h>
#include "bspwm.h"
#include "backend.h"
#include "backend_x11.h"
#include "ewmh.h"
#include "keybind.h"
#include "settings.h"

/* ------------------------------------------------------------------ */
/*  X11-specific global state                                         */
/* ------------------------------------------------------------------ */

xcb_connection_t *dpy;
xcb_screen_t *screen;
xcb_ewmh_connection_t *ewmh;

xcb_atom_t WM_STATE;
xcb_atom_t WM_TAKE_FOCUS;
xcb_atom_t WM_DELETE_WINDOW;
xcb_atom_t WM_WINDOW_ROLE;

uint8_t randr_base;

/* Forward declarations for atom helpers */
static void get_atom(char *name, xcb_atom_t *atom);

/* ------------------------------------------------------------------ */
/*  Lifecycle                                                         */
/* ------------------------------------------------------------------ */

int backend_init(int *default_screen_out)
{
	dpy = xcb_connect(NULL, default_screen_out);
	if (xcb_connection_has_error(dpy)) {
		return -1;
	}
	return 0;
}

void backend_destroy(void)
{
	if (ewmh) {
		xcb_ewmh_connection_wipe(ewmh);
		free(ewmh);
		ewmh = NULL;
	}
	if (dpy) {
		xcb_flush(dpy);
		xcb_disconnect(dpy);
		dpy = NULL;
	}
}

int backend_get_fd(void)
{
	return xcb_get_file_descriptor(dpy);
}

void backend_flush(void)
{
	xcb_flush(dpy);
}

bool backend_dispatch_events(void)
{
	xcb_aux_sync(dpy);
	return true;
}

bool backend_check_connection(void)
{
	return xcb_connection_has_error(dpy) == 0;
}

/* ------------------------------------------------------------------ */
/*  Screen / root                                                     */
/* ------------------------------------------------------------------ */

void backend_get_screen_size(int *width, int *height)
{
	*width = screen->width_in_pixels;
	*height = screen->height_in_pixels;
}

bspwm_wid_t backend_get_root(void)
{
	return screen->root;
}

/* ------------------------------------------------------------------ */
/*  Window management                                                 */
/* ------------------------------------------------------------------ */

bspwm_wid_t backend_create_internal_window(const char *kind, bspwm_rect_t rect, bool input_only)
{
	bspwm_wid_t win = xcb_generate_id(dpy);
	uint32_t mask = 0;
	uint32_t values[2];
	int vi = 0;

	if (strcmp(kind, "motion_recorder") == 0) {
		mask = XCB_CW_EVENT_MASK;
		values[vi++] = XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_POINTER_MOTION;
	} else if (strcmp(kind, "monitor_root") == 0) {
		mask = XCB_CW_EVENT_MASK;
		values[vi++] = XCB_EVENT_MASK_ENTER_WINDOW;
	} else if (strcmp(kind, "presel_feedback") == 0) {
		mask = XCB_CW_BACK_PIXEL | XCB_CW_SAVE_UNDER;
		values[vi++] = 0; /* color set separately */
		values[vi++] = 1;
	}

	xcb_create_window(dpy, XCB_COPY_FROM_PARENT, win, screen->root,
	                  rect.x, rect.y, rect.width ? rect.width : 1, rect.height ? rect.height : 1, 0,
	                  input_only ? XCB_WINDOW_CLASS_INPUT_ONLY : XCB_WINDOW_CLASS_INPUT_OUTPUT,
	                  XCB_COPY_FROM_PARENT, mask, values);

	/* Set WM class based on kind */
	if (strcmp(kind, "meta") == 0) {
		xcb_icccm_set_wm_class(dpy, win, sizeof("wm\0Bspwm"), "wm\0Bspwm");
	} else if (strcmp(kind, "motion_recorder") == 0) {
		xcb_icccm_set_wm_class(dpy, win, sizeof("motion_recorder\0Bspwm"), "motion_recorder\0Bspwm");
	} else if (strcmp(kind, "monitor_root") == 0) {
		xcb_icccm_set_wm_class(dpy, win, sizeof("root\0Bspwm"), "root\0Bspwm");
		if (ewmh) {
			xcb_ewmh_set_wm_window_type(ewmh, win, 1, &ewmh->_NET_WM_WINDOW_TYPE_DESKTOP);
		}
	} else if (strcmp(kind, "presel_feedback") == 0) {
		xcb_icccm_set_wm_class(dpy, win, sizeof("presel_feedback\0Bspwm"), "presel_feedback\0Bspwm");
		/* Make input shape NULL to pass clicks through */
		xcb_shape_rectangles(dpy, XCB_SHAPE_SO_SET, XCB_SHAPE_SK_INPUT,
		                     XCB_CLIP_ORDERING_UNSORTED, win, 0, 0, 0, NULL);
	}

	return win;
}

void backend_destroy_window(bspwm_wid_t win)
{
	xcb_destroy_window(dpy, win);
}

void backend_window_show(bspwm_wid_t win)
{
	xcb_map_window(dpy, win);
}

void backend_window_hide(bspwm_wid_t win)
{
	xcb_unmap_window(dpy, win);
}

void backend_window_move(bspwm_wid_t win, int16_t x, int16_t y)
{
	uint32_t values[] = {(uint32_t)x, (uint32_t)y};
	xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);
}

void backend_window_resize(bspwm_wid_t win, uint16_t w, uint16_t h)
{
	uint32_t values[] = {w, h};
	xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
}

void backend_window_move_resize(bspwm_wid_t win, int16_t x, int16_t y, uint16_t w, uint16_t h)
{
	uint32_t values[] = {(uint32_t)x, (uint32_t)y, w, h};
	xcb_configure_window(dpy, win,
		XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
		XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
}

void backend_window_set_border_width(bspwm_wid_t win, uint32_t bw)
{
	uint32_t values[] = {bw};
	xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_BORDER_WIDTH, values);
}

void backend_window_set_border_color(bspwm_wid_t win, uint32_t color)
{
	uint32_t values[] = {color};
	xcb_change_window_attributes(dpy, win, XCB_CW_BORDER_PIXEL, values);
}

bool backend_window_exists(bspwm_wid_t win)
{
	xcb_generic_error_t *err = NULL;
	xcb_get_window_attributes_reply_t *wa = xcb_get_window_attributes_reply(
		dpy, xcb_get_window_attributes(dpy, win), &err);
	if (wa) {
		free(wa);
		return true;
	}
	free(err);
	return false;
}

bool backend_window_get_geometry(bspwm_wid_t win, bspwm_rect_t *rect)
{
	xcb_get_geometry_reply_t *geo = xcb_get_geometry_reply(
		dpy, xcb_get_geometry(dpy, win), NULL);
	if (!geo) return false;
	rect->x = geo->x;
	rect->y = geo->y;
	rect->width = geo->width;
	rect->height = geo->height;
	free(geo);
	return true;
}

void backend_window_listen_enter(bspwm_wid_t win, bool enable)
{
	uint32_t mask = XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_FOCUS_CHANGE |
	                (enable ? XCB_EVENT_MASK_ENTER_WINDOW : 0);
	xcb_change_window_attributes(dpy, win, XCB_CW_EVENT_MASK, &mask);
}

/* ------------------------------------------------------------------ */
/*  Stacking                                                          */
/* ------------------------------------------------------------------ */

void backend_window_stack_above(bspwm_wid_t w1, bspwm_wid_t w2)
{
	uint32_t values[] = {w2, XCB_STACK_MODE_ABOVE};
	xcb_configure_window(dpy, w1,
		XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE, values);
}

void backend_window_stack_below(bspwm_wid_t w1, bspwm_wid_t w2)
{
	uint32_t values[] = {w2, XCB_STACK_MODE_BELOW};
	xcb_configure_window(dpy, w1,
		XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE, values);
}

void backend_window_raise(bspwm_wid_t win)
{
	uint32_t values[] = {XCB_STACK_MODE_ABOVE};
	xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_STACK_MODE, values);
}

void backend_window_lower(bspwm_wid_t win)
{
	uint32_t values[] = {XCB_STACK_MODE_BELOW};
	xcb_configure_window(dpy, win, XCB_CONFIG_WINDOW_STACK_MODE, values);
}

/* ------------------------------------------------------------------ */
/*  Focus                                                             */
/* ------------------------------------------------------------------ */

void backend_set_input_focus(bspwm_wid_t win)
{
	xcb_set_input_focus(dpy, XCB_INPUT_FOCUS_POINTER_ROOT, win, XCB_CURRENT_TIME);
}

void backend_clear_input_focus(void)
{
	xcb_set_input_focus(dpy, XCB_INPUT_FOCUS_POINTER_ROOT, root, XCB_CURRENT_TIME);
}

/* ------------------------------------------------------------------ */
/*  Window properties                                                 */
/* ------------------------------------------------------------------ */

bool backend_get_window_class(bspwm_wid_t win, char *class_name, char *instance_name, size_t len)
{
	xcb_icccm_get_wm_class_reply_t reply;
	if (xcb_icccm_get_wm_class_reply(dpy, xcb_icccm_get_wm_class(dpy, win), &reply, NULL) != 1)
		return false;
	if (reply.class_name)
		snprintf(class_name, len, "%s", reply.class_name);
	if (reply.instance_name)
		snprintf(instance_name, len, "%s", reply.instance_name);
	xcb_icccm_get_wm_class_reply_wipe(&reply);
	return true;
}

bool backend_get_window_name(bspwm_wid_t win, char *name, size_t len)
{
	xcb_icccm_get_text_property_reply_t reply;
	if (xcb_icccm_get_wm_name_reply(dpy, xcb_icccm_get_wm_name(dpy, win), &reply, NULL) != 1)
		return false;
	size_t safe_len = (size_t)reply.name_len < len - 1 ? (size_t)reply.name_len : len - 1;
	memcpy(name, reply.name, safe_len);
	name[safe_len] = '\0';
	xcb_icccm_get_text_property_reply_wipe(&reply);
	return true;
}

bool backend_get_window_role(bspwm_wid_t win, char *role, size_t len)
{
	if (role == NULL || len == 0) {
		return false;
	}
	role[0] = '\0';
	/* Interned once, in x11_setup_atoms(), like the other WM_* atoms. */
	if (WM_WINDOW_ROLE == XCB_ATOM_NONE) {
		return false;
	}
	/* long_length counts 32-bit units, not bytes: ask for just enough of
	 * them to fill `role`. */
	xcb_get_property_reply_t *reply = xcb_get_property_reply(dpy,
		xcb_get_property(dpy, 0, win, WM_WINDOW_ROLE, XCB_ATOM_STRING, 0,
		                 (uint32_t) ((len + 3) / 4)), NULL);
	if (reply == NULL) {
		return false;
	}
	int value_len = xcb_get_property_value_length(reply);
	if (value_len > 0) {
		size_t safe_len = (size_t) value_len < len - 1 ? (size_t) value_len : len - 1;
		memcpy(role, xcb_get_property_value(reply), safe_len);
		role[safe_len] = '\0';
	}
	free(reply);
	return role[0] != '\0';
}

bool backend_get_icccm_props(bspwm_wid_t win, bspwm_icccm_props_t *props)
{
	/* Pipeline: send both requests first */
	xcb_get_property_cookie_t protos_cookie = xcb_icccm_get_wm_protocols(dpy, win, ewmh->WM_PROTOCOLS);
	xcb_get_property_cookie_t hints_cookie = xcb_icccm_get_wm_hints(dpy, win);

	/* Collect protocols */
	xcb_icccm_get_wm_protocols_reply_t protos;
	if (xcb_icccm_get_wm_protocols_reply(dpy, protos_cookie, &protos, NULL) == 1) {
		for (uint32_t i = 0; i < protos.atoms_len && i < 32; i++) {
			if (protos.atoms[i] == WM_TAKE_FOCUS)
				props->take_focus = true;
			else if (protos.atoms[i] == WM_DELETE_WINDOW)
				props->delete_window = true;
		}
		xcb_icccm_get_wm_protocols_reply_wipe(&protos);
	}

	/* Collect hints */
	xcb_icccm_wm_hints_t hints;
	if (xcb_icccm_get_wm_hints_reply(dpy, hints_cookie, &hints, NULL) == 1 &&
	    (hints.flags & XCB_ICCCM_WM_HINT_INPUT)) {
		props->input_hint = hints.input;
	}

	return true;
}

bool backend_get_size_hints(bspwm_wid_t win, bspwm_size_hints_t *hints)
{
	xcb_size_hints_t xcb_hints;
	if (xcb_icccm_get_wm_normal_hints_reply(dpy,
	    xcb_icccm_get_wm_normal_hints(dpy, win), &xcb_hints, NULL) != 1)
		return false;

	hints->flags = 0;
	if (xcb_hints.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE)
		hints->flags |= BSP_SIZE_HINT_P_MIN_SIZE;
	if (xcb_hints.flags & XCB_ICCCM_SIZE_HINT_P_MAX_SIZE)
		hints->flags |= BSP_SIZE_HINT_P_MAX_SIZE;
	if (xcb_hints.flags & XCB_ICCCM_SIZE_HINT_P_RESIZE_INC)
		hints->flags |= BSP_SIZE_HINT_P_RESIZE_INC;
	if (xcb_hints.flags & XCB_ICCCM_SIZE_HINT_BASE_SIZE)
		hints->flags |= BSP_SIZE_HINT_BASE_SIZE;
	if (xcb_hints.flags & XCB_ICCCM_SIZE_HINT_P_ASPECT)
		hints->flags |= BSP_SIZE_HINT_P_ASPECT;

	hints->min_width = xcb_hints.min_width;
	hints->min_height = xcb_hints.min_height;
	hints->max_width = xcb_hints.max_width;
	hints->max_height = xcb_hints.max_height;
	hints->width_inc = xcb_hints.width_inc;
	hints->height_inc = xcb_hints.height_inc;
	hints->base_width = xcb_hints.base_width;
	hints->base_height = xcb_hints.base_height;
	hints->min_aspect_num = xcb_hints.min_aspect_num;
	hints->min_aspect_den = xcb_hints.min_aspect_den;
	hints->max_aspect_num = xcb_hints.max_aspect_num;
	hints->max_aspect_den = xcb_hints.max_aspect_den;

	return true;
}

bool backend_get_transient_for(bspwm_wid_t win, bspwm_wid_t *transient_for)
{
	xcb_window_t tf = XCB_NONE;
	xcb_icccm_get_wm_transient_for_reply(dpy,
		xcb_icccm_get_wm_transient_for(dpy, win), &tf, NULL);
	*transient_for = tf;
	return tf != XCB_NONE;
}

bool backend_is_override_redirect(bspwm_wid_t win)
{
	xcb_get_window_attributes_reply_t *wa = xcb_get_window_attributes_reply(
		dpy, xcb_get_window_attributes(dpy, win), NULL);
	if (!wa) return false;
	bool or_flag = wa->override_redirect;
	free(wa);
	return or_flag;
}

bool backend_get_urgency(bspwm_wid_t win)
{
	xcb_icccm_wm_hints_t hints;
	if (xcb_icccm_get_wm_hints_reply(dpy, xcb_icccm_get_wm_hints(dpy, win), &hints, NULL) == 1) {
		return (hints.flags & XCB_ICCCM_WM_HINT_X_URGENCY) && xcb_icccm_wm_hints_get_urgency(&hints);
	}
	return false;
}

void backend_set_window_state(bspwm_wid_t win, bspwm_wm_state_t state)
{
	uint32_t data[] = {state, XCB_NONE};
	xcb_change_property(dpy, XCB_PROP_MODE_REPLACE, win, WM_STATE, WM_STATE, 32, 2, data);
}

/* ------------------------------------------------------------------ */
/*  Window type                                                       */
/* ------------------------------------------------------------------ */

bool backend_get_window_type(bspwm_wid_t win, bspwm_window_type_t *type)
{
	xcb_ewmh_get_atoms_reply_t reply;
	if (xcb_ewmh_get_wm_window_type_reply(ewmh,
	    xcb_ewmh_get_wm_window_type(ewmh, win), &reply, NULL) != 1)
		return false;

	bool found = false;
	for (unsigned int i = 0; i < reply.atoms_len && !found; i++) {
		xcb_atom_t a = reply.atoms[i];
		if (a == ewmh->_NET_WM_WINDOW_TYPE_DOCK) { *type = BSP_WINDOW_TYPE_DOCK; found = true; }
		else if (a == ewmh->_NET_WM_WINDOW_TYPE_DESKTOP) { *type = BSP_WINDOW_TYPE_DESKTOP; found = true; }
		else if (a == ewmh->_NET_WM_WINDOW_TYPE_NOTIFICATION) { *type = BSP_WINDOW_TYPE_NOTIFICATION; found = true; }
		else if (a == ewmh->_NET_WM_WINDOW_TYPE_DIALOG) { *type = BSP_WINDOW_TYPE_DIALOG; found = true; }
		else if (a == ewmh->_NET_WM_WINDOW_TYPE_UTILITY) { *type = BSP_WINDOW_TYPE_UTILITY; found = true; }
		else if (a == ewmh->_NET_WM_WINDOW_TYPE_TOOLBAR) { *type = BSP_WINDOW_TYPE_TOOLBAR; found = true; }
	}
	xcb_ewmh_get_atoms_reply_wipe(&reply);
	if (!found) *type = BSP_WINDOW_TYPE_NORMAL;
	return true;
}

/* ------------------------------------------------------------------ */
/*  EWMH / compositor state                                          */
/* ------------------------------------------------------------------ */

void backend_ewmh_init(void)
{
	ewmh = calloc(1, sizeof(xcb_ewmh_connection_t));
	if (ewmh == NULL) {
		err("Can't allocate the EWMH connection.\n");
	}
	xcb_intern_atom_cookie_t *ewmh_cookies = xcb_ewmh_init_atoms(dpy, ewmh);
	xcb_ewmh_init_atoms_replies(ewmh, ewmh_cookies, NULL);
}

/* Other EWMH functions are in ewmh.c, which uses the ewmh global directly */

bool backend_ewmh_handle_struts(bspwm_wid_t win)
{
	return ewmh_handle_struts(win);
}

/* ------------------------------------------------------------------ */
/*  Monitor / output discovery                                        */
/* ------------------------------------------------------------------ */

#define MAX_MONITORS 256

int backend_query_outputs(bspwm_output_info_t *outputs, int max)
{
	xcb_randr_get_screen_resources_reply_t *sres =
		xcb_randr_get_screen_resources_reply(dpy,
			xcb_randr_get_screen_resources(dpy, root), NULL);
	if (!sres) return 0;

	int len = xcb_randr_get_screen_resources_outputs_length(sres);
	xcb_randr_output_t *xoutputs = xcb_randr_get_screen_resources_outputs(sres);

	if (len < 0 || len > MAX_MONITORS) {
		free(sres);
		return 0;
	}

	/* Pipeline: send all output info requests */
	xcb_randr_get_output_info_cookie_t cookies[MAX_MONITORS];
	for (int i = 0; i < len; i++)
		cookies[i] = xcb_randr_get_output_info(dpy, xoutputs[i], XCB_CURRENT_TIME);

	/* Get primary output */
	xcb_randr_get_output_primary_reply_t *gpo =
		xcb_randr_get_output_primary_reply(dpy,
			xcb_randr_get_output_primary(dpy, root), NULL);
	xcb_randr_output_t primary_output = gpo ? gpo->output : XCB_NONE;
	free(gpo);

	int count = 0;
	for (int i = 0; i < len && count < max; i++) {
		xcb_randr_get_output_info_reply_t *info =
			xcb_randr_get_output_info_reply(dpy, cookies[i], NULL);
		if (!info) continue;

		if (info->crtc != XCB_NONE) {
			xcb_randr_get_crtc_info_reply_t *cir =
				xcb_randr_get_crtc_info_reply(dpy,
					xcb_randr_get_crtc_info(dpy, info->crtc, XCB_CURRENT_TIME), NULL);
			if (cir) {
				bspwm_output_info_t *o = &outputs[count];
				o->rect = (bspwm_rect_t){cir->x, cir->y, cir->width, cir->height};
				o->id = xoutputs[i];
				o->primary = (xoutputs[i] == primary_output);

				char *name = (char *)xcb_randr_get_output_info_name(info);
				size_t name_len = xcb_randr_get_output_info_name_length(info);
				if (name_len > sizeof(o->name) - 1)
					name_len = sizeof(o->name) - 1;
				memcpy(o->name, name, name_len);
				o->name[name_len] = '\0';

				count++;
				free(cir);
			}
		}
		free(info);
	}

	free(sres);
	return count;
}

void backend_listen_output_changes(void)
{
	xcb_randr_select_input(dpy, root, XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);
}

/* ------------------------------------------------------------------ */
/*  Input                                                             */
/* ------------------------------------------------------------------ */

void backend_pointer_init(void)
{
	/* Delegated to pointer.c pointer_init() */
}

void backend_query_pointer(bspwm_wid_t *win, bspwm_point_t *pos)
{
	xcb_query_pointer_reply_t *qpr = xcb_query_pointer_reply(dpy,
		xcb_query_pointer(dpy, root), NULL);
	if (!qpr) return;
	if (win) *win = qpr->child;
	if (pos) { pos->x = qpr->root_x; pos->y = qpr->root_y; }
	free(qpr);
}

void backend_warp_pointer(bspwm_rect_t rect)
{
	int16_t cx = rect.x + rect.width / 2;
	int16_t cy = rect.y + rect.height / 2;
	xcb_warp_pointer(dpy, XCB_NONE, root, 0, 0, 0, 0, cx, cy);
}

void backend_allow_events(bool replay, uint32_t timestamp)
{
	xcb_allow_events(dpy, replay ? XCB_ALLOW_REPLAY_POINTER : XCB_ALLOW_SYNC_POINTER, timestamp);
	xcb_flush(dpy);
}

/* ------------------------------------------------------------------ */
/*  Client message / close                                            */
/* ------------------------------------------------------------------ */

void backend_close_window(bspwm_wid_t win)
{
	xcb_kill_client(dpy, win);
}

void backend_request_close(bspwm_wid_t win)
{
	xcb_client_message_event_t evt = {0};
	evt.response_type = XCB_CLIENT_MESSAGE;
	evt.format = 32;
	evt.window = win;
	evt.type = ewmh->WM_PROTOCOLS;
	evt.data.data32[0] = WM_DELETE_WINDOW;
	evt.data.data32[1] = XCB_CURRENT_TIME;
	xcb_send_event(dpy, false, win, XCB_EVENT_MASK_NO_EVENT, (const char *)&evt);
}

void backend_send_take_focus(bspwm_wid_t win, bspwm_icccm_props_t *props)
{
	if (!props || !props->take_focus) return;

	xcb_client_message_event_t evt;
	evt.response_type = XCB_CLIENT_MESSAGE;
	evt.format = 32;
	evt.sequence = 0;
	evt.window = win;
	evt.type = ewmh->WM_PROTOCOLS;
	evt.data.data32[0] = WM_TAKE_FOCUS;
	evt.data.data32[1] = XCB_CURRENT_TIME;

	xcb_send_event(dpy, false, win, XCB_EVENT_MASK_NO_EVENT, (const char *)&evt);
}

void backend_send_configure_notify(bspwm_wid_t win, bspwm_rect_t rect, uint32_t border_width)
{
	xcb_configure_notify_event_t evt = {0};
	evt.response_type = XCB_CONFIGURE_NOTIFY;
	evt.event = win;
	evt.window = win;
	evt.above_sibling = XCB_NONE;
	evt.x = rect.x;
	evt.y = rect.y;
	evt.width = rect.width;
	evt.height = rect.height;
	evt.border_width = border_width;
	evt.override_redirect = false;

	xcb_send_event(dpy, false, win, XCB_EVENT_MASK_STRUCTURE_NOTIFY, (const char *)&evt);
}

/* ------------------------------------------------------------------ */
/*  Color                                                             */
/* ------------------------------------------------------------------ */

uint32_t backend_get_color_pixel(const char *color)
{
	unsigned int red, green, blue;
	if (sscanf(color + 1, "%02x%02x%02x", &red, &green, &blue) == 3) {
		return (0xFF << 24) | (red << 16 | green << 8 | blue);
	}
	return screen->black_pixel;
}

/* ------------------------------------------------------------------ */
/*  Enumerate windows                                                 */
/* ------------------------------------------------------------------ */

void backend_enumerate_windows(backend_window_visitor_t visitor)
{
	xcb_query_tree_reply_t *qtr = xcb_query_tree_reply(dpy,
		xcb_query_tree(dpy, root), NULL);
	if (!qtr) return;
	int len = xcb_query_tree_children_length(qtr);
	xcb_window_t *children = xcb_query_tree_children(qtr);
	for (int i = 0; i < len; i++) {
		visitor(children[i]);
	}
	free(qtr);
}

/* ------------------------------------------------------------------ */
/*  Display name                                                      */
/* ------------------------------------------------------------------ */

bool backend_parse_display(char **host, int *display_num, int *screen_num)
{
	return xcb_parse_display(NULL, host, display_num, screen_num) != 0;
}

bool backend_register_root_events(void)
{
	uint32_t values[] = {
		XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
		XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
		XCB_EVENT_MASK_STRUCTURE_NOTIFY |
		XCB_EVENT_MASK_BUTTON_PRESS |
		XCB_EVENT_MASK_FOCUS_CHANGE
	};
	xcb_generic_error_t *e = xcb_request_check(dpy,
		xcb_change_window_attributes_checked(dpy, root, XCB_CW_EVENT_MASK, values));
	if (e) {
		free(e);
		return false;
	}
	return true;
}

/* ------------------------------------------------------------------ */
/*  Presel feedback                                                   */
/* ------------------------------------------------------------------ */

bspwm_wid_t backend_create_presel_feedback(uint32_t color)
{
	bspwm_wid_t win = xcb_generate_id(dpy);
	uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_SAVE_UNDER;
	uint32_t values[] = {color, 1};
	xcb_create_window(dpy, XCB_COPY_FROM_PARENT, win, root, 0, 0, 1, 1, 0,
	                  XCB_WINDOW_CLASS_INPUT_OUTPUT, XCB_COPY_FROM_PARENT, mask, values);
	xcb_icccm_set_wm_class(dpy, win, sizeof("presel_feedback\0Bspwm"), "presel_feedback\0Bspwm");
	xcb_shape_rectangles(dpy, XCB_SHAPE_SO_SET, XCB_SHAPE_SK_INPUT,
	                     XCB_CLIP_ORDERING_UNSORTED, win, 0, 0, 0, NULL);
	return win;
}

/* ------------------------------------------------------------------ */
/*  Atom helpers                                                      */
/* ------------------------------------------------------------------ */

static void get_atom(char *name, xcb_atom_t *atom)
{
	xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(dpy,
		xcb_intern_atom(dpy, 0, strlen(name), name), NULL);
	if (reply) {
		*atom = reply->atom;
		free(reply);
	} else {
		/* Server disconnect or atom quota: leave callers with a defined
		 * value instead of stack garbage. */
		*atom = XCB_ATOM_NONE;
	}
}

void backend_set_atom(bspwm_wid_t win, const char *atom_name, uint32_t value)
{
	xcb_atom_t atom;
	get_atom((char *)atom_name, &atom);
	xcb_change_property(dpy, XCB_PROP_MODE_REPLACE, win, atom, XCB_ATOM_CARDINAL, 32, 1, &value);
}

/* ------------------------------------------------------------------ */
/*  X11-specific setup helpers (called from bspwm.c)                  */
/* ------------------------------------------------------------------ */

void x11_setup_screen(void)
{
	screen = xcb_setup_roots_iterator(xcb_get_setup(dpy)).data;
	/* A NULL screen (incomplete/!ready X setup) otherwise segfaults
	 * later in backend_get_root() and every screen->* access. Fail
	 * cleanly with a diagnosable message instead. */
	if (screen == NULL) {
		err("Can't acquire the X screen.\n");
	}
}

void x11_setup_atoms(void)
{
	get_atom("WM_STATE", &WM_STATE);
	get_atom("WM_DELETE_WINDOW", &WM_DELETE_WINDOW);
	get_atom("WM_TAKE_FOCUS", &WM_TAKE_FOCUS);
	get_atom("WM_WINDOW_ROLE", &WM_WINDOW_ROLE);
}

void x11_setup_randr(void)
{
	const xcb_query_extension_reply_t *qep = xcb_get_extension_data(dpy, &xcb_randr_id);
	if (qep->present) {
		randr = true;
		randr_base = qep->first_event;
		backend_listen_output_changes();
	}
}

bool x11_try_xinerama(void)
{
	if (!xcb_get_extension_data(dpy, &xcb_xinerama_id)->present)
		return false;

	xcb_xinerama_is_active_reply_t *xia = xcb_xinerama_is_active_reply(dpy,
		xcb_xinerama_is_active(dpy), NULL);
	if (!xia || !xia->state) {
		free(xia);
		return false;
	}
	free(xia);

	xcb_xinerama_query_screens_reply_t *xsq = xcb_xinerama_query_screens_reply(dpy,
		xcb_xinerama_query_screens(dpy), NULL);
	if (!xsq) return false;

	xcb_xinerama_screen_info_t *xsi = xcb_xinerama_query_screens_screen_info(xsq);
	int n = xcb_xinerama_query_screens_screen_info_length(xsq);
	for (int i = 0; i < n; i++) {
		bspwm_output_info_t out = {0};
		out.rect = (bspwm_rect_t){xsi[i].x_org, xsi[i].y_org, xsi[i].width, xsi[i].height};
		out.id = i + 1;
		snprintf(out.name, sizeof(out.name), "XINERAMA-%d", i);
		/* Use update_monitors path indirectly via make_monitor + add_monitor */
	}
	free(xsq);
	return true;
}

/* ------------------------------------------------------------------ */
/*  Keybinding grabs                                                   */
/* ------------------------------------------------------------------ */

void backend_ungrab_keys(void)
{
	xcb_ungrab_key(dpy, XCB_GRAB_ANY, root, XCB_MOD_MASK_ANY);
}

void backend_grab_keys(void)
{
	backend_ungrab_keys();

	if (keybind_table.count == 0)
		return;

	/* TODO: cache the modifier mapping (currently fetched twice per call via
	 * xcb_get_modifier_mapping_reply for Num_Lock and Scroll_Lock lookups)
	 * and the xcb_key_symbols_t allocation, refreshing only on
	 * XCB_MAPPING_NOTIFY, similarly to the previous key_symbols caching
	 * fix. Avoids two server round-trips on every keybind reload. */
	xcb_key_symbols_t *syms = xcb_key_symbols_alloc(dpy);
	if (!syms)
		return;

	/* Lock mask combinations to grab through (NumLock, CapsLock, both, neither) */
	uint16_t num_lock = 0, caps_lock = 0, scroll_lock = 0;
	{
		xcb_keycode_t *kc;
		kc = xcb_key_symbols_get_keycode(syms, XK_Num_Lock);
		if (kc) {
			/* Find which modifier Num_Lock is mapped to */
			xcb_get_modifier_mapping_reply_t *mmr =
				xcb_get_modifier_mapping_reply(dpy, xcb_get_modifier_mapping(dpy), NULL);
			if (mmr) {
				xcb_keycode_t *modmap = xcb_get_modifier_mapping_keycodes(mmr);
				int kpk = mmr->keycodes_per_modifier;
				for (int i = 0; i < 8; i++) {
					for (int j = 0; j < kpk; j++) {
						if (modmap[i * kpk + j] == *kc)
							num_lock = (uint16_t)(1 << i);
					}
				}
				free(mmr);
			}
			free(kc);
		}
		caps_lock = XCB_MOD_MASK_LOCK;
		kc = xcb_key_symbols_get_keycode(syms, XK_Scroll_Lock);
		if (kc) {
			xcb_get_modifier_mapping_reply_t *mmr =
				xcb_get_modifier_mapping_reply(dpy, xcb_get_modifier_mapping(dpy), NULL);
			if (mmr) {
				xcb_keycode_t *modmap = xcb_get_modifier_mapping_keycodes(mmr);
				int kpk = mmr->keycodes_per_modifier;
				for (int i = 0; i < 8; i++) {
					for (int j = 0; j < kpk; j++) {
						if (modmap[i * kpk + j] == *kc)
							scroll_lock = (uint16_t)(1 << i);
					}
				}
				free(mmr);
			}
			free(kc);
		}
	}

	/* Any lock that isn't mapped comes back as 0, which collapses several of
	 * these combinations onto each other. Grabbing the same (key, modifier)
	 * twice is wasted round-trips, so keep only the distinct masks. */
	uint16_t all_masks[] = {
		0,
		num_lock,
		caps_lock,
		scroll_lock,
		num_lock | caps_lock,
		num_lock | scroll_lock,
		caps_lock | scroll_lock,
		num_lock | caps_lock | scroll_lock,
	};
	uint16_t lock_masks[LENGTH(all_masks)];
	size_t lock_mask_count = 0;
	for (size_t i = 0; i < LENGTH(all_masks); i++) {
		bool seen = false;
		for (size_t j = 0; j < lock_mask_count; j++) {
			if (lock_masks[j] == all_masks[i]) {
				seen = true;
				break;
			}
		}
		if (!seen) {
			lock_masks[lock_mask_count++] = all_masks[i];
		}
	}

	/* KBMOD_* flags map directly to X11 modifier bits */
	for (int i = 0; i < keybind_table.count; i++) {
		keybind_t *kb = &keybind_table.binds[i];

		xcb_keycode_t *keycodes = xcb_key_symbols_get_keycode(syms, kb->keysym);
		if (!keycodes)
			continue;

		uint16_t modfield = (uint16_t)kb->modifiers;

		for (xcb_keycode_t *kc = keycodes; *kc != XCB_NO_SYMBOL; kc++) {
			for (size_t m = 0; m < lock_mask_count; m++) {
				xcb_grab_key(dpy, 1, root, modfield | lock_masks[m],
				             *kc, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
			}
		}

		free(keycodes);
	}

	xcb_key_symbols_free(syms);
	xcb_flush(dpy);
}

void x11_setup_ewmh_supported(void)
{
	xcb_atom_t net_atoms[] = {
		ewmh->_NET_SUPPORTED,
		ewmh->_NET_SUPPORTING_WM_CHECK,
		ewmh->_NET_DESKTOP_NAMES,
		ewmh->_NET_DESKTOP_VIEWPORT,
		ewmh->_NET_NUMBER_OF_DESKTOPS,
		ewmh->_NET_CURRENT_DESKTOP,
		ewmh->_NET_CLIENT_LIST,
		ewmh->_NET_ACTIVE_WINDOW,
		ewmh->_NET_CLOSE_WINDOW,
		ewmh->_NET_WM_STRUT_PARTIAL,
		ewmh->_NET_WM_DESKTOP,
		ewmh->_NET_WM_STATE,
		ewmh->_NET_WM_STATE_HIDDEN,
		ewmh->_NET_WM_STATE_FULLSCREEN,
		ewmh->_NET_WM_STATE_BELOW,
		ewmh->_NET_WM_STATE_ABOVE,
		ewmh->_NET_WM_STATE_STICKY,
		ewmh->_NET_WM_STATE_DEMANDS_ATTENTION,
		ewmh->_NET_WM_WINDOW_TYPE,
		ewmh->_NET_WM_WINDOW_TYPE_DOCK,
		ewmh->_NET_WM_WINDOW_TYPE_DESKTOP,
		ewmh->_NET_WM_WINDOW_TYPE_NOTIFICATION,
		ewmh->_NET_WM_WINDOW_TYPE_DIALOG,
		ewmh->_NET_WM_WINDOW_TYPE_UTILITY,
		ewmh->_NET_WM_WINDOW_TYPE_TOOLBAR
	};
	xcb_ewmh_set_supported(ewmh, default_screen,
		sizeof(net_atoms) / sizeof(net_atoms[0]), net_atoms);
}
