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
    xcb_window_t win = (desk->focus == NULL ? XCB_NONE : desk->focus->client->window);
    xcb_ewmh_set_active_window(ewmh, default_screen, win);
}

void ewmh_update_number_of_desktops(void)
{
    xcb_ewmh_set_number_of_desktops(ewmh, default_screen, num_desktops);
}

void ewmh_update_current_desktop(void)
{
   desktop_t *d = desk_head;
   unsigned int i = 0, cd;

   while (d != NULL && i < num_desktops) {
       if (desk == d)
           cd = i;
       i++;
       d = d->next;
   }

   xcb_ewmh_set_current_desktop(ewmh, default_screen, cd);
}

void ewmh_update_desktop_names(void)
{
   char names[MAXLEN];
   desktop_t *d = desk_head;
   unsigned int pos, i;

   pos = i = 0;

   while (d != NULL && i < num_desktops) {
       for (unsigned int j = 0; j < strlen(d->name); j++)
           names[pos + j] = d->name[j];
       pos += strlen(d->name);
       names[pos] = '\0';
       pos++;
       d = d->next;
       i++;
   }

   if (i != (num_desktops - 1))
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
    desktop_t *d = desk_head;
    unsigned int i = 0;
    while (d != NULL && i < num_clients) {
        node_t *n = first_extrema(d->root);
        while (n != NULL) {
            wins[i++] = n->client->window;
            n = next_leaf(n);
        }
        d = d->next;
    }
    if (i != (num_clients - 1))
        return;
    xcb_ewmh_set_client_list(ewmh, default_screen, num_clients, wins);
}
