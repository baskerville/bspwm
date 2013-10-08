#ifndef BSPWM_EWMH_H
#define BSPWM_EWMH_H

#include <xcb/xcb_ewmh.h>

xcb_ewmh_connection_t *ewmh;

void ewmh_init(void);
void ewmh_update_active_window(void);
void ewmh_update_number_of_desktops(void);
uint32_t ewmh_get_desktop_index(desktop_t *d);
bool ewmh_locate_desktop(uint32_t i, coordinates_t *loc);
void ewmh_update_current_desktop(void);
void ewmh_set_wm_desktop(node_t *n, desktop_t *d);
void ewmh_update_wm_desktops(void);
void ewmh_update_desktop_names(void);
void ewmh_update_client_list(void);
bool ewmh_wm_state_add(client_t *c, xcb_atom_t state);
bool ewmh_wm_state_remove(client_t *c, xcb_atom_t state);
void ewmh_set_supporting(xcb_window_t win);

#endif
