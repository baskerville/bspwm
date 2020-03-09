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

#define MIN_WIDTH   32
#define MIN_HEIGHT  32

void arrange(monitor_t *m, desktop_t *d);
void apply_layout(monitor_t *m, desktop_t *d, node_t *n, xcb_rectangle_t rect, xcb_rectangle_t root_rect);
presel_t *make_presel(void);
void set_ratio(node_t *n, double rat);
void presel_dir(monitor_t *m, desktop_t *d, node_t *n, direction_t dir);
void presel_ratio(monitor_t *m, desktop_t *d, node_t *n, double ratio);
void cancel_presel(monitor_t *m, desktop_t *d, node_t *n);
void cancel_presel_in(monitor_t *m, desktop_t *d, node_t *n);
node_t *find_public(desktop_t *d);
node_t *insert_node(monitor_t *m, desktop_t *d, node_t *n, node_t *f);
void insert_receptacle(monitor_t *m, desktop_t *d, node_t *n);
bool activate_node(monitor_t *m, desktop_t *d, node_t *n);
void transfer_sticky_nodes(monitor_t *ms, desktop_t *ds, monitor_t *md, desktop_t *dd, node_t *n);
bool focus_node(monitor_t *m, desktop_t *d, node_t *n);
void hide_node(desktop_t *d, node_t *n);
void show_node(desktop_t *d, node_t *n);
node_t *make_node(uint32_t id);
client_t *make_client(void);
void initialize_client(node_t *n);
bool is_focusable(node_t *n);
bool is_leaf(node_t *n);
bool is_first_child(node_t *n);
bool is_second_child(node_t *n);
unsigned int clients_count_in(node_t *n);
node_t *brother_tree(node_t *n);
node_t *first_extrema(node_t *n);
node_t *second_extrema(node_t *n);
node_t *first_focusable_leaf(node_t *n);
node_t *next_leaf(node_t *n, node_t *r);
node_t *prev_leaf(node_t *n, node_t *r);
node_t *next_tiled_leaf(node_t *n, node_t *r);
node_t *prev_tiled_leaf(node_t *n, node_t *r);
bool is_adjacent(node_t *a, node_t *b, direction_t dir);
node_t *find_fence(node_t *n, direction_t dir);
bool is_child(node_t *a, node_t *b);
bool is_descendant(node_t *a, node_t *b);
bool find_by_id(uint32_t id, coordinates_t *loc);
node_t *find_by_id_in(node_t *r, uint32_t id);
void find_any_node(coordinates_t *ref, coordinates_t *dst, node_select_t *sel);
bool find_any_node_in(monitor_t *m, desktop_t *d, node_t *n, coordinates_t *ref, coordinates_t *dst, node_select_t *sel);
void find_first_ancestor(coordinates_t *ref, coordinates_t *dst, node_select_t *sel);
void find_nearest_neighbor(coordinates_t *ref, coordinates_t *dst, direction_t dir, node_select_t *sel);
unsigned int node_area(desktop_t *d, node_t *n);
int tiled_count(node_t *n, bool include_receptacles);
void find_by_area(area_peak_t ap, coordinates_t *ref, coordinates_t *dst, node_select_t *sel);
void rotate_tree(node_t *n, int deg);
void rotate_tree_rec(node_t *n, int deg);
void flip_tree(node_t *n, flip_t flp);
void equalize_tree(node_t *n);
int balance_tree(node_t *n);
void adjust_ratios(node_t *n, xcb_rectangle_t rect);
void unlink_node(monitor_t *m, desktop_t *d, node_t *n);
void close_node(node_t *n);
void kill_node(monitor_t *m, desktop_t *d, node_t *n);
void remove_node(monitor_t *m, desktop_t *d, node_t *n);
void free_node(node_t *n);
bool swap_nodes(monitor_t *m1, desktop_t *d1, node_t *n1, monitor_t *m2, desktop_t *d2, node_t *n2, bool follow);
bool transfer_node(monitor_t *ms, desktop_t *ds, node_t *ns, monitor_t *md, desktop_t *dd, node_t *nd, bool follow);
bool find_closest_node(coordinates_t *ref, coordinates_t *dst, cycle_dir_t dir, node_select_t *sel);
void circulate_leaves(monitor_t *m, desktop_t *d, node_t *n, circulate_dir_t dir);
void set_vacant(monitor_t *m, desktop_t *d, node_t *n, bool value);
void set_vacant_local(monitor_t *m, desktop_t *d, node_t *n, bool value);
void propagate_vacant_downward(monitor_t *m, desktop_t *d, node_t *n, bool value);
void propagate_vacant_upward(monitor_t *m, desktop_t *d, node_t *n);
bool set_layer(monitor_t *m, desktop_t *d, node_t *n, stack_layer_t l);
bool set_state(monitor_t *m, desktop_t *d, node_t *n, client_state_t s);
void set_floating(monitor_t *m, desktop_t *d, node_t *n, bool value);
void set_fullscreen(monitor_t *m, desktop_t *d, node_t *n, bool value);
void neutralize_occluding_windows(monitor_t *m, desktop_t *d, node_t *n);
void rebuild_constraints(node_t *n);
void update_constraints(node_t *n);
void propagate_flags_upward(monitor_t *m, desktop_t *d, node_t *n);
void set_hidden(monitor_t *m, desktop_t *d, node_t *n, bool value);
void set_hidden_local(monitor_t *m, desktop_t *d, node_t *n, bool value);
void propagate_hidden_downward(monitor_t *m, desktop_t *d, node_t *n, bool value);
void propagate_hidden_upward(monitor_t *m, desktop_t *d, node_t *n);
void set_sticky(monitor_t *m, desktop_t *d, node_t *n, bool value);
void set_private(monitor_t *m, desktop_t *d, node_t *n, bool value);
void set_locked(monitor_t *m, desktop_t *d, node_t *n, bool value);
void set_marked(monitor_t *m, desktop_t *d, node_t *n, bool value);
void set_urgent(monitor_t *m, desktop_t *d, node_t *n, bool value);
xcb_rectangle_t get_rectangle(monitor_t *m, desktop_t *d, node_t *n);
void listen_enter_notify(node_t *n, bool enable);
void regenerate_ids_in(node_t *n);

unsigned int sticky_count(node_t *n);
unsigned int private_count(node_t *n);
unsigned int locked_count(node_t *n);

#endif
