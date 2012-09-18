#ifndef _BSPWM_H
#define _BSPWM_H

#include "types.h"

xcb_connection_t *dpy;
int default_screen, screen_width, screen_height;
unsigned int num_clients;
uint32_t num_desktops;
xcb_screen_t *screen;
xcb_rectangle_t root_rect;
split_mode_t split_mode;
direction_t split_dir;
desktop_t *desk;
desktop_t *last_desk;
desktop_t *desk_head;
desktop_t *desk_tail;
rule_t *rule_head;
bool running;

int register_events(void);
void handle_zombie(int);
void setup(int);
void quit(void);

#endif
