#ifndef _TREE_H
#define _TREE_H

#include <stdio.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "utils.h"
#include "types.h"

Node *first_extrema(Node *);
Node *second_extrema(Node *);
Node *find_fence(Node *, direction_t);
Node *find_neighbor(Node *, direction_t);
void rotate_tree(Node *, rotate_t);
void dump_tree(Node *, char *, int);

#endif
