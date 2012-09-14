#include <string.h>
#include <xcb/xcb_ewmh.h>
#include "types.h"
#include "bspwm.h"
#include "settings.h"
#include "ewmh.h"

void ewmh_init(void)
{
    xcb_intern_atom_cookie_t *ewmh_cookies;
    ewmh_cookies = xcb_ewmh_init_atoms(dpy, &ewmh);
    xcb_ewmh_init_atoms_replies(&ewmh, ewmh_cookies, NULL);
}

void ewmh_update_wm_name(void)
{
    if (wm_name != NULL)
        xcb_ewmh_set_wm_name(&ewmh, screen->root, strlen(wm_name), wm_name);
}
