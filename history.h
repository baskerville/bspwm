#ifndef _HISTORY_H
#define _HISTORY_H

#include "types.h"

history_t *make_history(monitor_t *, desktop_t *, node_t *);
void history_add(monitor_t *, desktop_t *, node_t *);
void history_insert(monitor_t *, desktop_t *, node_t *);
void history_remove(desktop_t *, node_t *);
void history_transfer_node(monitor_t *, desktop_t *, node_t *);
void history_transfer_desktop(monitor_t *, desktop_t *);
void history_swap_nodes(monitor_t *, desktop_t *, node_t *, monitor_t *, desktop_t *, node_t *);
void history_swap_desktops(monitor_t *, desktop_t *, monitor_t *, desktop_t *);
node_t *history_get_node(desktop_t *, node_t *);
desktop_t *history_get_desktop(monitor_t *, desktop_t *);
monitor_t *history_get_monitor(monitor_t *);
bool *history_last_node(node_t *, client_select_t, coordinates_t *);
bool *history_last_desktop(desktop_t *, desktop_select_t, coordinates_t *);
bool *history_last_monitor(monitor_t *, desktop_select_t, coordinates_t *);
int history_rank(desktop_t *, node_t *);
void empty_history(void);

#endif
