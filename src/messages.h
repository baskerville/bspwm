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

#define NOD_NEA "node %s: Not enough arguments.\n"
#define NOD_IA  "node %s: Invalid argument: '%s'.\n"
#define NOD_TC  "node %s: Trailing commands.\n"
#define NOD_MA  "node: Missing arguments.\n"
#define NOD_MC  "node: Missing commands.\n"
#define NOD_UC  "node: Unknown command: '%s'.\n"

#define DES_NEA "desktop %s: Not enough arguments.\n"
#define DES_IA  "desktop %s: Invalid argument: '%s'.\n"
#define DES_TC  "desktop %s: Trailing commands.\n"
#define DES_MA  "desktop: Missing arguments.\n"
#define DES_MC  "desktop: Missing commands.\n"
#define DES_UC  "desktop: Unknown command: '%s'.\n"

#define MON_NEA "monitor %s: Not enough arguments.\n"
#define MON_IA  "monitor %s: Invalid argument: '%s'.\n"
#define MON_TC  "monitor %s: Trailing commands.\n"
#define MON_MA  "monitor: Missing arguments.\n"
#define MON_MC  "monitor: Missing commands.\n"
#define MON_UC  "monitor: Unknown command: '%s'.\n"

#define RUL_NEA "rule %s: Not enough arguments.\n"
#define RUL_MC  "rule: Missing commands.\n"
#define RUL_UC  "rule: Unknown command: '%s'.\n"

#define WM_NEA  "wm %s: Not enough arguments.\n"
#define WM_IA   "wm %s: Invalid argument: '%s'.\n"
#define WM_MC   "wm: Missing commands.\n"
#define WM_UC   "wm: Unknown command: '%s'.\n"

#define SUB_NEA "subscribe %s: Not enough arguments.\n"
#define SUB_IA  "subscribe %s: Invalid argument: '%s'.\n"

#define CFG_NEA "config %s: Not enough arguments.\n"
#define CFG_IV  "config: %s: Invalid value: '%s'.\n"
#define CFG_MA  "config: Missing arguments.\n"
#define CFG_UO  "config: Unknown option: '%s'.\n"
#define CFG_US  "config: Unknown setting: '%s'.\n"

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
