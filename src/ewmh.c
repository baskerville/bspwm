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
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include "bspwm.h"
#include "settings.h"
#include "tree.h"
#include "ewmh.h"

xcb_ewmh_connection_t *ewmh;

void ewmh_init(void)
{
	ewmh = calloc(1, sizeof(xcb_ewmh_connection_t));
	if (xcb_ewmh_init_atoms_replies(ewmh, xcb_ewmh_init_atoms(dpy, ewmh), NULL) == 0) {
		err("Can't initialize EWMH atoms.\n");
	}
}

void ewmh_update_active_window(void)
{
	xcb_window_t win = ((mon->desk->focus == NULL || mon->desk->focus->client == NULL) ? XCB_NONE : mon->desk->focus->id);
	xcb_ewmh_set_active_window(ewmh, default_screen, win);
}

void ewmh_update_number_of_desktops(void)
{
	uint32_t desktops_count = 0;

	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			desktops_count++;
		}
	}

	xcb_ewmh_set_number_of_desktops(ewmh, default_screen, desktops_count);
}

uint32_t ewmh_get_desktop_index(desktop_t *d)
{
	uint32_t i = 0;
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		for (desktop_t *cd = m->desk_head; cd != NULL; cd = cd->next, i++) {
			if (d == cd) {
				return i;
			}
		}
	}
	return 0;
}

bool ewmh_locate_desktop(uint32_t i, coordinates_t *loc)
{
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next, i--) {
			if (i == 0) {
				loc->monitor = m;
				loc->desktop = d;
				loc->node = NULL;
				return true;
			}
		}
	}
	return false;
}

void ewmh_update_current_desktop(void)
{
	if (mon == NULL) {
		return;
	}
	uint32_t i = ewmh_get_desktop_index(mon->desk);
	xcb_ewmh_set_current_desktop(ewmh, default_screen, i);
}

void ewmh_set_wm_desktop(node_t *n, desktop_t *d)
{
	uint32_t i = ewmh_get_desktop_index(d);
	for (node_t *f = first_extrema(n); f != NULL; f = next_leaf(f, n)) {
		if (f->client == NULL) {
			continue;
		}
		xcb_ewmh_set_wm_desktop(ewmh, f->id, i);
	}
}

void ewmh_update_wm_desktops(void)
{
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			uint32_t i = ewmh_get_desktop_index(d);
			for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root)) {
				if (n->client == NULL) {
					continue;
				}
				xcb_ewmh_set_wm_desktop(ewmh, n->id, i);
			}
		}
	}
}

void ewmh_update_desktop_names(void)
{
	char names[MAXLEN];
	unsigned int i, j;
	uint32_t names_len;
	i = 0;

	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			for (j = 0; d->name[j] != '\0' && (i + j) < sizeof(names); j++) {
				names[i + j] = d->name[j];
			}
			i += j;
			if (i < sizeof(names)) {
				names[i++] = '\0';
			}
		}
	}

	if (i < 1) {
		xcb_ewmh_set_desktop_names(ewmh, default_screen, 0, NULL);
		return;
	}

	names_len = i - 1;
	xcb_ewmh_set_desktop_names(ewmh, default_screen, names_len, names);
}

void ewmh_update_desktop_viewport(void)
{
	uint32_t desktops_count = 0;
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			desktops_count++;
		}
	}
	if (desktops_count == 0) {
		xcb_ewmh_set_desktop_viewport(ewmh, default_screen, 0, NULL);
		return;
	}
	xcb_ewmh_coordinates_t coords[desktops_count];
	uint16_t desktop = 0;
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			coords[desktop++] = (xcb_ewmh_coordinates_t){m->rectangle.x, m->rectangle.y};
		}
	}
	xcb_ewmh_set_desktop_viewport(ewmh, default_screen, desktop, coords);
}

