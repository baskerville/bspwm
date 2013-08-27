#ifndef _MESSAGES_H
#define _MESSAGES_H

#include "types.h"

#define OPT_CHR  '-'
#define CAT_CHR  '.'
#define EQL_TOK  "="

bool handle_message(char *, int, char *);
bool process_message(char **, int, char *);
bool cmd_window(char **, int);
bool cmd_desktop(char **, int);
bool cmd_monitor(char **, int);
bool cmd_query(char **, int, char *);
bool cmd_rule(char **, int, char *);
bool cmd_pointer(char **, int);
bool cmd_control(char **, int);
bool cmd_restore(char **, int);
bool cmd_config(char **, int, char *);
bool cmd_quit(char **, int);
bool get_setting(char *, char *);
bool set_setting(char *, char *);
bool parse_bool(char *, bool *);
bool parse_layout(char *, layout_t *);
bool parse_direction(char *, direction_t *);
bool parse_cycle_direction(char *, cycle_dir_t *);
bool parse_circulate_direction(char *, circulate_dir_t *);
bool parse_flip(char *, flip_t *);
bool parse_fence_move(char *, fence_move_t *);
bool parse_pointer_action(char *, pointer_action_t *);
bool parse_degree(char *, int *);
bool parse_window_id(char *, long int *);

#endif
