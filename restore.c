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

bool restore_monitor(json_t *json)
{
	monitor_t *m, *md;
	json_t *jbool;

	if ((md = json_deserialize_monitor_type(json)) == NULL) {
		return false;
	}
	if ((m = find_monitor(md->name)) == NULL) {
		warn("Could not find monitor: %s\n", md->name);
		free(md);
		return false;
	}

	*m = *md;
	free(md);

	if ((jbool = json_object_get(json, "focused")) != NULL && json_is_true(jbool)) {
		mon = m;
	}

	return true;
}

bool restore_desktop(json_t *json)
{
	desktop_t *dd;
	coordinates_t loc;

	if ((dd = json_deserialize_desktop_type(json)) == NULL) {
		warn("Failed to deserialize desktop\n");
		return false;
	}

	locate_desktop(dd->name, &loc);

	if (loc.desktop == NULL) {
		warn("Failed to find desktop: %s\n", dd->name);
		free(dd);
		return false;
	}

	*loc.desktop = *dd;
	free(dd);

	return true;
}

void restore_tree(const char *file_path)
{
	if (file_path == NULL)
		return;

	json_t *json = json_deserialize_file(file_path);
	if (json == NULL || !json_is_object(json)) {
		warn("File is not a JSON tree");
		return;
	}

	size_t dindex;
	const char *mkey;
	json_t *mvalue, *dvalue, *jdesktops;

	json_object_foreach(json, mkey, mvalue) {
		if (!restore_monitor(mvalue))
			warn("Failed to restore monitor: %s\n", mkey);
			continue;

		// if ((jdesktops = json_object_get(mvalue, "desktops")) == NULL || !json_is_array(jdesktops)) {
		// 	warn("Key not found: desktops\n");
		// 	continue;
		// }
		// json_array_foreach(jdesktops, dindex, dvalue) {
		// 	if (!restore_desktop(dvalue))
		// 		warn("Failed to restore desktop at index: %u\n", dindex);
		// 		continue;
		// }
	}
	json_decref(json);

	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
			for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root)) {
				uint32_t values[] = {CLIENT_EVENT_MASK | (focus_follows_pointer ? XCB_EVENT_MASK_ENTER_WINDOW : 0)};
				xcb_change_window_attributes(dpy, n->client->window, XCB_CW_EVENT_MASK, values);
				if (!IS_TILED(n->client)) {
					n->vacant = true;
					update_vacant_state(n->parent);
				}
				if (n->client->private) {
					update_privacy_level(n, true);
				}
			}
			/* Has the side effect of restoring the node's rectangles and the client's tiled rectangles */
			arrange(m, d);
		}
	}

	ewmh_update_current_desktop();
}

void restore_history(const char *file_path)
{
	if (file_path == NULL)
		return;

	json_t *json = json_deserialize_file(file_path);
	if (json == NULL || !json_is_array(json)) {
		warn("File is not a JSON history");
		return;
	}

	size_t index;
	json_t *value;
	coordinates_t *loc;

	json_array_foreach(json, index, value) {
		if ((loc = json_deserialize_coordinates_type(value)) == NULL) {
			free(loc);
			continue;
		}
		history_add(loc->monitor, loc->desktop, loc->node);
	}
	json_decref(json);
}

void restore_stack(const char *file_path)
{
	if (file_path == NULL)
		return;

	json_t *json = json_deserialize_file(file_path);
	if (json == NULL || !json_is_array(json)) {
		warn("File is not a JSON stack");
		return;
	}

	size_t index;
	json_t *value;
	node_t *n;

	json_array_foreach(json, index, value) {
		if ((n = json_deserialize_node_windowid(value)) == NULL) {
			free(n);
			continue;
		}
		stack_insert_after(stack_tail, n);
	}
	json_decref(json);
}
