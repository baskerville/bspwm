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

#include <ctype.h>
#include <string.h>
#include "bspwm.h"
#include "desktop.h"
#include "ewmh.h"
#include "history.h"
#include "monitor.h"
#include "window.h"
#include "query.h"
#include "stack.h"
#include "tree.h"
#include "settings.h"
#include "restore.h"

void restore_tree(char *file_path)
{
    if (file_path == NULL)
        return;

    FILE *snapshot = fopen(file_path, "r");
    if (snapshot == NULL) {
        warn("Restore tree: can't open file\n");
        return;
    }

    PUTS("restore tree");

    char line[MAXLEN];
    char name[MAXLEN];
    coordinates_t loc;
    monitor_t *m = NULL;
    desktop_t *d = NULL;
    node_t *n = NULL;
    unsigned int level, last_level = 0;

    while (fgets(line, sizeof(line), snapshot) != NULL) {
        unsigned int len = strlen(line);
        level = 0;

        while (level < len && isspace(line[level]))
            level++;

        if (level == 0) {
            int x, y;
            unsigned int w, h;
            char end = 0;
            name[0] = '\0';
            sscanf(line + level, "%s %ux%u%i%i %c", name, &w, &h, &x, &y, &end);
            m = find_monitor(name);
            if (m == NULL)
                continue;
            m->rectangle = (xcb_rectangle_t) {x, y, w, h};
            if (end != 0)
                mon = m;
        } else if (level == 2) {
            if (m == NULL)
                continue;
            int wg, top, right, bottom, left;
            unsigned int bw;
            char floating, layout = 0, end = 0;
            name[0] = '\0';
            loc.desktop = NULL;
            sscanf(line + level, "%s %u %i %i,%i,%i,%i %c %c %c", name, &bw, &wg, &top, &right, &bottom, &left, &layout, &floating, &end);
            locate_desktop(name, &loc);
            d = loc.desktop;
            if (d == NULL)
                continue;
            d->border_width = bw;
            d->window_gap = wg;
            d->top_padding = top;
            d->right_padding = right;
            d->bottom_padding = bottom;
            d->left_padding = left;
            if (layout == 'M')
                d->layout = LAYOUT_MONOCLE;
            else if (layout == 'T')
                d->layout = LAYOUT_TILED;
            d->floating = (floating == '-' ? false : true);
            if (end != 0)
                m->desk = d;
        } else {
            if (m == NULL || d == NULL)
                continue;
            node_t *birth = make_node();
            if (level == 4) {
                empty_desktop(d);
                d->root = birth;
            } else if (n != NULL) {
                if (level > last_level) {
                    n->first_child = birth;
                } else {
                    do {
                        n = n->parent;
                    } while (n != NULL && n->second_child != NULL);
                    if (n == NULL)
                        continue;
                    n->second_child = birth;
                }
                birth->parent = n;
            }
            n = birth;
            char br;
            if (isupper(line[level])) {
                char st;
                sscanf(line + level, "%c %c %lf", &st, &br, &n->split_ratio);
                if (st == 'H')
                    n->split_type = TYPE_HORIZONTAL;
                else if (st == 'V')
                    n->split_type = TYPE_VERTICAL;
            } else {
                client_t *c = make_client(XCB_NONE);
                num_clients++;
                char floating, transient, fullscreen, urgent, locked, sticky, private, sd, sm, end = 0;
                sscanf(line + level, "%c %s %X %u %hux%hu%hi%hi %c %c%c%c%c%c%c%c%c %c", &br, c->class_name, &c->window, &c->border_width, &c->floating_rectangle.width, &c->floating_rectangle.height, &c->floating_rectangle.x, &c->floating_rectangle.y, &sd, &floating, &transient, &fullscreen, &urgent, &locked, &sticky, &private, &sm, &end);
                c->floating = (floating == '-' ? false : true);
                c->transient = (transient == '-' ? false : true);
                c->fullscreen = (fullscreen == '-' ? false : true);
                c->urgent = (urgent == '-' ? false : true);
                c->locked = (locked == '-' ? false : true);
                c->sticky = (sticky == '-' ? false : true);
                c->private = (private == '-' ? false : true);
                n->split_mode = (sm == '-' ? MODE_AUTOMATIC : MODE_MANUAL);
                if (sd == 'U')
                    n->split_dir = DIR_UP;
                else if (sd == 'R')
                    n->split_dir = DIR_RIGHT;
                else if (sd == 'D')
                    n->split_dir = DIR_DOWN;
                else if (sd == 'L')
                    n->split_dir = DIR_LEFT;
                n->client = c;
                if (end != 0)
                    d->focus = n;
                if (c->sticky)
                    m->num_sticky++;
            }
            if (br == 'a')
                n->birth_rotation = 90;
            else if (br == 'c')
                n->birth_rotation = 270;
            else if (br == 'm')
                n->birth_rotation = 0;
        }
        last_level = level;
    }

    fclose(snapshot);

    for (monitor_t *m = mon_head; m != NULL; m = m->next)
        for (desktop_t *d = m->desk_head; d != NULL; d = d->next)
            for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root)) {
                uint32_t values[] = {CLIENT_EVENT_MASK | (focus_follows_pointer ? XCB_EVENT_MASK_ENTER_WINDOW : 0)};
                xcb_change_window_attributes(dpy, n->client->window, XCB_CW_EVENT_MASK, values);
                if (n->client->floating) {
                    n->vacant = true;
                    update_vacant_state(n->parent);
                }
                if (n->client->private)
                    update_privacy_level(n, true);
            }
    ewmh_update_current_desktop();
}

void restore_history(char *file_path)
{
    if (file_path == NULL)
        return;

    FILE *snapshot = fopen(file_path, "r");
    if (snapshot == NULL) {
        warn("Restore history: can't open '%s'.\n", file_path);
        return;
    }

    PUTS("restore history");

    char line[MAXLEN];
    char mnm[SMALEN];
    char dnm[SMALEN];
    xcb_window_t win;

    while (fgets(line, sizeof(line), snapshot) != NULL) {
        if (sscanf(line, "%s %s %X", mnm, dnm, &win) == 3) {
            coordinates_t loc;
            if (win != XCB_NONE && !locate_window(win, &loc)) {
                warn("Can't locate window 0x%X.\n", win);
                continue;
            }
            node_t *n = (win == XCB_NONE ? NULL : loc.node);
            if (!locate_desktop(dnm, &loc)) {
                warn("Can't locate desktop '%s'.\n", dnm);
                continue;
            }
            desktop_t *d = loc.desktop;
            if (!locate_monitor(mnm, &loc)) {
                warn("Can't locate monitor '%s'.\n", mnm);
                continue;
            }
            monitor_t *m = loc.monitor;
            history_add(m, d, n);
        } else {
            warn("Can't parse history entry: '%s'\n", line);
        }
    }

    fclose(snapshot);
}

void restore_stack(char *file_path)
{
    if (file_path == NULL)
        return;

    FILE *snapshot = fopen(file_path, "r");
    if (snapshot == NULL) {
        warn("Restore stack: can't open '%s'.\n", file_path);
        return;
    }

    PUTS("restore stack");

    char line[MAXLEN];
    xcb_window_t win;

    while (fgets(line, sizeof(line), snapshot) != NULL) {
        if (sscanf(line, "%X", &win) == 1) {
            coordinates_t loc;
            if (locate_window(win, &loc))
                stack_insert_after(stack_tail, loc.node);
            else
                warn("Can't locate window 0x%X.\n", win);
        } else {
            warn("Can't parse stack entry: '%s'\n", line);
        }
    }

    fclose(snapshot);
}
