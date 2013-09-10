#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "bspwm.h"
#include "window.h"
#include "rules.h"
#include "ewmh.h"
#include "settings.h"
#include "types.h"
#include "tree.h"

node_t *make_node(void)
{
    node_t *n = malloc(sizeof(node_t));
    n->parent = n->first_child = n->second_child = NULL;
    n->split_ratio = split_ratio;
    n->split_mode = MODE_AUTOMATIC;
    n->split_type = TYPE_VERTICAL;
    n->birth_rotation = 0;
    n->client = NULL;
    n->vacant = false;
    return n;
}

monitor_t *make_monitor(xcb_rectangle_t *rect)
{
    monitor_t *m = malloc(sizeof(monitor_t));
    snprintf(m->name, sizeof(m->name), "%s%02d", DEFAULT_MON_NAME, ++monitor_uid);
    m->prev = m->next = NULL;
    m->desk = m->last_desk = NULL;
    if (rect != NULL)
        m->rectangle = *rect;
    else
        warn("no rectangle was given for monitor '%s'\n", m->name);
    m->top_padding = m->right_padding = m->bottom_padding = m->left_padding = 0;
    m->wired = true;
    return m;
}

monitor_t *find_monitor(char *name)
{
    for (monitor_t *m = mon_head; m != NULL; m = m->next)
        if (streq(m->name, name))
            return m;
    return NULL;
}

monitor_t *get_monitor_by_id(xcb_randr_output_t id)
{
    for (monitor_t *m = mon_head; m != NULL; m = m->next)
        if (m->id == id)
            return m;
    return NULL;
}

monitor_t *add_monitor(xcb_rectangle_t *rect)
{
    monitor_t *m = make_monitor(rect);
    if (mon == NULL) {
        mon = m;
        mon_head = m;
        mon_tail = m;
    } else {
        mon_tail->next = m;
        m->prev = mon_tail;
        mon_tail = m;
    }
    num_monitors++;
    return m;
}

void remove_monitor(monitor_t *m)
{
    while (m->desk_head != NULL)
        remove_desktop(m, m->desk_head);
    monitor_t *prev = m->prev;
    monitor_t *next = m->next;
    if (prev != NULL)
        prev->next = next;
    if (next != NULL)
        next->prev = prev;
    if (mon_head == m)
        mon_head = next;
    if (mon_tail == m)
        mon_tail = prev;
    if (last_mon == m)
        last_mon = NULL;
    if (pri_mon == m)
        pri_mon = NULL;
    if (mon == m) {
        monitor_t *mm = (last_mon == NULL ? (prev == NULL ? next : prev) : last_mon);
        if (mm != NULL) {
            focus_node(mm, mm->desk, mm->desk->focus);
            last_mon = NULL;
        } else {
            mon = NULL;
        }
    }
    free(m);
    num_monitors--;
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
            desktop_show(ms->desk);
        if (md->desk != d)
            desktop_hide(d);
    }

    for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root))
        fit_monitor(md, n->client);
    arrange(md, d);
    if (d != dd && md->desk == d) {
        desktop_show(d);
    }

    ewmh_update_desktop_names();
    put_status();
}

void merge_monitors(monitor_t *ms, monitor_t *md)
{
    PRINTF("merge %s into %s\n", ms->name, md->name);

    desktop_t *d = ms->desk_head;
    while (d != NULL) {
        desktop_t *next = d->next;
        transfer_desktop(ms, md, d);
        d = next;
    }
}

void swap_monitors(monitor_t *m1, monitor_t *m2)
{
    if (m1 == NULL || m2 == NULL || m1 == m2)
        return;

    if (mon_head == m1)
        mon_head = m2;
    else if (mon_head == m2)
        mon_head = m1;
    if (mon_tail == m1)
        mon_tail = m2;
    else if (mon_tail == m2)
        mon_tail = m1;

    monitor_t *p1 = m1->prev;
    monitor_t *n1 = m1->next;
    monitor_t *p2 = m2->prev;
    monitor_t *n2 = m2->next;

    if (p1 != NULL && p1 != m2)
        p1->next = m2;
    if (n1 != NULL && n1 != m2)
        n1->prev = m2;
    if (p2 != NULL && p2 != m1)
        p2->next = m1;
    if (n2 != NULL && n2 != m1)
        n2->prev = m1;

    m1->prev = p2 == m1 ? m2 : p2;
    m1->next = n2 == m1 ? m2 : n2;
    m2->prev = p1 == m2 ? m1 : p1;
    m2->next = n1 == m2 ? m1 : n1;

    ewmh_update_desktop_names();
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

    ewmh_update_desktop_names();
    put_status();
}

