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

#ifndef BSPWM_TREE_H
#define BSPWM_TREE_H

void arrange(monitor_t *m, desktop_t *d);
void apply_layout(monitor_t *m, desktop_t *d, node_t *n, xcb_rectangle_t rect, xcb_rectangle_t root_rect);
presel_t *make_presel(void);
void presel_dir(monitor_t *m, desktop_t *d, node_t *n, direction_t dir);
void presel_ratio(monitor_t *m, desktop_t *d, node_t *n, double ratio);
void cancel_presel(monitor_t *m, desktop_t *d, node_t *n);
node_t *find_public(desktop_t *d);
node_t *insert_node(monitor_t *m, desktop_t *d, node_t *n, node_t *f);
void activate_node(monitor_t *m, desktop_t *d, node_t *n);
void transfer_sticky_nodes(monitor_t *m, desktop_t *ds, desktop_t *dd, node_t *n);
void focus_node(monitor_t *m, desktop_t *d, node_t *n);
void update_focused(void);
void hide_node(node_t *n);
void show_node(node_t *n);
node_t *make_node(uint32_t id);
client_t *make_client(void);
void initialize_client(node_t *n);
bool is_leaf(node_t *n);
bool is_first_child(node_t *n);
bool is_second_child(node_t *n);
unsigned int clients_count_in(node_t *n);
node_t *brother_tree(node_t *n);
node_t *first_extrema(node_t *n);
node_t *second_extrema(node_t *n);
node_t *next_leaf(node_t *n, node_t *r);
node_t *prev_leaf(node_t *n, node_t *r);
node_t *next_tiled_leaf(desktop_t *d, node_t *n, node_t *r);
node_t *prev_tiled_leaf(desktop_t *d, node_t *n, node_t *r);
bool is_adjacent(node_t *a, node_t *b, direction_t dir);
node_t *find_fence(node_t *n, direction_t dir);
node_t *nearest_neighbor(monitor_t *m, desktop_t *d, node_t *n, direction_t dir, node_select_t sel);
bool is_child(node_t *a, node_t *b);
bool is_descendant(node_t *a, node_t *b);
bool find_by_id(uint32_t id, coordinates_t *loc);
node_t *find_by_id_in(node_t *r, uint32_t id);
node_t *nearest_from_tree(monitor_t *m, desktop_t *d, node_t *n, direction_t dir, node_select_t sel);
node_t *nearest_from_history(monitor_t *m, desktop_t *d, node_t *n, direction_t dir, node_select_t sel);
node_t *nearest_from_distance(monitor_t *m, desktop_t *d, node_t *n, direction_t dir, node_select_t sel);
void get_opposite(direction_t src, direction_t *dst);
unsigned int node_area(monitor_t *m, desktop_t *d, node_t *n);
int tiled_count(node_t *n);
node_t *find_biggest(monitor_t *m, desktop_t *d, node_t *n, node_select_t sel);
void rotate_tree(node_t *n, int deg);
void rotate_brother(node_t *n);
void unrotate_tree(node_t *n, int rot);
void unrotate_brother(node_t *n);
void flip_tree(node_t *n, flip_t flp);
void equalize_tree(node_t *n);
int balance_tree(node_t *n);
void unlink_node(monitor_t *m, desktop_t *d, node_t *n);
void remove_node(monitor_t *m, desktop_t *d, node_t *n);
void destroy_tree(monitor_t *m, desktop_t *d, node_t *n);
bool swap_nodes(monitor_t *m1, desktop_t *d1, node_t *n1, monitor_t *m2, desktop_t *d2, node_t *n2);
bool transfer_node(monitor_t *ms, desktop_t *ds, node_t *ns, monitor_t *md, desktop_t *dd, node_t *nd);
node_t *closest_node(monitor_t *m, desktop_t *d, node_t *n, cycle_dir_t dir, node_select_t sel);
void circulate_leaves(monitor_t *m, desktop_t *d, node_t *n, circulate_dir_t dir);
void set_vacant_state(monitor_t *m, desktop_t *d, node_t *n, bool value);
void propagate_vacant_state(monitor_t *m, desktop_t *d, node_t *n);
void set_layer(monitor_t *m, desktop_t *d, node_t *n, stack_layer_t l);
void set_state(monitor_t *m, desktop_t *d, node_t *n, client_state_t s);
void set_floating(monitor_t *m, desktop_t *d, node_t *n, bool value);
void set_fullscreen(monitor_t *m, desktop_t *d, node_t *n, bool value);
void neutralize_occluding_windows(monitor_t *m, desktop_t *d, node_t *n);
void set_locked(monitor_t *m, desktop_t *d, node_t *n, bool value);
void set_sticky(monitor_t *m, desktop_t *d, node_t *n, bool value);
void set_private(monitor_t *m, desktop_t *d, node_t *n, bool value);
void set_urgent(monitor_t *m, desktop_t *d, node_t *n, bool value);
bool contains(xcb_rectangle_t a, xcb_rectangle_t b);
xcb_rectangle_t get_rectangle(monitor_t *m, desktop_t *d, node_t *n);
void get_side_handle(monitor_t *m, desktop_t *d, node_t *n, direction_t dir, xcb_point_t *pt);

unsigned int sticky_count(node_t *n);
unsigned int private_count(node_t *n);
unsigned int locked_count(node_t *n);

#endif
