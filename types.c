#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "bspwm.h"
#include "settings.h"
#include "types.h"

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
    return m;
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

client_t *make_client(xcb_window_t win)
{
    client_t *c = malloc(sizeof(client_t));
    strncpy(c->class_name, MISSING_VALUE, sizeof(c->class_name));
    c->border_width = border_width;
    c->window = win;
    c->floating = c->transient = c->fullscreen = c->locked = c->urgent = false;
    return c;
}

rule_t *make_rule(void)
{
    rule_t *r = malloc(sizeof(rule_t));
    r->effect.floating = false;
    r->next = NULL;
    return r;
}

pointer_state_t *make_pointer_state(void)
{
    pointer_state_t *p = malloc(sizeof(pointer_state_t));
    return p;
}
