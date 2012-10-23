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
node_t *first_extrema(node_t *);
node_t *second_extrema(node_t *);
node_t *next_leaf(node_t *);
node_t *prev_leaf(node_t *);
node_t *find_fence(node_t *, direction_t);
node_t *find_neighbor(node_t *, direction_t);
void move_fence(node_t *, direction_t, fence_move_t);
void rotate_tree(node_t *, rotate_t);
void magnetise_tree(node_t *, corner_t);
void update_root_dimensions(void);
void apply_layout(desktop_t *, node_t *, xcb_rectangle_t);
void insert_node(desktop_t *, node_t *);
void dump_tree(desktop_t *, node_t *, char *, int);
void list_desktops(char *);
void focus_node(desktop_t *, node_t *, bool);
void update_current(void);
void unlink_node(desktop_t *, node_t *);
void remove_node(desktop_t *, node_t *);
void swap_nodes(node_t *, node_t *);
void transfer_node(desktop_t *, desktop_t *, node_t *);
void select_desktop(desktop_t *);
void cycle_desktop(cycle_dir_t);
void nearest_leaf(desktop_t *, node_t *, nearest_arg_t, skip_client_t);
void cycle_leaf(desktop_t *, node_t *, cycle_dir_t, skip_client_t);
void update_vacant_state(node_t *);
desktop_t *find_desktop(char *);
void add_desktop(char *);

#endif
