/* * Copyright (c) 2012-2013 Bastien Dejean
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include "bspwm.h"
#include "tree.h"
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
    if (!record_history)
        return;
    history_needle = NULL;
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
                    if (history_needle == c)
                        history_needle = history_tail;
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
            if (history_needle == b)
                history_needle = c;
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
        if (h->latest && h->loc.node != NULL && h->loc.node != n && h->loc.desktop == d && is_visible(h->loc.desktop, h->loc.node))
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

bool history_find_node(history_dir_t hdi, coordinates_t *ref, coordinates_t *dst, client_select_t sel)
{
    if (history_needle == NULL || record_history)
        history_needle = history_tail;

    history_t *h;
    for (h = history_needle; h != NULL; h = (hdi == HISTORY_OLDER ? h->prev : h->next)) {
        if (!h->latest
                || h->loc.node == NULL
                || h->loc.node == ref->node
                || !is_visible(h->loc.desktop, h->loc.node)
                || !node_matches(&h->loc, ref, sel))
            continue;
        if (!record_history)
            history_needle = h;
        *dst = h->loc;
        return true;
    }
    return false;
}

bool history_find_desktop(history_dir_t hdi, coordinates_t *ref, coordinates_t *dst, desktop_select_t sel)
{
    if (history_needle == NULL || record_history)
        history_needle = history_tail;

    history_t *h;
    for (h = history_needle; h != NULL; h = (hdi == HISTORY_OLDER ? h->prev : h->next)) {
        if (!h->latest
                || h->loc.desktop == ref->desktop
                || !desktop_matches(&h->loc, ref, sel))
            continue;
        if (!record_history)
            history_needle = h;
        *dst = h->loc;
        return true;
    }
    return false;
}

bool history_find_monitor(history_dir_t hdi, coordinates_t *ref, coordinates_t *dst, desktop_select_t sel)
{
    if (history_needle == NULL || record_history)
        history_needle = history_tail;

    history_t *h;
    for (h = history_needle; h != NULL; h = (hdi == HISTORY_OLDER ? h->prev : h->next)) {
        if (!h->latest
                || h->loc.monitor == ref->monitor
                || !desktop_matches(&h->loc, ref, sel))
            continue;
        if (!record_history)
            history_needle = h;
        *dst = h->loc;
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