bool ewmh_handle_struts(xcb_window_t win)
{
	xcb_ewmh_wm_strut_partial_t struts;
	bool changed = false;
	if (xcb_ewmh_get_wm_strut_partial_reply(ewmh, xcb_ewmh_get_wm_strut_partial(ewmh, win), &struts, NULL) == 1) {
		for (monitor_t *m = mon_head; m != NULL; m = m->next) {
			xcb_rectangle_t rect = m->rectangle;
			if (rect.x < (int16_t) struts.left &&
			    (int16_t) struts.left < (rect.x + rect.width - 1) &&
			    (int16_t) struts.left_end_y >= rect.y &&
			    (int16_t) struts.left_start_y < (rect.y + rect.height)) {
				int dx = struts.left - rect.x;
				if (m->padding.left < 0) {
					m->padding.left += dx;
				} else {
					m->padding.left = MAX(dx, m->padding.left);
				}
				changed = true;
			}
			if ((rect.x + rect.width) > (int16_t) (screen_width - struts.right) &&
			    (int16_t) (screen_width - struts.right) > rect.x &&
			    (int16_t) struts.right_end_y >= rect.y &&
			    (int16_t) struts.right_start_y < (rect.y + rect.height)) {
				int dx = (rect.x + rect.width) - screen_width + struts.right;
				if (m->padding.right < 0) {
					m->padding.right += dx;
				} else {
					m->padding.right = MAX(dx, m->padding.right);
				}
				changed = true;
			}
			if (rect.y < (int16_t) struts.top &&
			    (int16_t) struts.top < (rect.y + rect.height - 1) &&
			    (int16_t) struts.top_end_x >= rect.x &&
			    (int16_t) struts.top_start_x < (rect.x + rect.width)) {
				int dy = struts.top - rect.y;
				if (m->padding.top < 0) {
					m->padding.top += dy;
				} else {
					m->padding.top = MAX(dy, m->padding.top);
				}
				changed = true;
			}
			if ((rect.y + rect.height) > (int16_t) (screen_height - struts.bottom) &&
			    (int16_t) (screen_height - struts.bottom) > rect.y &&
			    (int16_t) struts.bottom_end_x >= rect.x &&
			    (int16_t) struts.bottom_start_x < (rect.x + rect.width)) {
				int dy = (rect.y + rect.height) - screen_height + struts.bottom;
				if (m->padding.bottom < 0) {
					m->padding.bottom += dy;
				} else {
					m->padding.bottom = MAX(dy, m->padding.bottom);
				}
				changed = true;
			}
		}
	}
	return changed;
}

void ewmh_update_client_list(bool stacking)
{
	if (clients_count == 0) {
		xcb_ewmh_set_client_list(ewmh, default_screen, 0, NULL);
		xcb_ewmh_set_client_list_stacking(ewmh, default_screen, 0, NULL);
		return;
	}

	xcb_window_t wins[clients_count];
	unsigned int i = 0;

	if (stacking) {
		for (stacking_list_t *s = stack_head; s != NULL; s = s->next) {
			wins[i++] = s->node->id;
		}
		xcb_ewmh_set_client_list_stacking(ewmh, default_screen, clients_count, wins);
	} else {
		for (monitor_t *m = mon_head; m != NULL; m = m->next) {
			for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
				for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root)) {
					if (n->client == NULL) {
						continue;
					}
					wins[i++] = n->id;
				}
			}
		}
		xcb_ewmh_set_client_list(ewmh, default_screen, clients_count, wins);
	}
}

void ewmh_wm_state_update(node_t *n)
{
	client_t *c = n->client;
	size_t count = 0;
	uint32_t values[12];
#define HANDLE_WM_STATE(s)  \
	if (WM_FLAG_##s & c->wm_flags) { \
		values[count++] = ewmh->_NET_WM_STATE_##s; \
	}
	HANDLE_WM_STATE(MODAL)
	HANDLE_WM_STATE(STICKY)
	HANDLE_WM_STATE(MAXIMIZED_VERT)
	HANDLE_WM_STATE(MAXIMIZED_HORZ)
	HANDLE_WM_STATE(SHADED)
	HANDLE_WM_STATE(SKIP_TASKBAR)
	HANDLE_WM_STATE(SKIP_PAGER)
	HANDLE_WM_STATE(HIDDEN)
	HANDLE_WM_STATE(FULLSCREEN)
	HANDLE_WM_STATE(ABOVE)
	HANDLE_WM_STATE(BELOW)
	HANDLE_WM_STATE(DEMANDS_ATTENTION)
#undef HANDLE_WM_STATE
	xcb_ewmh_set_wm_state(ewmh, n->id, count, values);
}

void ewmh_set_supporting(xcb_window_t win)
{
	pid_t wm_pid = getpid();
	xcb_ewmh_set_supporting_wm_check(ewmh, root, win);
	xcb_ewmh_set_supporting_wm_check(ewmh, win, win);
	xcb_ewmh_set_wm_name(ewmh, win, strlen(WM_NAME), WM_NAME);
	xcb_ewmh_set_wm_pid(ewmh, win, wm_pid);
}
