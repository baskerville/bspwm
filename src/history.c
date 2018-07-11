/* Copyright (c) 2012, Bastien Dejean
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdbool.h>
#include "bspwm.h"
#include "tree.h"
#include "query.h"
#include "history.h"

history_t *make_history(monitor_t *m, desktop_t *d, node_t *n)
{
	history_t *h = calloc(1, sizeof(history_t));
	h->loc = (coordinates_t) {m, d, n};
	h->prev = h->next = NULL;
	h->latest = true;
	return h;
}

void history_add(monitor_t *m, desktop_t *d, node_t *n, bool focused)
{
	if (!record_history) {
		return;
	}

	if (focused) {
		history_needle = NULL;
	}

	history_t *h = make_history(m, d, n);

	if (history_head == NULL) {
		history_head = history_tail = h;
	} else if ((n != NULL && history_tail->loc.node != n) || (n == NULL && d != history_tail->loc.desktop)) {
		history_t *ip = focused ? history_tail : NULL;

		for (history_t *hh = history_tail; hh != NULL; hh = hh->prev) {
			if ((n != NULL && hh->loc.node == n) || (n == NULL && d == hh->loc.desktop)) {
				hh->latest = false;
			}
			if (ip == NULL && ((n != NULL && hh->loc.desktop == d) || (n == NULL && hh->loc.monitor == m))) {
				ip = hh;
			}
		}

		if (ip != NULL) {
			history_insert_after(h, ip);
		} else {
			ip = history_head;
			if (n != NULL) {
				for (history_t *hh = history_head; hh != NULL; hh = hh->next) {
					if (hh->latest && hh->loc.monitor == m) {
						ip = hh;
						break;
					}
				}
			}
			history_insert_before(h, ip);
		}
	} else {
		free(h);
	}
}

// Inserts `a` after `b`.
void history_insert_after(history_t *a, history_t *b)
{
	a->next = b->next;
	if (b->next != NULL) {
		b->next->prev = a;
	}
	b->next = a;
	a->prev = b;
	if (history_tail == b) {
		history_tail = a;
	}
}

// Inserts `a` before `b`.
void history_insert_before(history_t *a, history_t *b)
{
	a->prev = b->prev;
	if (b->prev != NULL) {
		b->prev->next = a;
	}
	b->prev = a;
	a->next = b;
	if (history_head == b) {
		history_head = a;
	}
}

void history_remove(desktop_t *d, node_t *n, bool deep)
{
	/* removing from the newest to the oldest is required */
	/* for maintaining the *latest* attribute */
	history_t *b = history_tail;
	while (b != NULL) {
		if ((n != NULL && ((deep && is_descendant(b->loc.node, n)) || (!deep && b->loc.node == n))) ||
		    (n == NULL && d == b->loc.desktop)) {
			history_t *a = b->next;
			history_t *c = b->prev;
			if (a != NULL) {
				/* remove duplicate entries */
				while (c != NULL && ((a->loc.node != NULL && a->loc.node == c->loc.node) ||
				       (a->loc.node == NULL && a->loc.desktop == c->loc.desktop))) {
					history_t *p = c->prev;
					if (history_head == c) {
						history_head = history_tail;
					}
					if (history_needle == c) {
						history_needle = history_tail;
					}
					free(c);
					c = p;
				}
				a->prev = c;
			}
			if (c != NULL) {
				c->next = a;
			}
			if (history_tail == b) {
				history_tail = c;
			}
			if (history_head == b) {
				history_head = a;
			}
			if (history_needle == b) {
				history_needle = c;
			}
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

node_t *history_last_node(desktop_t *d, node_t *n)
{
	for (history_t *h = history_tail; h != NULL; h = h->prev) {
		if (h->latest && h->loc.node != NULL && !h->loc.node->hidden &&
		    !is_descendant(h->loc.node, n) && h->loc.desktop == d) {
			return h->loc.node;
		}
	}
	return NULL;
}

desktop_t *history_last_desktop(monitor_t *m, desktop_t *d)
{
	for (history_t *h = history_tail; h != NULL; h = h->prev) {
		if (h->latest && h->loc.desktop != d && h->loc.monitor == m) {
			return h->loc.desktop;
		}
	}
	return NULL;
}

monitor_t *history_last_monitor(monitor_t *m)
{
	for (history_t *h = history_tail; h != NULL; h = h->prev) {
		if (h->latest && h->loc.monitor != m) {
			return h->loc.monitor;
		}
	}
	return NULL;
}

bool history_find_newest_node(coordinates_t *ref, coordinates_t *dst, node_select_t *sel)
{
	for (history_t *h = history_tail; h != NULL; h = h->prev) {
		if (h->loc.node == NULL ||
		    h->loc.node->hidden ||
		    !node_matches(&h->loc, ref, sel)) {
			continue;
		}
		*dst = h->loc;
		return true;
	}

	return false;
}

bool history_find_node(history_dir_t hdi, coordinates_t *ref, coordinates_t *dst, node_select_t *sel)
{
	if (history_needle == NULL || record_history) {
		history_needle = history_tail;
	}

	history_t *h;
	for (h = history_needle; h != NULL; h = (hdi == HISTORY_OLDER ? h->prev : h->next)) {
		if (!h->latest ||
		    h->loc.node == NULL ||
		    h->loc.node == ref->node ||
		    h->loc.node->hidden ||
		    !node_matches(&h->loc, ref, sel)) {
			continue;
		}
		if (!record_history) {
			history_needle = h;
		}
		*dst = h->loc;
		return true;
	}
	return false;
}

bool history_find_newest_desktop(coordinates_t *ref, coordinates_t *dst, desktop_select_t *sel)
{
	for (history_t *h = history_tail; h != NULL; h = h->prev) {
		if (desktop_matches(&h->loc, ref, sel)) {
			*dst = h->loc;
			return true;
		}
	}

	return false;
}

bool history_find_desktop(history_dir_t hdi, coordinates_t *ref, coordinates_t *dst, desktop_select_t *sel)
{
	if (history_needle == NULL || record_history) {
		history_needle = history_tail;
	}

	history_t *h;
	for (h = history_needle; h != NULL; h = (hdi == HISTORY_OLDER ? h->prev : h->next)) {
		if (!h->latest ||
		    h->loc.desktop == ref->desktop ||
		    !desktop_matches(&h->loc, ref, sel)) {
			continue;
		}
		if (!record_history) {
			history_needle = h;
		}
		*dst = h->loc;
		return true;
	}
	return false;
}

bool history_find_newest_monitor(coordinates_t *ref, coordinates_t *dst, monitor_select_t *sel)
{
	for (history_t *h = history_tail; h != NULL; h = h->prev) {
		if (monitor_matches(&h->loc, ref, sel)) {
			*dst = h->loc;
			return true;
		}
	}

	return false;
}

bool history_find_monitor(history_dir_t hdi, coordinates_t *ref, coordinates_t *dst, monitor_select_t *sel)
{
	if (history_needle == NULL || record_history) {
		history_needle = history_tail;
	}

	history_t *h;

	for (h = history_needle; h != NULL; h = (hdi == HISTORY_OLDER ? h->prev : h->next)) {
		if (!h->latest ||
		    h->loc.monitor == ref->monitor ||
		    !monitor_matches(&h->loc, ref, sel)) {
			continue;
		}
		if (!record_history) {
			history_needle = h;
		}
		*dst = h->loc;
		return true;
	}

	return false;
}

uint32_t history_rank(node_t *n)
{
	uint32_t r = 0;
	history_t *h = history_tail;
	while (h != NULL && (!h->latest || h->loc.node != n)) {
		h = h->prev;
		r++;
	}
	if (h == NULL) {
		return UINT32_MAX;
	} else {
		return r;
	}
}
