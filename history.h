#ifndef HISTORY_H
#define HISTORY_H

#include "types.h"

history_t *make_history(monitor_t *m, desktop_t *d, node_t *n);
void history_add(monitor_t *m, desktop_t *d, node_t *n);
void history_transfer_node(monitor_t *m, desktop_t *d, node_t *n);
void history_transfer_desktop(monitor_t *m, desktop_t *d);
void history_swap_nodes(monitor_t *m1, desktop_t *d1, node_t *n1, monitor_t *m2, desktop_t *d2, node_t *n2);
void history_swap_desktops(monitor_t *m1, desktop_t *d1, monitor_t *m2, desktop_t *d2);
void history_remove(desktop_t *d, node_t *n);
void empty_history(void);
node_t *history_get_node(desktop_t *d, node_t *n);
desktop_t *history_get_desktop(monitor_t *m, desktop_t *d);
monitor_t *history_get_monitor(monitor_t *m);
bool history_last_node(node_t *n, client_select_t sel, coordinates_t *loc);
bool history_last_desktop(desktop_t *d, desktop_select_t sel, coordinates_t *loc);
bool history_last_monitor(monitor_t *m, desktop_select_t sel, coordinates_t *loc);
int history_rank(desktop_t *d, node_t *n);

#endif
