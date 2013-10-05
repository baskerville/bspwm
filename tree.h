#ifndef _TREE_H
#define _TREE_H

void arrange(monitor_t *m, desktop_t *d);
void apply_layout(monitor_t *m, desktop_t *d, node_t *n, xcb_rectangle_t rect, xcb_rectangle_t root_rect);
void insert_node(monitor_t *m, desktop_t *d, node_t *n, node_t *f);
void pseudo_focus(desktop_t *d, node_t *n);
void focus_node(monitor_t *m, desktop_t *d, node_t *n);
void update_current(void);
node_t *make_node(void);
client_t *make_client(xcb_window_t win);
bool is_visible(desktop_t *d, node_t *n);
bool is_leaf(node_t *n);
bool is_tiled(client_t *c);
bool is_floating(client_t *c);
bool is_first_child(node_t *n);
bool is_second_child(node_t *n);
void change_split_ratio(node_t *n, value_change_t chg);
void reset_mode(coordinates_t *loc);
node_t *brother_tree(node_t *n);
node_t *closest_visible(desktop_t *d, node_t *n);
node_t *first_extrema(node_t *n);
node_t *second_extrema(node_t *n);
node_t *next_leaf(node_t *n, node_t *r);
node_t *prev_leaf(node_t *n, node_t *r);
bool is_adjacent(node_t *a, node_t *b, direction_t dir);
node_t *find_fence(node_t *n, direction_t dir);
node_t *nearest_neighbor(desktop_t *d, node_t *n, direction_t dir, client_select_t sel);
node_t *nearest_from_history(desktop_t *d, node_t *n, direction_t dir, client_select_t sel);
node_t *nearest_from_distance(desktop_t *d, node_t *n, direction_t dir, client_select_t sel);
void get_opposite(direction_t src, direction_t *dst);
int tiled_area(node_t *n);
node_t *find_biggest(desktop_t *d, node_t *c, client_select_t sel);
void move_fence(node_t *n, direction_t dir, fence_move_t mov);
void rotate_tree(node_t *n, int deg);
void rotate_brother(node_t *n);
void unrotate_tree(node_t *n, int rot);
void unrotate_brother(node_t *n);
void flip_tree(node_t *n, flip_t flp);
int balance_tree(node_t *n);
void unlink_node(desktop_t *d, node_t *n);
void remove_node(desktop_t *d, node_t *n);
void destroy_tree(node_t *n);
bool swap_nodes(monitor_t *m1, desktop_t *d1, node_t *n1, monitor_t *m2, desktop_t *d2, node_t *n2);
bool transfer_node(monitor_t *ms, desktop_t *ds, node_t *ns, monitor_t *md, desktop_t *dd, node_t *nd);
node_t *closest_node(desktop_t *d, node_t *n, cycle_dir_t dir, client_select_t sel);
void circulate_leaves(monitor_t *m, desktop_t *d, circulate_dir_t dir);
void update_vacant_state(node_t *n);

#endif
