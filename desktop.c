#include <stdlib.h>
#include "bspwm.h"
#include "ewmh.h"
#include "history.h"
#include "monitor.h"
#include "query.h"
#include "tree.h"
#include "window.h"
#include "desktop.h"

void focus_desktop(monitor_t *m, desktop_t *d)
{
    focus_monitor(m);

    if (d == mon->desk)
        return;

    PRINTF("focus desktop %s\n", d->name);

    show_desktop(d);
    hide_desktop(mon->desk);

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

    if (d != dd && md->desk == d)
        show_desktop(d);

    history_transfer_desktop(md, d);

    ewmh_update_wm_desktops();
    ewmh_update_desktop_names();
    ewmh_update_current_desktop();
    put_status();
}

desktop_t *make_desktop(const char *name)
{
    desktop_t *d = malloc(sizeof(desktop_t));
    if (name == NULL)
        snprintf(d->name, sizeof(d->name), "%s%d", DEFAULT_DESK_NAME, ++desktop_uid);
    else
        snprintf(d->name, sizeof(d->name), "%s", name);
    d->layout = LAYOUT_TILED;
    d->prev = d->next = NULL;
    d->root = d->focus = NULL;
    d->window_gap = WINDOW_GAP;
    d->border_width = BORDER_WIDTH;
    d->tags_field = 1;
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
}

void unlink_desktop(monitor_t *m, desktop_t *d)
{
    desktop_t *prev = d->prev;
    desktop_t *next = d->next;
    desktop_t *last_desk = history_get_desktop(m, d);
    if (prev != NULL)
        prev->next = next;
    if (next != NULL)
        next->prev = prev;
    if (m->desk_head == d)
        m->desk_head = next;
    if (m->desk_tail == d)
        m->desk_tail = prev;
    if (m->desk == d)
        m->desk = (last_desk == NULL ? (prev == NULL ? next : prev) : last_desk);
    d->prev = d->next = NULL;
}

void remove_desktop(monitor_t *m, desktop_t *d)
{
    PRINTF("remove desktop %s\n", d->name);

    unlink_desktop(m, d);
    history_remove(d, NULL);
    empty_desktop(d);
    free(d);
    num_desktops--;
    ewmh_update_number_of_desktops();
    ewmh_update_desktop_names();
    put_status();
}

void merge_desktops(monitor_t *ms, desktop_t *ds, monitor_t *md, desktop_t *dd)
{
    if (ds == NULL || dd == NULL || ds == dd)
        return;
    node_t *n = first_extrema(ds->root);
    while (n != NULL) {
        node_t *next = next_leaf(n, ds->root);
        transfer_node(ms, ds, n, md, dd, dd->focus);
        n = next;
    }
}

void swap_desktops(monitor_t *m1, desktop_t *d1, monitor_t *m2, desktop_t *d2)
{
    if (d1 == NULL || d2 == NULL || d1 == d2)
        return;

    PRINTF("swap desktops %s %s\n", d1->name, d2->name);

    bool d1_focused = (m1->desk == d1);
    bool d2_focused = (m2->desk == d2);

    if (m1 != m2) {
        if (m1->desk == d1)
            m1->desk = d2;
        if (m1->desk_head == d1)
            m1->desk_head = d2;
        if (m1->desk_tail == d1)
            m1->desk_tail = d2;
        if (m2->desk == d2)
            m2->desk = d1;
        if (m2->desk_head == d2)
            m2->desk_head = d1;
        if (m2->desk_tail == d2)
            m2->desk_tail = d1;
    } else {
        if (m1->desk_head == d1)
            m1->desk_head = d2;
        else if (m1->desk_head == d2)
            m1->desk_head = d1;
        if (m1->desk_tail == d1)
            m1->desk_tail = d2;
        else if (m1->desk_tail == d2)
            m1->desk_tail = d1;
    }

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

    if (m1 != m2) {
        for (node_t *n = first_extrema(d1->root); n != NULL; n = next_leaf(n, d1->root))
            fit_monitor(m2, n->client);
        for (node_t *n = first_extrema(d2->root); n != NULL; n = next_leaf(n, d2->root))
            fit_monitor(m1, n->client);
        history_swap_desktops(m1, d1, m2, d2);
        arrange(m1, d2);
        arrange(m2, d1);
        if (d1_focused && !d2_focused) {
            hide_desktop(d1);
            show_desktop(d2);
        } else if (!d1_focused && d2_focused) {
            show_desktop(d1);
            hide_desktop(d2);
        }
    }

    update_input_focus();
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
        if (is_visible(d, n))
            window_show(n->client->window);
}

void hide_desktop(desktop_t *d)
{
    if (!visible)
        return;
    for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root))
        if (is_visible(d, n))
            window_hide(n->client->window);
}

bool is_urgent(desktop_t *d)
{
    for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root))
        if (n->client->urgent)
            return true;
    return false;
}
