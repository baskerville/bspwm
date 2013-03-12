#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "bspwm.h"
#include "ewmh.h"
#include "settings.h"
#include "types.h"
#include "tree.h"

node_t *make_node(void)
{
    node_t *n = malloc(sizeof(node_t));
    n->parent = n->first_child = n->second_child = NULL;
    n->split_ratio = SPLIT_RATIO;
    n->split_type = TYPE_VERTICAL;
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
    return m;
}

monitor_t *find_monitor(char *name)
{
    for (monitor_t *m = mon_head; m != NULL; m = m->next)
        if (strcmp(m->name, name) == 0)
            return m;
    return NULL;
}

void add_monitor(xcb_rectangle_t *rect)
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
    if (m == mon_head)
        mon_head = next;
    if (m == mon_tail)
        mon_tail = prev;
    free(m);
    num_monitors--;
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
    d->root = d->focus = d->last_focus = NULL;
    return d;
}

void add_desktop(monitor_t *m, char *name)
{
    desktop_t *d = make_desktop(name);
    if (m->desk == NULL) {
        m->desk = d;
        m->desk_head = d;
        m->desk_tail = d;
    } else {
        m->desk_tail->next = d;
        d->prev = m->desk_tail;
        m->desk_tail = d;
    }
    num_desktops++;
    ewmh_update_number_of_desktops();
    ewmh_update_desktop_names();
    put_status();
}

void empty_desktop(desktop_t *d)
{
    destroy_tree(d->root);
    d->root = d->focus = d->last_focus = NULL;
}

void remove_desktop(monitor_t *m, desktop_t *d)
{
    empty_desktop(d);
    desktop_t *prev = d->prev;
    desktop_t *next = d->next;
    if (prev != NULL)
        prev->next = next;
    if (next != NULL)
        next->prev = prev;
    if (d == m->desk_head)
        m->desk_head = next;
    if (d == m->desk_tail)
        m->desk_tail = prev;
    free(d);
    num_desktops--;
}

client_t *make_client(xcb_window_t win)
{
    client_t *c = malloc(sizeof(client_t));
    strncpy(c->class_name, MISSING_VALUE, sizeof(c->class_name));
    c->uid = ++client_uid;
    c->border_width = border_width;
    c->window = win;
    c->floating = c->transient = c->fullscreen = c->locked = c->urgent = false;
    return c;
}

rule_t *make_rule(void)
{
    rule_t *r = malloc(sizeof(rule_t));
    r->uid = ++rule_uid;
    r->effect.floating = false;
    r->effect.follow = false;
    r->effect.monitor = NULL;
    r->effect.desktop = NULL;
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
