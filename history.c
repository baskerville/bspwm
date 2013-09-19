#include <stdlib.h>
#include "types.h"
#include "query.h"

focus_history_t *make_focus_history(void)
{
    focus_history_t *f = malloc(sizeof(focus_history_t));
    f->head = f->tail = NULL;
    return f;
}

node_list_t *make_node_list(void)
{
    node_list_t *n = malloc(sizeof(node_list_t));
    n->node = NULL;
    n->prev = n->next = NULL;
    n->latest = true;
    return n;
}

void history_add(focus_history_t *f, node_t *n)
{
    node_list_t *a = make_node_list();
    a->node = n;
    if (f->head == NULL) {
        f->head = f->tail = a;
    } else if (f->head->node != n) {
        for (node_list_t *b = f->head; b != NULL; b = b->next)
            if (b->node == n)
                b->latest = false;
        f->head->prev = a;
        a->next = f->head;
        f->head = a;
    } else {
        free(a);
    }
}

void history_remove(focus_history_t *f, node_t *n)
{
    /* in order to maintain the `latest` node list state,
       we remove node lists from head to tail */
    node_list_t *b = f->head;
    while (b != NULL) {
        if (b->node == n) {
            node_list_t *a = b->prev;
            node_list_t *c = b->next;
            if (a != NULL) {
                /* remove duplicate entries */
                while (c != NULL && c->node == a->node) {
                    node_list_t *d = c->next;
                    if (f->tail == c)
                        f->tail = f->head;
                    free(c);
                    c = d;
                }
                a->next = c;
            }
            if (c != NULL)
                c->prev = a;
            if (f->head == b)
                f->head = c;
            if (f->tail == b)
                f->tail = a;
            free(b);
            b = c;
        } else {
            b = b->next;
        }
    }
}

void empty_history(focus_history_t *f)
{
    node_list_t *a = f->head;
    while (a != NULL) {
        node_list_t *b = a->next;
        free(a);
        a = b;
    }
    f->head = f->tail = NULL;
}

node_t *history_get(focus_history_t *f, int i)
{
    node_list_t *a = f->head;
    while (a != NULL && i > 0) {
        a = a->next;
        i--;
    }
    if (a == NULL)
        return NULL;
    else
        return a->node;
}

node_t *history_last(focus_history_t *f, node_t *n, client_select_t sel)
{
    for (node_list_t *a = f->head; a != NULL; a = a->next) {
        if (!a->latest || a->node == n || !node_matches(n, a->node, sel))
            continue;
        return a->node;
    }
    return NULL;
}

int history_rank(focus_history_t *f, node_t *n)
{
    int i = 0;
    node_list_t *a = f->head;
    while (a != NULL && (!a->latest || a->node != n)) {
        a = a->next;
        i++;
    }
    if (a == NULL)
        return -1;
    else
        return i;
}
