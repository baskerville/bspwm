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

#include <ctype.h>
#include <string.h>
#include <jansson.h>
#include "bspwm.h"
#include "desktop.h"
#include "ewmh.h"
#include "history.h"
#include "monitor.h"
#include "query.h"
#include "stack.h"
#include "tree.h"
#include "settings.h"
#include "restore.h"
#include "json.h"
#include "window.h"

void restore_client(client_t *cs, client_t *cd)
{
	cd->border_width = cs->border_width;
	cd->locked = cs->locked;
	cd->sticky = cs->sticky;
	cd->urgent = cs->urgent;
	cd->private = cs->private;
	cd->state = cs->state;
	cd->last_state = cs->last_state;
	cd->layer = cs->layer;
	cd->last_layer = cs->last_layer;
	cd->floating_rectangle = cs->floating_rectangle;
	cd->tiled_rectangle = cs->tiled_rectangle;
}

void restore_node(node_t *ns, monitor_t *md, desktop_t *dd, node_t *nd)
{
	nd->split_type = ns->split_type;
	nd->split_ratio = ns->split_ratio;
	nd->split_mode = ns->split_mode;
	nd->split_dir = ns->split_dir;
	nd->birth_rotation = ns->birth_rotation;
	nd->rectangle = ns->rectangle;

	if (ns->first_child) {
		nd->first_child = make_node();
		nd->first_child->parent = nd;
		restore_node(ns->first_child, md, dd, nd->first_child);
	}
	if (ns->second_child) {
		nd->second_child = make_node();
		nd->second_child->parent = nd;
		restore_node(ns->second_child, md, dd, nd->second_child);
	}

	if (ns->client) {
		coordinates_t loc;
		locate_window(ns->client->window, &loc);
		if (loc.node) {
			unlink_node(loc.monitor, loc.desktop, loc.node);
			nd->client = loc.node->client;
			history_replace_node(loc.node, md, dd, nd);
			stack_replace_node(loc.node, nd);
			loc.node->client = NULL;
			ewmh_set_wm_desktop(nd, dd);
			if (dd->focus == loc.node)
				dd->focus = nd;
			destroy_tree(loc.node);
			restore_client(ns->client, nd->client);
			if (nd->client->sticky) {
				window_show(nd->client->window);
			} else {
				if (dd == md->desk) {
					window_show(nd->client->window);
				} else {
					window_hide(nd->client->window);
				}
			}
		}
		free(ns->client);
	}

	free(ns);
}

bool restore_desktop(json_t *json)
{
	desktop_t *ds = json_deserialize_desktop_type(json);
	if (!ds)
		return false;

	coordinates_t loc;
	locate_desktop(ds->name, &loc);
	if (!loc.desktop) {
		warn("Failed to find desktop: %s\n", ds->name);
		free(ds);
		return false;
	}

	desktop_t *dd = make_desktop(NULL);

	dd->layout = ds->layout;
	dd->top_padding = ds->top_padding;
	dd->right_padding = ds->right_padding;
	dd->bottom_padding = ds->bottom_padding;
	dd->left_padding = ds->left_padding;
	dd->window_gap = ds->window_gap;
	dd->border_width = ds->border_width;
	dd->focus = ds->focus;

	if (ds->root) {
		dd->root = make_node();
		restore_node(ds->root, loc.monitor, dd, dd->root);
	}

	add_desktop(loc.monitor, dd);
	merge_desktops(loc.monitor, loc.desktop, loc.monitor, dd);
	swap_desktops(loc.monitor, loc.desktop, loc.monitor, dd);
	if (mon->desk == loc.desktop)
		focus_desktop(loc.monitor, dd);
	remove_desktop(loc.monitor, loc.desktop);
	rename_desktop(loc.monitor, dd, ds->name);

	free(ds);

	return true;
}

