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

void restore_tree(const char *file_path)
{
	if (file_path == NULL)
		return;

	FILE *snapshot = fopen(file_path, "r");
	if (snapshot == NULL) {
		warn("Restore tree: can't open '%s'.\n", file_path);
		return;
	}

	char line[MAXLEN];
	char name[MAXLEN];
	coordinates_t loc;
	monitor_t *m = NULL;
	desktop_t *d = NULL;
	node_t *n = NULL;
	unsigned int level, last_level = 0;

	while (fgets(line, sizeof(line), snapshot) != NULL) {
		unsigned int len = strlen(line);
		level = 0;

		while (level < len && isspace(line[level]))
			level++;

		if (level == 0) {
			int x, y, top, right, bottom, left;
			unsigned int w, h;
			char end = 0;
			name[0] = '\0';
			sscanf(line + level, "%s %ux%u%i%i %i,%i,%i,%i %c", name, &w, &h, &x, &y,
			       &top, &right, &bottom, &left, &end);
			m = find_monitor(name);
			if (m == NULL)
				continue;
			m->rectangle = (xcb_rectangle_t) {x, y, w, h};
			m->top_padding = top;
			m->right_padding = right;
			m->bottom_padding = bottom;
			m->left_padding = left;
			if (end != 0)
				mon = m;
		} else if (level == 1) {
			if (m == NULL)
				continue;
			int wg, top, right, bottom, left;
			unsigned int bw;
			char layout = 0, end = 0;
			name[0] = '\0';
			loc.desktop = NULL;
			sscanf(line + level, "%s %u %i %i,%i,%i,%i %c %c", name,
			       &bw, &wg, &top, &right, &bottom, &left, &layout, &end);
			locate_desktop(name, &loc);
			d = loc.desktop;
			if (d == NULL) {
				continue;
			}
			d->border_width = bw;
			d->window_gap = wg;
			d->top_padding = top;
			d->right_padding = right;
			d->bottom_padding = bottom;
			d->left_padding = left;
			if (layout == 'M') {
				d->layout = LAYOUT_MONOCLE;
			} else if (layout == 'T') {
				d->layout = LAYOUT_TILED;
			}
			if (end != 0) {
				m->desk = d;
			}
		} else {
			if (m == NULL || d == NULL)
				continue;
			node_t *birth = make_node();
			if (level == 2) {
				empty_desktop(d);
				d->root = birth;
			} else if (n != NULL) {
				if (level > last_level) {
					n->first_child = birth;
				} else {
					do {
						n = n->parent;
					} while (n != NULL && n->second_child != NULL);
					if (n == NULL)
						continue;
					n->second_child = birth;
				}
				birth->parent = n;
			}
			n = birth;
			char birth_rotation;
			if (isupper(line[level])) {
				char split_type;
				sscanf(line + level, "%c %c %lf", &split_type, &birth_rotation, &n->split_ratio);
				if (split_type == 'H') {
					n->split_type = TYPE_HORIZONTAL;
				} else if (split_type == 'V') {
					n->split_type = TYPE_VERTICAL;
				}
			} else {
				client_t *c = make_client(XCB_NONE, d->border_width);
				num_clients++;
				char urgent, locked, sticky, private, split_dir, split_mode, state, layer, end = 0;
				sscanf(line + level, "%c %s %s %X %u %hux%hu%hi%hi %c%c %c%c %c%c%c%c %c", &birth_rotation,
				       c->class_name, c->instance_name, &c->window, &c->border_width,
				       &c->floating_rectangle.width, &c->floating_rectangle.height,
				       &c->floating_rectangle.x, &c->floating_rectangle.y,
				       &split_dir, &split_mode, &state, &layer,
				       &urgent, &locked, &sticky, &private, &end);
				n->split_mode = (split_mode == '-' ? MODE_AUTOMATIC : MODE_MANUAL);
				if (split_dir == 'U') {
					n->split_dir = DIR_UP;
				} else if (split_dir == 'R') {
					n->split_dir = DIR_RIGHT;
				} else if (split_dir == 'D') {
					n->split_dir = DIR_DOWN;
				} else if (split_dir == 'L') {
					n->split_dir = DIR_LEFT;
				}
				if (state == 'f') {
					c->state = STATE_FLOATING;
				} else if (state == 'F') {
					c->state = STATE_FULLSCREEN;
				} else if (state == 'p') {
					c->state = STATE_PSEUDO_TILED;
				}
				if (layer == 'b') {
					c->layer = LAYER_BELOW;
				} else if (layer == 'a') {
					c->layer = LAYER_ABOVE;
				}
				c->urgent = (urgent == '-' ? false : true);
				c->locked = (locked == '-' ? false : true);
				c->sticky = (sticky == '-' ? false : true);
				c->private = (private == '-' ? false : true);
				n->client = c;
				if (end != 0) {
					d->focus = n;
				}
				if (c->sticky) {
					m->num_sticky++;
				}
			}
			if (birth_rotation == 'a') {
				n->birth_rotation = 90;
			} else if (birth_rotation == 'c') {
				n->birth_rotation = 270;
			} else if (birth_rotation == 'm') {
				n->birth_rotation = 0;
			}
		}
		last_level = level;
	}

	fclose(snapshot);

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
