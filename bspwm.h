#ifndef _BSPWM_H
#define _BSPWM_H

#include "types.h"

#define ROOT_EVENT_MASK        (XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY)
#define CLIENT_EVENT_MASK      (XCB_EVENT_MASK_PROPERTY_CHANGE)
#define CLIENT_EVENT_MASK_FFP  (XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_POINTER_MOTION)

xcb_connection_t *dpy;
int default_screen, screen_width, screen_height;
uint32_t num_clients;
uint32_t num_desktops;
unsigned int num_monitors;
unsigned int monitor_uid;
unsigned int desktop_uid;
unsigned int client_uid;
unsigned int rule_uid;
xcb_screen_t *screen;
xcb_window_t root;
uint8_t root_depth;
FILE *status_fifo;

split_mode_t split_mode;
direction_t split_dir;
monitor_t *mon;
monitor_t *last_mon;
monitor_t *mon_head;
monitor_t *mon_tail;
rule_t *rule_head;
rule_t *rule_tail;

pointer_state_t *frozen_pointer;
xcb_window_t under_pointer;

int exit_status;

bool visible;
bool running;

void register_events(void);
void setup(void);
void cleanup(void);
void quit(void);

#endif
