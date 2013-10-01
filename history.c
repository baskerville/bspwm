#include <stdlib.h>
#include "bspwm.h"
#include "query.h"

history_t *make_history(monitor_t *m, desktop_t *d, node_t *n)
{
    history_t *h = malloc(sizeof(history_t));
    h->loc = (coordinates_t) {m, d, n};
    h->prev = h->next = NULL;
    h->latest = true;
    return h;
}

void history_add(monitor_t *m, desktop_t *d, node_t *n)
{
    history_t *h = make_history(m, d, n);
    if (history_head == NULL) {
        history_head = history_tail = h;
    } else if ((n != NULL && history_tail->loc.node != n) || (n == NULL && d != history_tail->loc.desktop)) {
        for (history_t *hh = history_tail; hh != NULL; hh = hh->prev)
            if ((n != NULL && hh->loc.node == n) || (n == NULL && d == hh->loc.desktop))
                hh->latest = false;
        history_tail->next = h;
        h->prev = history_tail;
        history_tail = h;
    } else {
        free(h);
    }
}

void history_transfer_node(monitor_t *m, desktop_t *d, node_t *n)
{
    for (history_t *h = history_head; h != NULL; h = h->next)
        if (h->loc.node == n) {
            h->loc.monitor = m;
            h->loc.desktop = d;
        }
}

void history_transfer_desktop(monitor_t *m, desktop_t *d)
{
    for (history_t *h = history_head; h != NULL; h = h->next)
        if (h->loc.desktop == d)
            h->loc.monitor = m;
}

void history_swap_nodes(monitor_t *m1, desktop_t *d1, node_t *n1, monitor_t *m2, desktop_t *d2, node_t *n2)
{
    for (history_t *h = history_head; h != NULL; h = h->next)
        if (h->loc.node == n1) {
            h->loc.monitor = m2;
            h->loc.desktop = d2;
        } else if (h->loc.node == n2) {
            h->loc.monitor = m1;
            h->loc.desktop = d1;
        }
}

void history_swap_desktops(monitor_t *m1, desktop_t *d1, monitor_t *m2, desktop_t *d2)
{
    for (history_t *h = history_head; h != NULL; h = h->next)
        if (h->loc.desktop == d1)
            h->loc.monitor = m2;
        else if (h->loc.desktop == d2)
            h->loc.monitor = m1;
}

void history_remove(desktop_t *d, node_t *n)
{
   /* removing from the newest to the oldest is required */
   /* for maintaining the *latest* attribute */
    history_t *b = history_tail;
    while (b != NULL) {
        if ((n != NULL && n == b->loc.node) || (n == NULL && d == b->loc.desktop)) {
            history_t *a = b->next;
            history_t *c = b->prev;
            if (a != NULL) {
                /* remove duplicate entries */
                while (c != NULL && ((a->loc.node != NULL && a->loc.node == c->loc.node)
                            || (a->loc.node == NULL && a->loc.desktop == c->loc.desktop))) {
                    history_t *d = c->prev;
                    if (history_head == c)
                        history_head = history_tail;
                    free(c);
                    c = d;
                }
                a->prev = c;
            }
            if (c != NULL)
                c->next = a;
            if (history_tail == b)
                history_tail = c;
            if (history_head == b)
                history_head = a;
            free(b);
            b = c;
        } else {
            b = b->prev;
        }
    }
}

void empty_history(void)
{
    history_t *h = history_head;
    while (h != NULL) {
        history_t *next = h->next;
        free(h);
        h = next;
    }
    history_head = history_tail = NULL;
}

node_t *history_get_node(desktop_t *d, node_t *n)
{
    for (history_t *h = history_tail; h != NULL; h = h->prev)
        if (h->latest && h->loc.node != NULL && h->loc.node != n && h->loc.desktop == d)
            return h->loc.node;
    return NULL;
}

desktop_t *history_get_desktop(monitor_t *m, desktop_t *d)
{
    for (history_t *h = history_tail; h != NULL; h = h->prev)
        if (h->latest && h->loc.desktop != d && h->loc.monitor == m)
            return h->loc.desktop;
    return NULL;
}

monitor_t *history_get_monitor(monitor_t *m)
{
    for (history_t *h = history_tail; h != NULL; h = h->prev)
        if (h->latest && h->loc.monitor != m)
            return h->loc.monitor;
    return NULL;
}

bool history_last_node(node_t *n, client_select_t sel, coordinates_t *loc)
{
    for (history_t *h = history_tail; h != NULL; h = h->prev) {
        if (!h->latest || h->loc.node == NULL || h->loc.node == n || !node_matches(n, h->loc.node, sel))
            continue;
        *loc = h->loc;
        return true;
    }
    return false;
}

bool history_last_desktop(desktop_t *d, desktop_select_t sel, coordinates_t *loc)
{
    for (history_t *h = history_tail; h != NULL; h = h->prev) {
        if (!h->latest || h->loc.desktop == d || !desktop_matches(h->loc.desktop, sel))
            continue;
        *loc = h->loc;
        return true;
    }
    return false;
}

bool history_last_monitor(monitor_t *m, desktop_select_t sel, coordinates_t *loc)
{
    for (history_t *h = history_tail; h != NULL; h = h->prev) {
        if (!h->latest || h->loc.monitor == m || !desktop_matches(h->loc.monitor->desk, sel))
            continue;
        *loc = h->loc;
        return true;
    }
    return false;
}

int history_rank(desktop_t *d, node_t *n)
{
    int i = 0;
    history_t *h = history_tail;
    while (h != NULL && (!h->latest || h->loc.node != n || h->loc.desktop != d)) {
        h = h->prev;
        i++;
    }
    if (h == NULL)
        return -1;
    else
        return i;
}
