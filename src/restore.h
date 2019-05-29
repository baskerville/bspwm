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

#ifndef BSPWM_RESTORE_H
#define BSPWM_RESTORE_H

#include "jsmn.h"

bool restore_state(const char *file_path);
monitor_t *restore_monitor(jsmntok_t **t, char *json);
desktop_t *restore_desktop(jsmntok_t **t, char *json);
node_t *restore_node(jsmntok_t **t, char *json);
presel_t *restore_presel(jsmntok_t **t, char *json);
client_t *restore_client(jsmntok_t **t, char *json);
void restore_rectangle(xcb_rectangle_t *r, jsmntok_t **t, char *json);
void restore_constraints(constraints_t *c, jsmntok_t **t, char *json);
void restore_padding(padding_t *p, jsmntok_t **t, char *json);
void restore_history(jsmntok_t **t, char *json);
void restore_subscribers(jsmntok_t **t, char *json);
void restore_subscriber(subscriber_list_t *s, jsmntok_t **t, char *json);
void restore_coordinates(coordinates_t *loc, jsmntok_t **t, char *json);
void restore_stack(jsmntok_t **t, char *json);
bool keyeq(char *s, jsmntok_t *key, char *json);

#endif
