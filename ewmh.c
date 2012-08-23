#include <xcb/xcb_ewmh.h>
#include "types.h"
#include "bspwm.h"
#include "ewmh.h"

void ewmh_init(void)
{
    xcb_intern_atom_cookie_t *ewmh_cookies;
    ewmh_cookies = xcb_ewmh_init_atoms(dpy, &ewmh);
    xcb_ewmh_init_atoms_replies(&ewmh, ewmh_cookies, NULL);
}
