#ifndef _RULES_H
#define _RULES_H

#define MATCH_ANY "_"

rule_t *next_match(rule_t *, xcb_window_t);
bool is_match(rule_t *, xcb_window_t);
void handle_rules(xcb_window_t, bool *, bool *, bool *);

#endif
