#ifndef _EWMH_H
#define _EWMH_H

#include <xcb/xcb_ewmh.h>

xcb_ewmh_connection_t *ewmh;

void ewmh_init(void);
void ewmh_update_wm_name(void);
void ewmh_update_active_window(void);
void ewmh_update_number_of_desktops(void);
uint32_t ewmh_get_desktop_index(desktop_t *);
bool ewmh_locate_desktop(uint32_t, desktop_location_t *);
void ewmh_update_current_desktop(void);
void ewmh_set_wm_desktop(node_t *, desktop_t *);
void ewmh_update_desktop_names(void);
void ewmh_update_client_list(void);

#endif
