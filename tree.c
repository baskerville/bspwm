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
