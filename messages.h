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
bool parse_direction(char *, direction_t *);
bool parse_fence_move(char *, fence_move_t *);

#endif
