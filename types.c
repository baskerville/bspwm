#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "types.h"

node_t *make_node(void)
{
    node_t *n = malloc(sizeof(node_t));
    n->parent = n->first_child = n->second_child = n->next_leaf = n->prev_leaf = NULL;
    n->client = NULL;
    return n;
}

desktop_t *make_desktop(void)
{
    desktop_t *d = malloc(sizeof(desktop_t));
    d->name = NULL;
    d->layout = LAYOUT_TILED;
    d->prev = d->next = NULL;
    d->root = d->view = d->focus = d->last_focus = d->head = d->tail = NULL;
    return d;
}

client_t *make_client(void)
{
    client_t *c = malloc(sizeof(client_t));
    c->window = 0;
    c->floating = false;
    c->fullscreen = false;
    c->locked = false;
    return c;
}