client_t *make_client(xcb_window_t win)
{
    client_t *c = malloc(sizeof(client_t));
    strncpy(c->class_name, MISSING_VALUE, sizeof(c->class_name));
    c->border_width = border_width;
    c->window = win;
    c->floating = c->transient = c->fullscreen = c->locked = c->urgent = false;
    c->icccm_focus = false;
    xcb_icccm_get_wm_protocols_reply_t protocols;
    if (xcb_icccm_get_wm_protocols_reply(dpy, xcb_icccm_get_wm_protocols(dpy, win, ewmh->WM_PROTOCOLS), &protocols, NULL) == 1) {
        if (has_proto(WM_TAKE_FOCUS, &protocols))
            c->icccm_focus = true;
        xcb_icccm_get_wm_protocols_reply_wipe(&protocols);
    }
    return c;
}

rule_t *make_rule(void)
{
    rule_t *r = malloc(sizeof(rule_t));
    r->uid = ++rule_uid;
    r->effect.floating = false;
    r->effect.follow = false;
    r->effect.focus = false;
    r->effect.desc[0] = '\0';
    r->prev = NULL;
    r->next = NULL;
    return r;
}

pointer_state_t *make_pointer_state(void)
{
    pointer_state_t *p = malloc(sizeof(pointer_state_t));
    p->monitor = NULL;
    p->desktop = NULL;
    p->node = p->vertical_fence = p->horizontal_fence = NULL;
    p->client = NULL;
    p->window = XCB_NONE;
    return p;
}

focus_history_t *make_focus_history(void)
{
    focus_history_t *f = malloc(sizeof(focus_history_t));
    f->head = f->tail = NULL;
    return f;
}

node_list_t *make_node_list(void)
{
    node_list_t *n = malloc(sizeof(node_list_t));
    n->node = NULL;
    n->prev = n->next = NULL;
    n->latest = true;
    return n;
}

void history_add(focus_history_t *f, node_t *n)
{
    node_list_t *a = make_node_list();
    a->node = n;
    if (f->head == NULL) {
        f->head = f->tail = a;
    } else if (f->head->node != n) {
        for (node_list_t *b = f->head; b != NULL; b = b->next)
            if (b->node == n)
                b->latest = false;
        f->head->prev = a;
        a->next = f->head;
        f->head = a;
    } else {
        free(a);
    }
}

void history_remove(focus_history_t *f, node_t *n)
{
    /* in order to maintain the `latest` node list state,
       we remove node lists from head to tail */
    node_list_t *b = f->head;
    while (b != NULL) {
        if (b->node == n) {
            node_list_t *a = b->prev;
            node_list_t *c = b->next;
            if (a != NULL) {
                /* remove duplicate entries */
                while (c != NULL && c->node == a->node) {
                    node_list_t *d = c->next;
                    if (f->tail == c)
                        f->tail = f->head;
                    free(c);
                    c = d;
                }
                a->next = c;
            }
            if (c != NULL)
                c->prev = a;
            if (f->head == b)
                f->head = c;
            if (f->tail == b)
                f->tail = a;
            free(b);
            b = c;
        } else {
            b = b->next;
        }
    }
}

void empty_history(focus_history_t *f)
{
    node_list_t *a = f->head;
    while (a != NULL) {
        node_list_t *b = a->next;
        free(a);
        a = b;
    }
    f->head = f->tail = NULL;
}

node_t *history_get(focus_history_t *f, int i)
{
    node_list_t *a = f->head;
    while (a != NULL && i > 0) {
        a = a->next;
        i--;
    }
    if (a == NULL)
        return NULL;
    else
        return a->node;
}

node_t *history_last(focus_history_t *f, node_t *n, client_select_t sel)
{
    for (node_list_t *a = f->head; a != NULL; a = a->next) {
        if (!a->latest || a->node == n || !node_matches(n, a->node, sel))
            continue;
        return a->node;
    }
    return NULL;
}

int history_rank(focus_history_t *f, node_t *n)
{
    int i = 0;
    node_list_t *a = f->head;
    while (a != NULL && (!a->latest || a->node != n)) {
        a = a->next;
        i++;
    }
    if (a == NULL)
        return -1;
    else
        return i;
}
