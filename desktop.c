#include <stdlib.h>
#include <string.h>
#include "bspwm.h"
#include "desktop.h"
#include "monitor.h"
#include "tree.h"
#include "history.h"
#include "window.h"
#include "query.h"
#include "ewmh.h"

void select_desktop(monitor_t *m, desktop_t *d)
{
    select_monitor(m);

    if (d == mon->desk)
        return;

    PRINTF("select desktop %s\n", d->name);

    show_desktop(d);
    hide_desktop(mon->desk);

    mon->last_desk = mon->desk;
    mon->desk = d;

    ewmh_update_current_desktop();
    put_status();
}

desktop_t *closest_desktop(monitor_t *m, desktop_t *d, cycle_dir_t dir, desktop_select_t sel)
{
    desktop_t *f = (dir == CYCLE_PREV ? d->prev : d->next);
    if (f == NULL)
        f = (dir == CYCLE_PREV ? m->desk_tail : m->desk_head);

    while (f != d) {
        if (desktop_matches(f, sel))
            return f;
        f = (dir == CYCLE_PREV ? f->prev : f->next);
        if (f == NULL)
            f = (dir == CYCLE_PREV ? m->desk_tail : m->desk_head);
    }

    return NULL;
}

void change_layout(monitor_t *m, desktop_t *d, layout_t l)
{
    d->layout = l;
    arrange(m, d);
    if (d == mon->desk)
        put_status();
}

void transfer_desktop(monitor_t *ms, monitor_t *md, desktop_t *d)
{
    if (ms == md)
        return;

    desktop_t *dd = ms->desk;
    unlink_desktop(ms, d);
    insert_desktop(md, d);

    if (d == dd) {
        if (ms->desk != NULL)
            show_desktop(ms->desk);
        if (md->desk != d)
            hide_desktop(d);
    }

    for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root))
        fit_monitor(md, n->client);
    arrange(md, d);
    if (d != dd && md->desk == d) {
        show_desktop(d);
    }

    ewmh_update_wm_desktops();
    ewmh_update_desktop_names();
    ewmh_update_current_desktop();
    put_status();
}

desktop_t *make_desktop(const char *name)
{
    desktop_t *d = malloc(sizeof(desktop_t));
    if (name == NULL)
        snprintf(d->name, sizeof(d->name), "%s%02d", DEFAULT_DESK_NAME, ++desktop_uid);
    else
        strncpy(d->name, name, sizeof(d->name));
    d->layout = LAYOUT_TILED;
    d->prev = d->next = NULL;
    d->root = d->focus = NULL;
    d->history = make_focus_history();
    d->window_gap = WINDOW_GAP;
    return d;
}

void insert_desktop(monitor_t *m, desktop_t *d)
{
    if (m->desk == NULL) {
        m->desk = d;
        m->desk_head = d;
        m->desk_tail = d;
    } else {
        m->desk_tail->next = d;
        d->prev = m->desk_tail;
        m->desk_tail = d;
    }
}

void add_desktop(monitor_t *m, desktop_t *d)
{
    PRINTF("add desktop %s\n", d->name);

    insert_desktop(m, d);
    num_desktops++;
    ewmh_update_number_of_desktops();
    ewmh_update_desktop_names();
    put_status();
}

void empty_desktop(desktop_t *d)
{
    destroy_tree(d->root);
    d->root = d->focus = NULL;
    empty_history(d->history);
}

void unlink_desktop(monitor_t *m, desktop_t *d)
{
    desktop_t *prev = d->prev;
    desktop_t *next = d->next;
    if (prev != NULL)
        prev->next = next;
    if (next != NULL)
        next->prev = prev;
    if (m->desk_head == d)
        m->desk_head = next;
    if (m->desk_tail == d)
        m->desk_tail = prev;
    if (m->last_desk == d)
        m->last_desk = NULL;
    if (m->desk == d)
        m->desk = (m->last_desk == NULL ? (prev == NULL ? next : prev) : m->last_desk);
    d->prev = d->next = NULL;
}

void remove_desktop(monitor_t *m, desktop_t *d)
{
    PRINTF("remove desktop %s\n", d->name);

    unlink_desktop(m, d);
    empty_desktop(d);
    free(d);
    num_desktops--;
    ewmh_update_number_of_desktops();
    ewmh_update_desktop_names();
    put_status();
}

void swap_desktops(monitor_t *m, desktop_t *d1, desktop_t *d2)
{
    if (d1 == NULL || d2 == NULL || d1 == d2)
        return;

    if (m->desk_head == d1)
        m->desk_head = d2;
    else if (m->desk_head == d2)
        m->desk_head = d1;
    if (m->desk_tail == d1)
        m->desk_tail = d2;
    else if (m->desk_tail == d2)
        m->desk_tail = d1;

    desktop_t *p1 = d1->prev;
    desktop_t *n1 = d1->next;
    desktop_t *p2 = d2->prev;
    desktop_t *n2 = d2->next;

    if (p1 != NULL && p1 != d2)
        p1->next = d2;
    if (n1 != NULL && n1 != d2)
        n1->prev = d2;
    if (p2 != NULL && p2 != d1)
        p2->next = d1;
    if (n2 != NULL && n2 != d1)
        n2->prev = d1;

    d1->prev = p2 == d1 ? d2 : p2;
    d1->next = n2 == d1 ? d2 : n2;
    d2->prev = p1 == d2 ? d1 : p1;
    d2->next = n1 == d2 ? d1 : n1;

    ewmh_update_wm_desktops();
    ewmh_update_desktop_names();
    ewmh_update_current_desktop();
    put_status();
}

void show_desktop(desktop_t *d)
{
    if (!visible)
        return;
    for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root))
        window_show(n->client->window);
}

void hide_desktop(desktop_t *d)
{
    if (!visible)
        return;
    for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root))
        window_hide(n->client->window);
}

bool is_urgent(desktop_t *d)
{
    for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root))
        if (n->client->urgent)
            return true;
    return false;
}
