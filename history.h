#ifndef _HISTORY_H
#define _HISTORY_H

#include "types.h"

focus_history_t *make_focus_history(void);
node_list_t *make_node_list(void);
void history_add(focus_history_t *, node_t *);
void history_remove(focus_history_t *, node_t *);
void empty_history(focus_history_t *);
node_t *history_get(focus_history_t *, int);
node_t *history_last(focus_history_t *, node_t *, client_select_t);
int history_rank(focus_history_t *, node_t *);

#endif
