#ifndef BSPWM_DESKTOP_H
#define BSPWM_DESKTOP_H

#define DEFAULT_DESK_NAME    "Desktop"
#define WINDOW_GAP           6
#define BORDER_WIDTH         1

void focus_desktop(monitor_t *m, desktop_t *d);
desktop_t *closest_desktop(monitor_t *m, desktop_t *d, cycle_dir_t dir, desktop_select_t sel);
void change_layout(monitor_t *m, desktop_t *d, layout_t l);
void transfer_desktop(monitor_t *ms, monitor_t *md, desktop_t *d);
desktop_t *make_desktop(const char *name);
void insert_desktop(monitor_t *m, desktop_t *d);
void add_desktop(monitor_t *m, desktop_t *d);
void empty_desktop(desktop_t *d);
void unlink_desktop(monitor_t *m, desktop_t *d);
void remove_desktop(monitor_t *m, desktop_t *d);
void merge_desktops(monitor_t *ms, desktop_t *ds, monitor_t *md, desktop_t *dd);
void swap_desktops(monitor_t *m1, desktop_t *d1, monitor_t *m2, desktop_t *d2);
void show_desktop(desktop_t *d);
void hide_desktop(desktop_t *d);
bool is_urgent(desktop_t *d);

#endif
