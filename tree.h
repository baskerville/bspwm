#ifndef _TREE_H
#define _TREE_H

#define INC_EXP 0.9
#define DEC_EXP 1.1

bool is_leaf(Node *);
void change_split_ratio(Node *, value_change_t);
Node *first_extrema(Node *);
Node *second_extrema(Node *);
Node *find_fence(Node *, direction_t);
Node *find_neighbor(Node *, direction_t);
void move_fence(Node *, direction_t, fence_move_t);
void rotate_tree(Node *, rotate_t);
void dump_tree(Node *, char *, int);

#endif
