#ifndef _RULE_H
#define _RULE_H

#define MATCH_ALL  "*"
#define LST_SEP    ","

rule_t *make_rule(void);
void add_rule(rule_t *r);
void remove_rule(rule_t *r);
void remove_rule_by_name(char *name);
bool remove_rule_by_index(int idx);
bool is_match(rule_t *r, xcb_window_t win);
void handle_rules(xcb_window_t win, monitor_t **m, desktop_t **d, unsigned int *tags_field, bool *floating, bool *fullscreen, bool *locked, bool *sticky, bool *follow, bool *transient, bool *takes_focus, bool *manage);
void list_rules(char *pattern, char *rsp);

#endif
