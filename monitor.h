#ifndef _MONITOR_H
#define _MONITOR_H

#define DEFAULT_MON_NAME     "MONITOR"

monitor_t *make_monitor(xcb_rectangle_t rect);
monitor_t *find_monitor(char *name);
monitor_t *get_monitor_by_id(xcb_randr_output_t id);
void fit_monitor(monitor_t *m, client_t *c);
void update_root(monitor_t *m);
void focus_monitor(monitor_t *m);
monitor_t *add_monitor(xcb_rectangle_t rect);
void remove_monitor(monitor_t *m);
void merge_monitors(monitor_t *ms, monitor_t *md);
void swap_monitors(monitor_t *m1, monitor_t *m2);
monitor_t *closest_monitor(monitor_t *m, cycle_dir_t dir, desktop_select_t sel);
monitor_t *nearest_monitor(monitor_t *m, direction_t dir, desktop_select_t sel);
bool import_monitors(void);

#endif
