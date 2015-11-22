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

#ifndef BSPWM_MESSAGES_H
#define BSPWM_MESSAGES_H

#include "types.h"
#include "subscribe.h"

int handle_message(char *msg, int msg_len, FILE *rsp);
int process_message(char **args, int num, FILE *rsp);
int cmd_window(char **args, int num);
int cmd_desktop(char **args, int num);
int cmd_monitor(char **args, int num);
int cmd_query(char **args, int num, FILE *rsp);
int cmd_rule(char **args, int num, FILE *rsp);
int cmd_pointer(char **args, int num);
int cmd_restore(char **args, int num);
int cmd_control(char **args, int num, FILE *rsp);
int cmd_config(char **args, int num, FILE *rsp);
int cmd_quit(char **args, int num);
int set_setting(coordinates_t loc, char *name, char *value);
int get_setting(coordinates_t loc, char *name, FILE* rsp);
bool parse_subscriber_mask(char *s, subscriber_mask_t *mask);
bool parse_bool(char *value, bool *b);
bool parse_layout(char *s, layout_t *l);
bool parse_client_state(char *s, client_state_t *t);
bool parse_stack_layer(char *s, stack_layer_t *l);
bool parse_direction(char *s, direction_t *d);
bool parse_cycle_direction(char *s, cycle_dir_t *d);
bool parse_circulate_direction(char *s, circulate_dir_t *d);
bool parse_history_direction(char *s, history_dir_t *d);
bool parse_flip(char *s, flip_t *f);
bool parse_pointer_action(char *s, pointer_action_t *a);
bool parse_child_polarity(char *s, child_polarity_t *p);
bool parse_degree(char *s, int *d);
bool parse_window_id(char *s, long int *i);
bool parse_bool_declaration(char *s, char **key, bool *value, alter_state_t *state);
bool parse_index(char *s, int *i);

#endif
