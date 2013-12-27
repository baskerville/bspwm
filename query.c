/* * Copyright (c) 2012-2013 Bastien Dejean
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include "bspwm.h"
#include "desktop.h"
#include "history.h"
#include "messages.h"
#include "monitor.h"
#include "tree.h"
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
                snprintf(line, sizeof(line), "%s %ux%u%+i%+i", m->name, m->rectangle.width, m->rectangle.height, m->rectangle.x, m->rectangle.y);
                strncat(rsp, line, REMLEN(rsp));
                if (m == mon)
                    strncat(rsp, " *", REMLEN(rsp));
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
            snprintf(line, sizeof(line), "%s %u %i %i,%i,%i,%i %c %c", d->name, d->border_width, d->window_gap, d->top_padding, d->right_padding, d->bottom_padding, d->left_padding, (d->layout == LAYOUT_TILED ? 'T' : 'M'), (d->floating ? 'f' : '-'));
            strncat(rsp, line, REMLEN(rsp));
            if (d == m->desk)
                strncat(rsp, " *", REMLEN(rsp));
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
        snprintf(line, sizeof(line), "%c %s 0x%X %u %ux%u%+i%+i %c %c%c%c%c%c%c%c%c%c", (n->birth_rotation == 90 ? 'a' : (n->birth_rotation == 270 ? 'c' : 'm')), c->class_name, c->window, c->border_width, c->floating_rectangle.width, c->floating_rectangle.height, c->floating_rectangle.x, c->floating_rectangle.y, (n->split_dir == DIR_UP ? 'U' : (n->split_dir == DIR_RIGHT ? 'R' : (n->split_dir == DIR_DOWN ? 'D' : 'L'))), (c->floating ? 'f' : '-'), (c->pseudo_tiled ? 'd' : '-'), (c->transient ? 't' : '-'), (c->fullscreen ? 'F' : '-'), (c->urgent ? 'u' : '-'), (c->locked ? 'l' : '-'), (c->sticky ? 's' : '-'), (c->private ? 'i' : '-'), (n->split_mode ? 'p' : '-'));
    } else {
        snprintf(line, sizeof(line), "%c %c %lf", (n->split_type == TYPE_HORIZONTAL ? 'H' : 'V'), (n->birth_rotation == 90 ? 'a' : (n->birth_rotation == 270 ? 'c' : 'm')), n->split_ratio);
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
    for (history_t *h = history_head; h != NULL; h = h->next) {
        if ((loc.monitor != NULL && h->loc.monitor != loc.monitor)
                || (loc.desktop != NULL && h->loc.desktop != loc.desktop))
            continue;
        xcb_window_t win = XCB_NONE;
        if (h->loc.node != NULL)
            win = h->loc.node->client->window;
        snprintf(line, sizeof(line), "%s %s 0x%X", h->loc.monitor->name, h->loc.desktop->name, win);
        strncat(rsp, line, REMLEN(rsp));
        strncat(rsp, "\n", REMLEN(rsp));
    }
}

void query_stack(char *rsp)
{
    char line[MAXLEN];
    for (stacking_list_t *s = stack_head; s != NULL; s = s->next) {
        snprintf(line, sizeof(line), "0x%X", s->node->client->window);
        strncat(rsp, line, REMLEN(rsp));
        strncat(rsp, "\n", REMLEN(rsp));
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
    client_select_t sel = {CLIENT_TYPE_ALL, CLIENT_CLASS_ALL, false, false, false};
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
        } else if (streq("urgent", tok)) {
            sel.urgent = true;
        } else if (streq("manual", tok)) {
            sel.manual = true;
        } else if (streq("local", tok)) {
            sel.local = true;
        }
    }

    dst->monitor = ref->monitor;
    dst->desktop = ref->desktop;
    dst->node = NULL;

    direction_t dir;
    cycle_dir_t cyc;
    history_dir_t hdi;
    if (parse_direction(desc, &dir)) {
        dst->node = nearest_neighbor(ref->monitor, ref->desktop, ref->node, dir, sel);
    } else if (parse_cycle_direction(desc, &cyc)) {
        dst->node = closest_node(ref->monitor, ref->desktop, ref->node, cyc, sel);
    } else if (parse_history_direction(desc, &hdi)) {
        history_find_node(hdi, ref, dst, sel);
    } else if (streq("last", desc)) {
        history_find_node(HISTORY_OLDER, ref, dst, sel);
    } else if (streq("biggest", desc)) {
        dst->node = find_biggest(ref->monitor, ref->desktop, ref->node, sel);
    } else if (streq("focused", desc)) {
        coordinates_t loc = {mon, mon->desk, mon->desk->focus};
        if (node_matches(&loc, ref, sel)) {
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
    desktop_select_t sel = {DESKTOP_STATUS_ALL, false, false};
    char *tok;
    while ((tok = strrchr(desc, CAT_CHR)) != NULL) {
        tok[0] = '\0';
        tok++;
        if (streq("free", tok)) {
            sel.status = DESKTOP_STATUS_FREE;
        } else if (streq("occupied", tok)) {
            sel.status = DESKTOP_STATUS_OCCUPIED;
        } else if (streq("urgent", tok)) {
            sel.urgent = true;
        } else if (streq("local", tok)) {
            sel.local = true;
        }
    }

    dst->desktop = NULL;

    cycle_dir_t cyc;
    history_dir_t hdi;
    int idx;
    if (parse_cycle_direction(desc, &cyc)) {
        dst->monitor = ref->monitor;
        dst->desktop = closest_desktop(ref->monitor, ref->desktop, cyc, sel);
    } else if (parse_history_direction(desc, &hdi)) {
        history_find_desktop(hdi, ref, dst, sel);
    } else if (streq("last", desc)) {
        history_find_desktop(HISTORY_OLDER, ref, dst, sel);
    } else if (streq("focused", desc)) {
        coordinates_t loc = {mon, mon->desk, NULL};
        if (desktop_matches(&loc, ref, sel)) {
            dst->monitor = mon;
            dst->desktop = mon->desk;
        }
    } else if (parse_index(desc, &idx)) {
        desktop_from_index(idx, dst);
    } else {
        locate_desktop(desc, dst);
    }

    return (dst->desktop != NULL);
}

bool monitor_from_desc(char *desc, coordinates_t *ref, coordinates_t *dst)
{
    desktop_select_t sel = {DESKTOP_STATUS_ALL, false, false};
    char *tok;
    while ((tok = strrchr(desc, CAT_CHR)) != NULL) {
        tok[0] = '\0';
        tok++;
        if (streq("free", tok)) {
            sel.status = DESKTOP_STATUS_FREE;
        } else if (streq("occupied", tok)) {
            sel.status = DESKTOP_STATUS_OCCUPIED;
        }
    }

    dst->monitor = NULL;

    direction_t dir;
    cycle_dir_t cyc;
    history_dir_t hdi;
    int idx;
    if (parse_direction(desc, &dir)) {
        dst->monitor = nearest_monitor(ref->monitor, dir, sel);
    } else if (parse_cycle_direction(desc, &cyc)) {
        dst->monitor = closest_monitor(ref->monitor, cyc, sel);
    } else if (parse_history_direction(desc, &hdi)) {
        history_find_monitor(hdi, ref, dst, sel);
    } else if (streq("last", desc)) {
        history_find_monitor(HISTORY_OLDER, ref, dst, sel);
    } else if (streq("primary", desc)) {
        if (pri_mon != NULL) {
            coordinates_t loc = {pri_mon, pri_mon->desk, NULL};
            if (desktop_matches(&loc, ref, sel))
                dst->monitor = pri_mon;
        }
    } else if (streq("focused", desc)) {
        coordinates_t loc = {mon, mon->desk, NULL};
        if (desktop_matches(&loc, ref, sel))
            dst->monitor = mon;
    } else if (parse_index(desc, &idx)) {
        monitor_from_index(idx, dst);
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

bool desktop_from_index(int i, coordinates_t *loc)
{
    for (monitor_t *m = mon_head; m != NULL; m = m->next)
        for (desktop_t *d = m->desk_head; d != NULL; d = d->next, i--)
            if (i == 1) {
                loc->monitor = m;
                loc->desktop = d;
                loc->node = NULL;
                return true;
            }
    return false;
}

bool monitor_from_index(int i, coordinates_t *loc)
{
    for (monitor_t *m = mon_head; m != NULL; m = m->next, i--)
        if (i == 1) {
            loc->monitor = m;
            loc->desktop = NULL;
            loc->node = NULL;
            return true;
        }
    return false;
}

bool node_matches(coordinates_t *loc, coordinates_t *ref, client_select_t sel)
{
    if (sel.type != CLIENT_TYPE_ALL &&
            is_tiled(loc->node->client)
            ? sel.type == CLIENT_TYPE_FLOATING
            : sel.type == CLIENT_TYPE_TILED)
        return false;

    if (sel.class != CLIENT_CLASS_ALL &&
            streq(loc->node->client->class_name, ref->node->client->class_name)
            ? sel.class == CLIENT_CLASS_DIFFER
            : sel.class == CLIENT_CLASS_EQUAL)
        return false;

    if (sel.manual && loc->node->split_mode != MODE_MANUAL)
        return false;

    if (sel.local && loc->desktop != ref->desktop)
        return false;

    if (sel.urgent && !loc->node->client->urgent)
        return false;

    return true;
}

bool desktop_matches(coordinates_t *loc, coordinates_t *ref, desktop_select_t sel)
{
    if (sel.status != DESKTOP_STATUS_ALL &&
            loc->desktop->root == NULL
            ? sel.status == DESKTOP_STATUS_OCCUPIED
            : sel.status == DESKTOP_STATUS_FREE)
        return false;

    if (sel.urgent && !is_urgent(loc->desktop))
        return false;

    if (sel.local && ref->monitor != loc->monitor)
        return false;

    return true;
}