bool restore_monitor(json_t *json)
{
	monitor_t *ms = json_deserialize_monitor_type(json);
	if (!ms)
		return false;

	monitor_t *md = find_monitor(ms->name);
	if (!md) {
		warn("Could not find monitor: %s\n", ms->name);
		free(ms);
		return false;
	}

	md->top_padding = ms->top_padding;
	md->right_padding = ms->right_padding;
	md->bottom_padding = ms->bottom_padding;
	md->left_padding = ms->left_padding;

	free(ms);

	if (json_is_true(json_object_get(json, "focused")))
		focus_monitor(md);

	return true;
}

void restore_tree(const char *file_path)
{
	if (!file_path)
		return;

	json_t *json = json_deserialize_file(file_path);
	if (!json_is_object(json)) {
		warn("File is not a JSON tree");
		return;
	}

	size_t dindex;
	const char *mkey;
	json_t *mvalue, *dvalue, *jdesktops;

	json_object_foreach(json, mkey, mvalue) {
		if (!restore_monitor(mvalue)) {
			warn("Failed to restore monitor: %s\n", mkey);
			continue;
		}

		jdesktops = json_object_get(mvalue, "desktops");
		if (!json_is_array(jdesktops)) {
			warn("Key not found: desktops\n");
			continue;
		}
		json_array_foreach(jdesktops, dindex, dvalue) {
			if (!restore_desktop(dvalue)) {
				warn("Failed to restore desktop at index: %u\n", dindex);
				continue;
			}
		}
		json_array_foreach(jdesktops, dindex, dvalue) {
			if (!restore_desktop(dvalue)) {
				warn("Failed to restore desktop at index: %u\n", dindex);
				continue;
			}
		}

		const char *desk_name = json_string_value(json_object_get(mvalue, "deskName"));
		if (desk_name) {
			coordinates_t loc;
			locate_desktop(desk_name, &loc);
			if (loc.desktop)
				focus_desktop(loc.monitor, loc.desktop);
		}
	}
	json_decref(json);

	unsigned int num_sticky;
	for (monitor_t *m = mon_head; m; m = m->next) {
		num_sticky = 0;
		for (desktop_t *d = m->desk_head; d; d = d->next) {
			for (node_t *n = first_extrema(d->root); n; n = next_leaf(n, d->root)) {
				uint32_t values[] = {CLIENT_EVENT_MASK | (focus_follows_pointer ? XCB_EVENT_MASK_ENTER_WINDOW : 0)};
				xcb_change_window_attributes(dpy, n->client->window, XCB_CW_EVENT_MASK, values);
				if (!IS_TILED(n->client)) {
					n->vacant = true;
					update_vacant_state(n->parent);
				}
				if (n->client->private) {
					update_privacy_level(n, true);
				}
				if (n->client->sticky) {
					++num_sticky;
				}
			}
			/* Has the side effect of restoring the node's rectangles and the client's tiled rectangles */
			arrange(m, d);
		}
		m->num_sticky = num_sticky;
	}

	ewmh_update_current_desktop();
}

void restore_history(const char *file_path)
{
	if (!file_path)
		return;

	json_t *json = json_deserialize_file(file_path);
	if (!json_is_array(json)) {
		warn("File is not a JSON history");
		return;
	}

	size_t index;
	json_t *value;
	coordinates_t *loc;

	json_array_foreach(json, index, value) {
		loc = json_deserialize_coordinates_type(value);
		if (!loc)
			continue;
		history_add(loc->monitor, loc->desktop, loc->node);
		free(loc);
	}
	json_decref(json);
}

void restore_stack(const char *file_path)
{
	if (!file_path)
		return;

	json_t *json = json_deserialize_file(file_path);
	if (!json_is_array(json)) {
		warn("File is not a JSON stack");
		return;
	}

	size_t index;
	json_t *value;
	node_t *n;

	json_array_foreach(json, index, value) {
		n = json_deserialize_node_window(value);
		if (!n)
			continue;
		stack_insert_after(stack_tail, n);
	}
	json_decref(json);
}
