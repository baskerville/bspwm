#ifndef QUERY_H
#define QUERY_H

typedef enum {
    DOMAIN_MONITOR,
    DOMAIN_DESKTOP,
    DOMAIN_WINDOW,
    DOMAIN_TREE,
    DOMAIN_HISTORY,
    DOMAIN_STACK
} domain_t;

void query_monitors(coordinates_t loc, domain_t dom, char *rsp);
void query_desktops(monitor_t *m, domain_t dom, coordinates_t loc, unsigned int depth, char *rsp);
void query_tree(desktop_t *d, node_t *n, char *rsp, unsigned int depth);
void query_history(coordinates_t loc, char *rsp);
void query_stack(char *rsp);
void query_windows(coordinates_t loc, char *rsp);
bool node_from_desc(char *desc, coordinates_t *ref, coordinates_t *dst);
bool desktop_from_desc(char *desc, coordinates_t *ref, coordinates_t *dst);
bool monitor_from_desc(char *desc, coordinates_t *ref, coordinates_t *dst);
bool locate_window(xcb_window_t win, coordinates_t *loc);
bool locate_desktop(char *name, coordinates_t *loc);
bool locate_monitor(char *name, coordinates_t *loc);
bool desktop_from_index(int i, coordinates_t *loc);
bool monitor_from_index(int i, coordinates_t *loc);
bool node_matches(node_t *c, node_t *t, client_select_t sel);
bool desktop_matches(desktop_t *t, desktop_select_t sel);

#endif
