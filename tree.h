#ifndef _TREE_H
#define _TREE_H

node_t *make_node(void);
client_t *make_client(xcb_window_t);
void arrange(monitor_t *, desktop_t *);
void apply_layout(monitor_t *, desktop_t *, node_t *, xcb_rectangle_t, xcb_rectangle_t);
void focus_node(monitor_t *, desktop_t *, node_t *);
void insert_node(monitor_t *, desktop_t *, node_t *, node_t *);
void unlink_node(desktop_t *, node_t *);
void remove_node(desktop_t *, node_t *);
void transfer_node(monitor_t *, desktop_t *, node_t *, monitor_t *, desktop_t *, node_t *);
void swap_nodes(monitor_t *, desktop_t *, node_t *, monitor_t *, desktop_t *, node_t *);
void pseudo_focus(desktop_t *, node_t *);
void update_current(void);
node_t *find_fence(node_t *, direction_t);
node_t *nearest_neighbor(desktop_t *, node_t *, direction_t, client_select_t);
node_t *nearest_from_distance(desktop_t *, node_t *, direction_t, client_select_t);
node_t *nearest_from_history(desktop_t *, node_t *, direction_t, client_select_t);
node_t *find_biggest(desktop_t *, node_t *, client_select_t);
bool is_leaf(node_t *);
bool is_tiled(client_t *);
bool is_floating(client_t *);
bool is_first_child(node_t *);
bool is_second_child(node_t *);
void change_split_ratio(node_t *, value_change_t);
void reset_mode(coordinates_t *);
node_t *brother_tree(node_t *);
node_t *first_extrema(node_t *);
node_t *second_extrema(node_t *);
node_t *next_leaf(node_t *, node_t *);
node_t *prev_leaf(node_t *, node_t *);
bool is_adjacent(node_t *, node_t *, direction_t);
void get_opposite(direction_t, direction_t *);
int tiled_area(node_t *);
void move_fence(node_t *, direction_t, fence_move_t);
void rotate_tree(node_t *, int);
void rotate_brother(node_t *);
void unrotate_tree(node_t *, int);
void unrotate_brother(node_t *);
void flip_tree(node_t *, flip_t);
int balance_tree(node_t *);
void destroy_tree(node_t *);
node_t *closest_node(desktop_t *, node_t *, cycle_dir_t, client_select_t);
void circulate_leaves(monitor_t *, desktop_t *, circulate_dir_t);
void update_vacant_state(node_t *);

#endif
