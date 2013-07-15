#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "bspwm.h"
#include "tree.h"
#include "settings.h"
#include "messages.h"
#include "query.h"

void query_monitors(coordinates_t loc, domain_t dom, char *rsp)
{
    char line[MAXLEN];
    for (monitor_t *m = mon_head; m != NULL; m = m->next) {
        if (loc.monitor != NULL && m != loc.monitor)
            continue;
        if (dom != DOMAIN_DESKTOP) {
            if (dom == DOMAIN_MONITOR) {
                snprintf(line, sizeof(line), "%s\n", m->name);
                strncat(rsp, line, REMLEN(rsp));
                continue;
            } else {
                snprintf(line, sizeof(line), "%s %ux%u%+i%+i %i,%i,%i,%i", m->name, m->rectangle.width, m->rectangle.height, m->rectangle.x, m->rectangle.y, m->top_padding, m->right_padding, m->bottom_padding, m->left_padding);
                strncat(rsp, line, REMLEN(rsp));
                if (m == mon)
                    strncat(rsp, " #", REMLEN(rsp));
                else if (m == last_mon)
                    strncat(rsp, " ~", REMLEN(rsp));
                strncat(rsp, "\n", REMLEN(rsp));
            }
        }
        query_desktops(m, dom, loc, (dom == DOMAIN_DESKTOP ? 0 : 1), rsp);
    }
}

void query_desktops(monitor_t *m, domain_t dom, coordinates_t loc, unsigned int depth, char *rsp)
{
    char line[MAXLEN];
    for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
        if (loc.desktop != NULL && d != loc.desktop)
            continue;
        for (unsigned int i = 0; i < depth; i++)
            strncat(rsp, "  ", REMLEN(rsp));
        if (dom == DOMAIN_DESKTOP) {
            snprintf(line, sizeof(line), "%s\n", d->name);
            strncat(rsp, line, REMLEN(rsp));
            continue;
        } else {
            snprintf(line, sizeof(line), "%s %c", d->name, (d->layout == LAYOUT_TILED ? 'T' : 'M'));
            strncat(rsp, line, REMLEN(rsp));
            if (d == m->desk)
                strncat(rsp, " @", REMLEN(rsp));
            else if (d == m->last_desk)
                strncat(rsp, " ~", REMLEN(rsp));
            strncat(rsp, "\n", REMLEN(rsp));
        }
        query_tree(d, d->root, rsp, depth + 1);
    }
}

void query_tree(desktop_t *d, node_t *n, char *rsp, unsigned int depth)
{
    if (n == NULL)
        return;

    char line[MAXLEN];

    for (unsigned int i = 0; i < depth; i++)
        strncat(rsp, "  ", REMLEN(rsp));

    if (is_leaf(n)) {
        client_t *c = n->client;
        snprintf(line, sizeof(line), "%c %s %X %u %ux%u%+i%+i %c%c%c%c%c", (n->birth_rotation == 90 ? 'a' : (n->birth_rotation == 270 ? 'c' : 'm')), c->class_name, c->window, c->border_width, c->floating_rectangle.width, c->floating_rectangle.height, c->floating_rectangle.x, c->floating_rectangle.y, (c->floating ? 'f' : '-'), (c->transient ? 't' : '-'), (c->fullscreen ? 'F' : '-'), (c->urgent ? 'u' : '-'), (c->locked ? 'l' : '-'));
    } else {
        snprintf(line, sizeof(line), "%c %c %.2f", (n->split_type == TYPE_HORIZONTAL ? 'H' : 'V'), (n->birth_rotation == 90 ? 'a' : (n->birth_rotation == 270 ? 'c' : 'm')), n->split_ratio);
    }

    strncat(rsp, line, REMLEN(rsp));

    if (n == d->focus)
        strncat(rsp, " *", REMLEN(rsp));
    strncat(rsp, "\n", REMLEN(rsp));

    query_tree(d, n->first_child, rsp, depth + 1);
    query_tree(d, n->second_child, rsp, depth + 1);
}

void query_history(coordinates_t loc, char *rsp)
{
    char line[MAXLEN];
    for (monitor_t *m = mon_head; m != NULL; m = m->next) {
        if (loc.monitor != NULL && m != loc.monitor)
            continue;
        for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
            if (loc.desktop != NULL && d != loc.desktop)
                continue;
            snprintf(line, sizeof(line), "%s\n", d->name);
            strncat(rsp, line, REMLEN(rsp));
            for (node_list_t *a = d->history->tail; a != NULL; a = a->prev) {
                snprintf(line, sizeof(line), "  %X\n", a->node->client->window);
                strncat(rsp, line, REMLEN(rsp));
            }
        }
    }
}

void query_windows(coordinates_t loc, char *rsp)
{
    char line[MAXLEN];

    for (monitor_t *m = mon_head; m != NULL; m = m->next) {
        if (loc.monitor != NULL && m != loc.monitor)
            continue;
        for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
            if (loc.desktop != NULL && d != loc.desktop)
                continue;
            for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root)) {
                if (loc.node != NULL && n != loc.node)
                    continue;
                snprintf(line, sizeof(line), "0x%X\n", n->client->window);
                strncat(rsp, line, REMLEN(rsp));
            }
        }
    }
}

