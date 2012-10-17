#include <stdio.h>
#include <string.h>
#include <xcb/xcb_ewmh.h>
#include "types.h"
#include "bspwm.h"
#include "settings.h"
#include "tree.h"
#include "ewmh.h"

void ewmh_init(void)
{
    ewmh = (xcb_ewmh_connection_t *) malloc(sizeof(xcb_ewmh_connection_t));
    xcb_intern_atom_cookie_t *ewmh_cookies;
    ewmh_cookies = xcb_ewmh_init_atoms(dpy, ewmh);
    xcb_ewmh_init_atoms_replies(ewmh, ewmh_cookies, NULL);
}

void ewmh_update_wm_name(void)
{
    if (wm_name != NULL)
        xcb_ewmh_set_wm_name(ewmh, screen->root, strlen(wm_name), wm_name);
}

void ewmh_update_active_window(void)
{
    xcb_window_t win = (mon->desk->focus == NULL ? XCB_NONE : mon->desk->focus->client->window);
    xcb_ewmh_set_active_window(ewmh, default_screen, win);
}

void ewmh_update_number_of_desktops(void)
{
    xcb_ewmh_set_number_of_desktops(ewmh, default_screen, num_desktops);
}

uint32_t ewmh_get_desktop_index(desktop_t *d)
{
    uint32_t i = 0;
    for (monitor_t *m = mon_head; m != NULL; m = m->next)
        for (desktop_t *cd = m->desk_head; cd != NULL; cd = cd->next, i++)
            if (d == cd)
                return i;

    return 0;
}

void ewmh_update_current_desktop(void)
{
    uint32_t i = ewmh_get_desktop_index(mon->desk);
    xcb_ewmh_set_current_desktop(ewmh, default_screen, i);
}

void ewmh_set_wm_desktop(node_t *n, desktop_t *d)
{
    uint32_t i = ewmh_get_desktop_index(d);
    xcb_ewmh_set_wm_desktop(ewmh, n->client->window, i);
}

void ewmh_update_desktop_names(void)
{
    char names[MAXLEN];
    monitor_t *m = mon_head;
    unsigned int pos, i;
    pos = i = 0;

    while (m != NULL) {
        desktop_t *d = m->desk_head;

        while (d != NULL && i < num_desktops) {
            for (unsigned int j = 0; j < strlen(d->name); j++)
                names[pos + j] = d->name[j];
            pos += strlen(d->name);
            names[pos] = '\0';
            pos++;
            d = d->next;
            i++;
        }

        m = m->next;
    }

    if (i != num_desktops)
        return;

    pos--;

    xcb_ewmh_set_desktop_names(ewmh, default_screen, pos, names);
}

void ewmh_update_client_list(void)
{
    if (num_clients == 0) {
        xcb_ewmh_set_client_list(ewmh, default_screen, 0, NULL);
        return;
    }

    xcb_window_t wins[num_clients];
    unsigned int i = 0;

    for (monitor_t *m = mon_head; m != NULL; m = m->next)
        for (desktop_t *d = m->desk_head; d != NULL; d = d->next)
            for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n))
                wins[i++] = n->client->window;

    if (i != num_clients)
        return;

    xcb_ewmh_set_client_list(ewmh, default_screen, num_clients, wins);
}
