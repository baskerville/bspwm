#ifndef _TREE_H
#define _TREE_H

#define INC_EXP 0.9
#define DEC_EXP 1.1

bool is_leaf(node_t *);
bool is_tiled(client_t *);
bool is_floating(client_t *);
bool is_first_child(node_t *);
bool is_second_child(node_t *);
void change_split_ratio(node_t *, value_change_t);
void change_layout(monitor_t *, desktop_t *, layout_t);
node_t *first_extrema(node_t *);
node_t *second_extrema(node_t *);
node_t *next_leaf(node_t *, node_t *);
node_t *prev_leaf(node_t *, node_t *);
node_t *find_fence(node_t *, direction_t);
node_t *find_neighbor(node_t *, direction_t);
void get_opposite(direction_t, direction_t*);
node_t *nearest_neighbor(desktop_t *, node_t *, direction_t);
int tiled_area(node_t *);
node_t *find_biggest(desktop_t *);
void move_fence(node_t *, direction_t, fence_move_t);
void rotate_tree(node_t *, rotate_t);
void rotate_brother(node_t *);
void unrotate_tree(node_t *, rotate_t);
void unrotate_brother(node_t *);
void flip_tree(node_t *, flip_t);
int balance_tree(node_t *);
void arrange(monitor_t *, desktop_t *);
void apply_layout(monitor_t *, desktop_t *, node_t *, xcb_rectangle_t, xcb_rectangle_t);
void insert_node(monitor_t *, desktop_t *, node_t *);
void pseudo_focus(desktop_t *, node_t *);
void focus_node(monitor_t *, desktop_t *, node_t *);
void update_current(void);
void unlink_node(desktop_t *, node_t *);
void remove_node(desktop_t *, node_t *);
void destroy_tree(node_t *);
void swap_nodes(node_t *, node_t *);
void fit_monitor(monitor_t *, client_t *);
void transfer_node(monitor_t *, desktop_t *, monitor_t *, desktop_t *, node_t *);
void select_monitor(monitor_t *);
void select_desktop(monitor_t *, desktop_t *);
void cycle_monitor(cycle_dir_t);
void cycle_desktop(monitor_t *, desktop_t *, cycle_dir_t, skip_desktop_t);
void cycle_leaf(monitor_t *, desktop_t *, node_t *, cycle_dir_t, skip_client_t);
void nearest_leaf(monitor_t *, desktop_t *, node_t *, nearest_arg_t, skip_client_t);
void circulate_leaves(monitor_t *, desktop_t *, circulate_dir_t);
void update_vacant_state(node_t *);
void put_status(void);
void list_history(char *);
void list_monitors(list_option_t, char *);
void list_desktops(monitor_t *, list_option_t, unsigned int, char *);
void list(desktop_t *, node_t *, char *, unsigned int);
void restore_layout(char *);
void restore_history(char *);

#endif
