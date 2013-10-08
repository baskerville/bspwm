#ifndef MESSAGES_H
#define MESSAGES_H

#include "types.h"

#define OPT_CHR  '-'
#define CAT_CHR  '.'
#define EQL_TOK  "="

bool handle_message(char *msg, int msg_len, char *rsp);
bool process_message(char **args, int num, char *rsp);
bool cmd_window(char **args, int num);
bool cmd_desktop(char **args, int num);
bool cmd_monitor(char **args, int num);
bool cmd_tag(char **args, int num, char *rsp);
bool cmd_query(char **args, int num, char *rsp);
bool cmd_rule(char **args, int num, char *rsp);
bool cmd_pointer(char **args, int num);
bool cmd_restore(char **args, int num);
bool cmd_control(char **args, int num);
bool cmd_config(char **args, int num, char *rsp);
bool cmd_quit(char **args, int num);
bool set_setting(coordinates_t loc, char *name, char *value);
bool get_setting(coordinates_t loc, char *name, char* rsp);
bool parse_bool(char *value, bool *b);
bool parse_layout(char *s, layout_t *l);
bool parse_direction(char *s, direction_t *d);
bool parse_cycle_direction(char *s, cycle_dir_t *d);
bool parse_circulate_direction(char *s, circulate_dir_t *d);
bool parse_flip(char *s, flip_t *f);
bool parse_fence_move(char *s, fence_move_t *m);
bool parse_pointer_action(char *s, pointer_action_t *a);
bool parse_degree(char *s, int *d);
bool parse_window_id(char *s, long int *i);
bool parse_bool_declaration(char *s, char **key, bool *value, alter_state_t *state);
bool parse_index(char *s, int *i);

#endif
