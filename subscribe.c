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
#include <unistd.h>
#include <ctype.h>
#include <stdarg.h>
#include "bspwm.h"
#include "tree.h"
#include "settings.h"
#include "subscribe.h"
#include "json.h"

subscriber_list_t *make_subscriber_list(FILE *stream, unsigned int field)
{
	subscriber_list_t *sb = malloc(sizeof(subscriber_list_t));
	sb->prev = sb->next = NULL;
	sb->stream = stream;
	sb->field = field;
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

void add_subscriber(FILE *stream, unsigned int field)
{
	subscriber_list_t *sb = make_subscriber_list(stream, field);
	if (subscribe_head == NULL) {
		subscribe_head = subscribe_tail = sb;
	} else {
		subscribe_tail->next = sb;
		sb->prev = subscribe_tail;
		subscribe_tail = sb;
	}
}

bool exists_subscriber(subscriber_mask_t mask)
{
	for (subscriber_list_t *sb = subscribe_head; sb != NULL; sb = sb->next) {
		if (sb->field & mask)
			return true;
	}
	return false;
}

void put_status(subscriber_mask_t mask, json_t *json)
{
	json_t *jkey = json_serialize_subscriber_mask_type(&mask);
	if (!json_is_string(jkey)) {
		warn("put_status failed: !json_is_string(jkey)\n");
		json_decref(json);
		return;
	}
	if (!json_is_object(json)) {
		warn("put_status failed: !json_is_object(json): %s\n", json_string_value(jkey));
		json_decref(jkey);
		return;
	}

	subscriber_list_t *sb = subscribe_head;
	subscriber_list_t *next;
	int ret;
	while (sb) {
		next = sb->next;
		if (sb->field & mask) {
			if (sb->field != mask) {
				json_t *jobj = json_object();
				if (json_object_set(jobj, json_string_value(jkey), json) == -1) {
					json_decref(jobj);
					warn("put_status failed: json_object_set: %s\n", json_string_value(jkey));
					break;
				}
				ret = json_dumpf(jobj, sb->stream, JSON_COMPACT | JSON_PRESERVE_ORDER);
				json_decref(jobj);
			} else {
				ret = json_dumpf(json, sb->stream, JSON_COMPACT | JSON_PRESERVE_ORDER);
			}
			if (ret == -1 || fprintf(sb->stream, "\n") < 0 || fflush(sb->stream) != 0)
				remove_subscriber(sb);
		}
		sb = next;
	}
	json_decref(jkey);
	json_decref(json);
}
