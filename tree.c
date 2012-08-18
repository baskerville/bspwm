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
    Node *p = n->parent;
    Node *neighbor = NULL;

    if (n == NULL || p == NULL)
        return NULL;

    while (p != NULL && neighbor == NULL) {
        switch (dir) {
            case DIR_UP:
                if (p->split_type == TYPE_HORIZONTAL && p->rectangle.y < n->rectangle.y)
                    neighbor = second_extrema(p->first_child);
                break;
            case DIR_LEFT:
                if (p->split_type == TYPE_VERTICAL && p->rectangle.x < n->rectangle.x)
                    neighbor = second_extrema(p->first_child);
                break;
            case DIR_DOWN:
                if (p->split_type == TYPE_HORIZONTAL && (p->rectangle.y + p->rectangle.height) > (n->rectangle.y + n->rectangle.height))
                    neighbor = first_extrema(p->second_child);
                break;
            case DIR_RIGHT:
                if (p->split_type == TYPE_VERTICAL && (p->rectangle.x + p->rectangle.width) > (n->rectangle.x + n->rectangle.width))
                    neighbor = first_extrema(p->second_child);
                break;
        }
        p = p->parent;
    }
    return neighbor;
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
