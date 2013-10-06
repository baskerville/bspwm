#ifndef _BSPWM_H
#define _BSPWM_H

#include "types.h"

#define ROOT_EVENT_MASK        (XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY)
#define CLIENT_EVENT_MASK      (XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_FOCUS_CHANGE)
#define CLIENT_EVENT_MASK_FFP  (CLIENT_EVENT_MASK | XCB_EVENT_MASK_ENTER_WINDOW)

xcb_connection_t *dpy;
int default_screen, screen_width, screen_height;
uint32_t num_clients;
uint32_t num_desktops;
unsigned int num_monitors;
unsigned int monitor_uid;
unsigned int desktop_uid;
xcb_screen_t *screen;
xcb_window_t root;
uint8_t root_depth;
FILE *status_fifo;
char *status_prefix;
char config_path[MAXLEN];

monitor_t *mon;
monitor_t *mon_head;
monitor_t *mon_tail;
monitor_t *pri_mon;
history_t *history_head;
history_t *history_tail;
stack_t *stack_head;
stack_t *stack_tail;
rule_t *rule_head;
rule_t *rule_tail;

pointer_state_t *frozen_pointer;
xcb_window_t motion_recorder;
xcb_atom_t WM_TAKE_FOCUS;
xcb_atom_t WM_DELETE_WINDOW;
xcb_atom_t _BSPWM_FLOATING_WINDOW;
int exit_status;

bool visible;
bool auto_raise;
bool sticky_still;
bool running;
bool randr;

void init(void);
void setup(void);
void register_events(void);
void quit(void);
void cleanup(void);
void put_status(void);

#endif
