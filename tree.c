#include "tree.h"

Node *first_extrema(Node *n)
{
    if (n == NULL)
        return NULL;
    else if (n->first_child == NULL)
        return n;
    else
        return first_extrema(n->first_child);
}

Node *second_extrema(Node *n)
{
    if (n == NULL)
        return NULL;
    else if (n->second_child == NULL)
        return n;
    else
        return second_extrema(n->second_child);
}

Node *find_neighbor(Node *n, direction_t dir)
{
    if (n == NULL)
        return NULL;
    switch (dir) {
        case DIR_UP:
            break;
        case DIR_LEFT:
            break;
        case DIR_DOWN:
            break;
        case DIR_RIGHT:
            break;
    }
    return NULL;
}

void rotate_tree(Node *n, rotate_t rot)
{
    Node *tmp;
    if (n == NULL)
        return;
    rotate_tree(n->first_child, rot);
    rotate_tree(n->second_child, rot);
    if ((rot == ROTATE_CLOCK_WISE && n->split_type == TYPE_HORIZONTAL)
            || (rot == ROTATE_COUNTER_CW && n->split_type == TYPE_VERTICAL)
            || rot == ROTATE_FULL_CYCLE) {
        tmp = n->first_child;
        n->first_child = n->second_child;
        n->second_child = tmp;
        n->split_ratio = 1.0 - n->split_ratio;
    }
    if (rot != ROTATE_FULL_CYCLE) {
        if (n->split_type == TYPE_HORIZONTAL)
            n->split_type = TYPE_VERTICAL;
        else if (n->split_type == TYPE_VERTICAL)
            n->split_type = TYPE_HORIZONTAL;
    }
}
