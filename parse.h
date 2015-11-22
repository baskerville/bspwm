#ifndef BSPWM_PARSE_H
#define BSPWM_PARSE_H

#include "types.h"

#define OPT_CHR  '-'
#define CAT_CHR  '.'
#define EQL_TOK  "="

bool parse_bool(char *value, bool *b);
bool parse_split_type(char *s, split_type_t *t);
bool parse_split_mode(char *s, split_mode_t *m);
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
