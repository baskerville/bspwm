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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>
#include "bspwm.h"
#include "tree.h"
#include "settings.h"
#include "subscribe.h"

subscriber_list_t *make_subscriber_list(FILE *stream, int field)
{
	subscriber_list_t *sb = malloc(sizeof(subscriber_list_t));
	sb->prev = sb->next = NULL;
	sb->stream = stream;
	sb->field = field;
	return sb;
}

void remove_subscriber(subscriber_list_t *sb)
{
	if (sb == NULL) {
		return;
	}
	subscriber_list_t *a = sb->prev;
	subscriber_list_t *b = sb->next;
	if (a != NULL) {
		a->next = b;
	}
	if (b != NULL) {
		b->prev = a;
	}
	if (sb == subscribe_head) {
		subscribe_head = b;
	}
	if (sb == subscribe_tail) {
		subscribe_tail = a;
	}
	fclose(sb->stream);
	free(sb);
}

void add_subscriber(FILE *stream, int field)
{
	subscriber_list_t *sb = make_subscriber_list(stream, field);
	if (subscribe_head == NULL) {
		subscribe_head = subscribe_tail = sb;
	} else {
		subscribe_tail->next = sb;
		sb->prev = subscribe_tail;
		subscribe_tail = sb;
	}
	if (sb->field & SBSC_MASK_REPORT) {
		print_report(sb->stream);
	}
}

int print_report(FILE *stream)
{
	fprintf(stream, "%s", status_prefix);
	bool urgent = false;
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		fprintf(stream, "%c%s", (mon == m ? 'M' : 'm'), m->name);
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next, urgent = false) {
			for (node_t *n = first_extrema(d->root); n != NULL && !urgent; n = next_leaf(n, d->root)) {
				urgent |= n->client->urgent;
			}
			char c = (urgent ? 'u' : (d->root == NULL ? 'f' : 'o'));
			if (m->desk == d) {
				c = toupper(c);
			}
			fprintf(stream, ":%c%s", c, d->name);
		}
		if (m->desk != NULL) {
			fprintf(stream, ":L%c", LAYOUT_CHR(m->desk->layout));
			if (m->desk->focus != NULL) {
				node_t *n = m->desk->focus;
				if (n->client != NULL) {
					fprintf(stream, ":T%c", STATE_CHR(n->client->state));
				} else {
					fprintf(stream, ":T@");
				}
				int i = 0;
				char flags[4];
				if (n->sticky) {
					flags[i++] = 'S';
				}
				if (n->private) {
					flags[i++] = 'P';
				}
				if (n->locked) {
					flags[i++] = 'L';
				}
				flags[i] = '\0';
				fprintf(stream, ":G%s", flags);
			}
		}
		if (m != mon_tail) {
			fprintf(stream, "%s", ":");
		}
	}
	fprintf(stream, "%s", "\n");
	return fflush(stream);
}

void put_status(subscriber_mask_t mask, ...)
{
	subscriber_list_t *sb = subscribe_head;
	int ret;
	while (sb != NULL) {
		subscriber_list_t *next = sb->next;
		if (sb->field & mask) {
			if (mask == SBSC_MASK_REPORT) {
				ret = print_report(sb->stream);
			} else {
				char *fmt;
				va_list args;
				va_start(args, mask);
				fmt = va_arg(args, char *);
				vfprintf(sb->stream, fmt, args);
				va_end(args);
				ret = fflush(sb->stream);
			}
			if (ret != 0) {
				remove_subscriber(sb);
			}
		}
		sb = next;
	}
}
