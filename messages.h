#ifndef _MESSAGES_H
#define _MESSAGES_H

#include "types.h"

void process_message(char*, char*);
void get_setting(char*, char*);
void set_setting(char*, char*, char*);
bool parse_bool(char *, bool *);
bool parse_layout(char *, layout_t *);
bool parse_direction(char *, direction_t *);
bool parse_nearest_argument(char *, nearest_arg_t *);
bool parse_cycle_direction(char *, cycle_dir_t *);
bool parse_skip_client(char *, skip_client_t *);
bool parse_corner(char *, corner_t *);
bool parse_rotate(char *, rotate_t *);
bool parse_fence_move(char *, fence_move_t *);

#endif
