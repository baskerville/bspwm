#ifndef _BSPWM_H
#define _BSPWM_H

xcb_connection_t *dpy;
int default_screen, screen_width, screen_height;
unsigned int num_clients;
unsigned int num_desktops;
xcb_screen_t *screen;
split_mode_t split_mode;
direction_t split_dir;
desktop_t *desk;
desktop_t *last_desk;
desktop_t *desk_head;
desktop_t *desk_tail;
bool running;

enum { WM_PROTOCOLS, WM_DELETE_WINDOW, WM_COUNT };

xcb_atom_t wmatoms[WM_COUNT];

int register_events(void);
xcb_screen_t *screen_of_display(xcb_connection_t *, int);
void sigchld(int);
void setup(int);
void quit(void);

#endif
