#ifndef _EWMH_H
#define _EWMH_H

#include <xcb/xcb_ewmh.h>

xcb_ewmh_connection_t ewmh;

void ewmh_init(void);
void ewmh_update_wm_name(void);

#endif
