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

void handle_message(char *msg, int msg_len, FILE *rsp);
void process_message(char **args, int num, FILE *rsp);
void cmd_node(char **args, int num, FILE *rsp);
void cmd_desktop(char **args, int num, FILE *rsp);
void cmd_monitor(char **args, int num, FILE *rsp);
void cmd_query(char **args, int num, FILE *rsp);
void cmd_rule(char **args, int num, FILE *rsp);
void cmd_wm(char **args, int num, FILE *rsp);
void cmd_subscribe(char **args, int num, FILE *rsp);
void cmd_quit(char **args, int num, FILE *rsp);
void cmd_config(char **args, int num, FILE *rsp);
void set_setting(coordinates_t loc, char *name, char *value, FILE *rsp);
void get_setting(coordinates_t loc, char *name, FILE* rsp);
void handle_failure(int code, char *src, char *val, FILE *rsp);
void fail(FILE *rsp, char *fmt, ...);

#endif
