#ifndef _RULE_H
#define _RULE_H

#define MATCH_ALL  "*"

rule_t *make_rule(void);
void add_rule(rule_t *);
void remove_rule(rule_t *);
void remove_rule_by_uid(unsigned int);
void prune_rules(desktop_t *);
rule_t *find_rule(unsigned int);
bool is_match(rule_t *, xcb_window_t);
void handle_rules(xcb_window_t, monitor_t **, desktop_t **, bool *, bool *, bool *, bool *, bool *, bool *, bool *);
void list_rules(char *, char *);

#endif
