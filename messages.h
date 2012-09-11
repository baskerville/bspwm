#ifndef _MESSAGES_H
#define _MESSAGES_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "types.h"

void process_message(char*, char*);
void get_setting(char*, char*);
void set_setting(char*, char*);
void split_ratio_cmd(char *);
bool is_bool(char *);
bool parse_bool(char *);
bool parse_layout(char *, layout_t *);
direction_t parse_direction(char *);
fence_move_t parse_fence_move(char *);

#endif
