#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "types.h"

Node *make_node(void)
{
    Node *n = malloc(sizeof(Node));
    n->parent = n->first_child = n->second_child = n->next_leaf = n->prev_leaf = NULL;
    n->client = NULL;
    return n;
}

Desktop *make_desktop(void)
{
    Desktop *d = malloc(sizeof(Desktop));
    d->name = NULL;
    d->prev = d->next = NULL;
    return d;
}
