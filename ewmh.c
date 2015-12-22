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

void ewmh_init(void)
{
	ewmh = malloc(sizeof(xcb_ewmh_connection_t));
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
		xcb_ewmh_set_wm_desktop(ewmh, f->id, i);
	}
}

void ewmh_update_wm_desktops(void)
{
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			uint32_t i = ewmh_get_desktop_index(d);
			for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root)) {
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
					wins[i++] = n->id;
				}
			}
		}
		xcb_ewmh_set_client_list(ewmh, default_screen, clients_count, wins);
	}
}

bool ewmh_wm_state_add(node_t *n, xcb_atom_t state)
{
	client_t *c = n->client;

	if (c == NULL || c->wm_states_count >= MAX_WM_STATES) {
		return false;
	}

	for (int i = 0; i < c->wm_states_count; i++) {
		if (c->wm_state[i] == state) {
			return false;
		}
	}

	c->wm_state[c->wm_states_count] = state;
	c->wm_states_count++;
	xcb_ewmh_set_wm_state(ewmh, n->id, c->wm_states_count, c->wm_state);
	return true;
}

bool ewmh_wm_state_remove(node_t *n, xcb_atom_t state)
{
	client_t *c = n->client;
	if (c == NULL) {
		return false;
	}
	for (int i = 0; i < c->wm_states_count; i++) {
		if (c->wm_state[i] == state) {
			for (int j = i; j < (c->wm_states_count - 1); j++) {
				c->wm_state[j] = c->wm_state[j + 1];
			}
			c->wm_states_count--;
			xcb_ewmh_set_wm_state(ewmh, n->id, c->wm_states_count, c->wm_state);
			return true;
		}
	}
	return false;
}

void ewmh_set_supporting(xcb_window_t win)
{
	pid_t wm_pid = getpid();
	xcb_ewmh_set_supporting_wm_check(ewmh, root, win);
	xcb_ewmh_set_supporting_wm_check(ewmh, win, win);
	xcb_ewmh_set_wm_name(ewmh, win, strlen(WM_NAME), WM_NAME);
	xcb_ewmh_set_wm_pid(ewmh, win, wm_pid);
}
