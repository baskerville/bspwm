#ifndef _BSPWM_H
#define _BSPWM_H

xcb_connection_t *dpy;
int default_screen, screen_width, screen_height;
xcb_screen_t *screen;
split_mode_t split_mode;
direction_t split_dir;
bool running;

char *WM_ATOM_NAME[]   = { "WM_PROTOCOLS", "WM_DELETE_WINDOW" };
char *NET_ATOM_NAME[]  = { "_NET_SUPPORTED", "_NET_WM_STATE_FULLSCREEN", "_NET_WM_STATE", "_NET_ACTIVE_WINDOW" };

enum { WM_PROTOCOLS, WM_DELETE_WINDOW, WM_COUNT };
enum { NET_SUPPORTED, NET_FULLSCREEN, NET_WM_STATE, NET_ACTIVE, NET_COUNT };

xcb_atom_t wmatoms[WM_COUNT], netatoms[NET_COUNT];

int register_events(void);
xcb_screen_t *screen_of_display(xcb_connection_t *, int);
void sigchld(int);
void setup(int);

#endif
