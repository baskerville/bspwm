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
#include "bspwm.h"
#include "window.h"
#include "stack.h"

stacking_list_t *make_stack(node_t *n)
{
	stacking_list_t *s = malloc(sizeof(stacking_list_t));
	s->node = n;
	s->prev = s->next = NULL;
	return s;
}

void stack_insert_after(stacking_list_t *a, node_t *n)
{
	stacking_list_t *s = make_stack(n);
	if (a == NULL) {
		stack_head = stack_tail = s;
	} else {
		if (a->node == n)
			return;
		remove_stack_node(n);
		stacking_list_t *b = a->next;
		if (b != NULL)
			b->prev = s;
		s->next = b;
		s->prev = a;
		a->next = s;
		if (stack_tail == a)
			stack_tail = s;
	}
}

void stack_insert_before(stacking_list_t *a, node_t *n)
{
	stacking_list_t *s = make_stack(n);
	if (a == NULL) {
		stack_head = stack_tail = s;
	} else {
		if (a->node == n)
			return;
		remove_stack_node(n);
		stacking_list_t *b = a->prev;
		if (b != NULL)
			b->next = s;
		s->prev = b;
		s->next = a;
		a->prev = s;
		if (stack_head == a)
			stack_head = s;
	}
}

void remove_stack(stacking_list_t *s)
{
	if (s == NULL)
		return;
	stacking_list_t *a = s->prev;
	stacking_list_t *b = s->next;
	if (a != NULL)
		a->next = b;
	if (b != NULL)
		b->prev = a;
	if (s == stack_head)
		stack_head = b;
	if (s == stack_tail)
		stack_tail = a;
	free(s);
}

void remove_stack_node(node_t *n)
{
	for (stacking_list_t *s = stack_head; s != NULL; s = s->next) {
		if (s->node == n) {
			remove_stack(s);
			return;
		}
	}
}

int stack_cmp(client_t *c1, client_t *c2)
{
	if (c1->layer == c2->layer) {
		if (!c1->floating && c2->floating) {
			return -1;
		} else if (c1->floating && !c2->floating) {
			return 1;
		} else {
			return 0;
		}
	} else {
		if (c1->layer == LAYER_BELOW) {
			return -1;
		} else if (c1->layer == LAYER_ABOVE) {
			return 1;
		/* c1->layer == LAYER_NORMAL */
		} else {
			if (c2->layer == LAYER_ABOVE) {
				return -1;
			/* c2->layer == LAYER_BELOW */
			} else {
				return 1;
			}
		}
	}
}

void stack(node_t *n)
{
	PRINTF("stack %X\n", n->client->window);

	if (stack_head == NULL) {
		stack_insert_after(NULL, n);
	} else {
		if (n->client->floating && !auto_raise)
			return;
		stacking_list_t *s = stack_head;
		while (s != NULL && stack_cmp(n->client, s->node->client) >= 0) {
			s = s->next;
		}
		if (s == NULL) {
			s = stack_tail;
		}
		if (s->node == n) {
			s = s->prev;
			if (s == NULL) {
				return;
			}
		}
		if (stack_cmp(n->client, s->node->client) < 0) {
			stack_insert_before(s, n);
			window_below(n->client->window, s->node->client->window);
		} else {
			stack_insert_after(s, n);
			window_above(n->client->window, s->node->client->window);
		}
	}
}
