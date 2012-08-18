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
    d->tiling_layout = LAYOUT_TILED;
    d->selected_layer = LAYER_TILING;
    d->tiling_layer.head = NULL;
    d->tiling_layer.focus = NULL;
    d->floating_layer.head = NULL;
    d->floating_layer.focus = NULL;
    return d;
}
