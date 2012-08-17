#ifndef _TREE_H
#define _TREE_H

#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "utils.h"
#include "types.h"

Node *first_extrema(Node *);
Node *second_extrema(Node *);
Node *find_neighbor(Node *, direction_t);

#endif
