#ifndef _QUERY_H
#define _QUERY_H

typedef enum {
    DOMAIN_MONITOR,
    DOMAIN_DESKTOP,
    DOMAIN_WINDOW,
    DOMAIN_TREE,
    DOMAIN_HISTORY
} domain_t;

void query_monitors(coordinates_t, domain_t, char *);
void query_desktops(monitor_t *, domain_t, coordinates_t, unsigned int, char *);
void query_tree(desktop_t *, node_t *, char *, unsigned int);
void query_history(coordinates_t, char *);
void query_windows(coordinates_t, char *);
bool locate_window(xcb_window_t, coordinates_t *);
bool locate_desktop(char *, coordinates_t *);
bool locate_monitor(char *, coordinates_t *);
bool node_from_desc(char *, coordinates_t *, coordinates_t *);
bool desktop_from_desc(char *, coordinates_t *, coordinates_t *);
bool monitor_from_desc(char *, coordinates_t *, coordinates_t *);
bool desktop_from_index(int, coordinates_t *);
bool monitor_from_index(int, coordinates_t *);
bool node_matches(node_t *, node_t *, client_select_t);
bool desktop_matches(desktop_t *, desktop_select_t);

#endif
