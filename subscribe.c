/* Copyright (c) 2012-2014, Bastien Dejean
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
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */

#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include "bspwm.h"
#include "tree.h"
#include "settings.h"
#include "subscribe.h"

subscriber_list_t *make_subscriber_list(int fd)
{
	subscriber_list_t *sb = malloc(sizeof(subscriber_list_t));
	sb->prev = sb->next = NULL;
	sb->fd = fd;
	sb->stream = fdopen(fd, "w");
	if (sb->stream == NULL) {
		warn("Can't open subscriber %i\n", fd);
		close(fd);
		free(sb);
		return NULL;
	}
	return sb;
}

void remove_subscriber(subscriber_list_t *sb)
{
	if (sb == NULL)
		return;
	subscriber_list_t *a = sb->prev;
	subscriber_list_t *b = sb->next;
	if (a != NULL)
		a->next = b;
	if (b != NULL)
		b->prev = a;
	if (sb == subscribe_head)
		subscribe_head = b;
	if (sb == subscribe_tail)
		subscribe_tail = a;
	fclose(sb->stream);
	free(sb);
}

void add_subscriber(int fd)
{
	subscriber_list_t *sb = make_subscriber_list(fd);
	if (sb == NULL)
		return;
	if (subscribe_head == NULL) {
		subscribe_head = subscribe_tail = sb;
	} else {
		subscribe_tail->next = sb;
		sb->prev = subscribe_tail;
		subscribe_tail = sb;
	}
	feed_subscriber(sb);
}

void feed_subscriber(subscriber_list_t *sb)
{
	fprintf(sb->stream, "%s", status_prefix);
	bool urgent = false;
	for (monitor_t *m = mon_head; m != NULL; m = m->next) {
		fprintf(sb->stream, "%c%s:", (mon == m ? 'M' : 'm'), m->name);
		for (desktop_t *d = m->desk_head; d != NULL; d = d->next, urgent = false) {
			for (node_t *n = first_extrema(d->root); n != NULL && !urgent; n = next_leaf(n, d->root))
				urgent |= n->client->urgent;
			char c = (urgent ? 'u' : (d->root == NULL ? 'f' : 'o'));
			if (m->desk == d)
				c = toupper(c);
			fprintf(sb->stream, "%c%s:", c, d->name);
		}
	}
	if (mon != NULL && mon->desk != NULL)
		fprintf(sb->stream, "L%s", (mon->desk->layout == LAYOUT_TILED ? "tiled" : "monocle"));
	fprintf(sb->stream, "%s", "\n");
	int ret = fflush(sb->stream);
	if (ret != 0)
		remove_subscriber(sb);
}
