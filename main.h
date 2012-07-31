#ifndef _MAIN_H
#define _MAIN_H

#define INPUT_FIFO      "/tmp/bspwm-input"
#define SELECT_TIMEOUT  300

xcb_connection_t *dpy;
int default_screen;
bool running;

#endif
