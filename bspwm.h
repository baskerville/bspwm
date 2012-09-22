#ifndef _BSPWM_H
#define _BSPWM_H

#include "types.h"

#define ROOT_EVENT_MASK    (XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY)
#define CLIENT_EVENT_MASK  (XCB_EVENT_MASK_PROPERTY_CHANGE)
#define MOUSE_MODIFIER     XCB_MOD_MASK_4

xcb_connection_t *dpy;
int default_screen, screen_width, screen_height;
unsigned int num_clients;
uint32_t num_desktops;
xcb_screen_t *screen;
xcb_rectangle_t root_rect;
uint8_t root_depth;

split_mode_t split_mode;
direction_t split_dir;
desktop_t *desk;
desktop_t *last_desk;
desktop_t *desk_head;
desktop_t *desk_tail;
rule_t *rule_head;
pointer_state_t *frozen_pointer;

bool running;

void register_events(void);
void handle_zombie(int);
void setup(void);
void quit(void);

#endif
