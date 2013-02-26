#ifndef _MESSAGES_H
#define _MESSAGES_H

#include "types.h"

#define TOK_SEP  " "

void process_message(char*, char*);
void get_setting(char*, char*);
void set_setting(char*, char*, char*);
bool parse_bool(char *, bool *);
bool parse_layout(char *, layout_t *);
bool parse_direction(char *, direction_t *);
bool parse_nearest_argument(char *, nearest_arg_t *);
bool parse_cycle_direction(char *, cycle_dir_t *);
bool parse_circulate_direction(char *, circulate_dir_t *);
bool parse_list_option(char *, list_option_t *);
bool parse_send_option(char *, send_option_t *);
bool parse_skip_client(char *, skip_client_t *);
bool parse_skip_desktop(char *, skip_desktop_t *);
bool parse_rotate(char *, rotate_t *);
bool parse_flip(char *, flip_t *);
bool parse_fence_move(char *, fence_move_t *);
bool parse_pointer_action(char *, pointer_action_t *);

#endif