bool node_from_desc(char *desc, coordinates_t *ref, coordinates_t *dst)
{
    client_select_t sel;
    sel.type = CLIENT_TYPE_ALL;
    sel.class = CLIENT_CLASS_ALL;
    sel.mode = CLIENT_MODE_ALL;
    char *tok;
    while ((tok = strrchr(desc, CAT_CHR)) != NULL) {
        tok[0] = '\0';
        tok++;
        if (streq("tiled", tok)) {
            sel.type = CLIENT_TYPE_TILED;
        } else if (streq("floating", tok)) {
            sel.type = CLIENT_TYPE_FLOATING;
        } else if (streq("like", tok)) {
            sel.class = CLIENT_CLASS_EQUAL;
        } else if (streq("unlike", tok)) {
            sel.class = CLIENT_CLASS_DIFFER;
        } else if (streq("automatic", tok)) {
            sel.mode = CLIENT_MODE_AUTOMATIC;
        } else if (streq("manual", tok)) {
            sel.mode = CLIENT_MODE_MANUAL;
        }
    }

    dst->monitor = ref->monitor;
    dst->desktop = ref->desktop;
    dst->node = NULL;

    direction_t dir;
    cycle_dir_t cyc;
    if (parse_direction(desc, &dir)) {
        dst->node = nearest_neighbor(dst->desktop, ref->node, dir, sel);
    } else if (parse_cycle_direction(desc, &cyc)) {
        dst->node = closest_node(ref->desktop, ref->node, cyc, sel);
    } else if (streq("last", desc)) {
        int i = 1;
        do {
            dst->node = history_get(ref->desktop->history, i++);
        } while (dst->node != NULL && (dst->node == ref->node || !node_matches(ref->node, dst->node, sel)));
    } else if (streq("biggest", desc)) {
        dst->node = find_biggest(ref->desktop, ref->node, sel);
    } else if (streq("focused", desc)) {
        if (node_matches(ref->node, mon->desk->focus, sel)) {
            dst->monitor = mon;
            dst->desktop = mon->desk;
            dst->node = mon->desk->focus;
        }
    } else {
        long int wid;
        if (parse_window_id(desc, &wid))
            locate_window(wid, dst);
    }

    return (dst->node != NULL);
}

bool desktop_from_desc(char *desc, coordinates_t *ref, coordinates_t *dst)
{
    desktop_select_t sel;
    sel = DESKTOP_ALL;
    char *tok;
    while ((tok = strrchr(desc, CAT_CHR)) != NULL) {
        tok[0] = '\0';
        tok++;
        if (streq("free", tok)) {
            sel = DESKTOP_FREE;
        } else if (streq("occupied", tok)) {
            sel = DESKTOP_OCCUPIED;
        }
    }

    dst->desktop = NULL;

    cycle_dir_t cyc;
    if (parse_cycle_direction(desc, &cyc)) {
        dst->monitor = ref->monitor;
        dst->desktop = closest_desktop(ref->monitor, ref->desktop, cyc, sel);
    } else if (streq("last", desc)) {
        if (mon->last_desk != NULL && desktop_matches(mon->last_desk, sel)) {
            dst->monitor = mon;
            dst->desktop = mon->last_desk;
        }
    } else if (streq("focused", desc)) {
        if (desktop_matches(mon->desk, sel)) {
            dst->monitor = mon;
            dst->desktop = mon->desk;
        }
    } else {
        locate_desktop(desc, dst);
    }

    return (dst->desktop != NULL);
}

bool monitor_from_desc(char *desc, coordinates_t *ref, coordinates_t *dst)
{
    desktop_select_t sel;
    sel = DESKTOP_ALL;
    char *tok;
    while ((tok = strrchr(desc, CAT_CHR)) != NULL) {
        tok[0] = '\0';
        tok++;
        if (streq("free", tok)) {
            sel = DESKTOP_FREE;
        } else if (streq("occupied", tok)) {
            sel = DESKTOP_OCCUPIED;
        }
    }

    dst->monitor = NULL;

    direction_t dir;
    cycle_dir_t cyc;
    if (parse_direction(desc, &dir)) {
        dst->monitor = nearest_monitor(ref->monitor, dir, sel);
    } else if (parse_cycle_direction(desc, &cyc)) {
        dst->monitor = closest_monitor(ref->monitor, cyc, sel);
    } else if (streq("last", desc)) {
        if (last_mon != NULL && desktop_matches(last_mon->desk, sel)) {
            dst->monitor = last_mon;
        }
    } else if (streq("focused", desc)) {
        if (desktop_matches(mon->desk, sel)) {
            dst->monitor = mon;
        }
    } else {
        locate_monitor(desc, dst);
    }

    return (dst->monitor != NULL);
}

bool locate_window(xcb_window_t win, coordinates_t *loc)
{
    for (monitor_t *m = mon_head; m != NULL; m = m->next)
        for (desktop_t *d = m->desk_head; d != NULL; d = d->next)
            for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root))
                if (n->client->window == win) {
                    loc->monitor = m;
                    loc->desktop = d;
                    loc->node = n;
                    return true;
                }
    return false;
}

bool locate_desktop(char *name, coordinates_t *loc)
{
    for (monitor_t *m = mon_head; m != NULL; m = m->next)
        for (desktop_t *d = m->desk_head; d != NULL; d = d->next)
            if (streq(d->name, name)) {
                loc->monitor = m;
                loc->desktop = d;
                return true;
            }
    return false;
}

bool locate_monitor(char *name, coordinates_t *loc)
{
    for (monitor_t *m = mon_head; m != NULL; m = m->next)
        if (streq(m->name, name)) {
            loc->monitor = m;
            return true;
        }
    return false;
}
