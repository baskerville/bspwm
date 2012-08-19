#include "types.h"

Node *make_node(void)
{
    Node *n = malloc(sizeof(Node));
    n->parent = n->first_child = n->second_child = NULL;
    n->client = NULL;
    return n;
}

Desktop *make_desktop(void)
{
    Desktop *d = malloc(sizeof(Desktop));
    d->name = NULL;
    d->previous = d->next = NULL;
    d->layout = LAYOUT_TILED;
    return d;
}
