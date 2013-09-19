#ifndef _DESKTOP_H
#define _DESKTOP_H

#define DEFAULT_DESK_NAME    "Desktop"
#define WINDOW_GAP           6

desktop_t *make_desktop(const char *);
void insert_desktop(monitor_t *, desktop_t *);
void add_desktop(monitor_t *, desktop_t *);
void empty_desktop(desktop_t *);
void unlink_desktop(monitor_t *, desktop_t *);
void remove_desktop(monitor_t *, desktop_t *);
void swap_desktops(monitor_t *, desktop_t *, desktop_t *);
void transfer_desktop(monitor_t *, monitor_t *, desktop_t *);
void show_desktop(desktop_t *);
void hide_desktop(desktop_t *);

#endif
