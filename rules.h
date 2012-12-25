#ifndef _RULES_H
#define _RULES_H

void add_rule(rule_t *);
bool is_match(rule_t *, xcb_window_t);
void handle_rules(xcb_window_t, monitor_t **, desktop_t **, bool *, bool *, bool *, bool *, bool *);
void list_rules(char *);

#endif
