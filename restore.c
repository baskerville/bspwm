#include <ctype.h>
#include <string.h>
#include "bspwm.h"
#include "desktop.h"
#include "ewmh.h"
#include "history.h"
#include "monitor.h"
#include "query.h"
#include "settings.h"
#include "stack.h"
#include "tree.h"
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
    num_clients = 0;
    unsigned int level, last_level = 0;

    while (fgets(line, sizeof(line), snapshot) != NULL) {
        unsigned int len = strlen(line);
        level = 0;

        while (level < len && isspace(line[level]))
            level++;

        if (level == 0) {
            int x, y, left, right, top, bottom;
            unsigned int w, h;
            char end = 0;
            name[0] = '\0';
            sscanf(line + level, "%s %ux%u%i%i %i,%i,%i,%i %c", name, &w, &h, &x, &y, &top, &right, &bottom, &left, &end);
            m = find_monitor(name);
            if (m == NULL)
                continue;
            m->rectangle = (xcb_rectangle_t) {x, y, w, h};
            m->top_padding = top;
            m->right_padding = right;
            m->bottom_padding = bottom;
            m->left_padding = left;
            if (end != 0)
                mon = m;
        } else if (level == 2) {
            if (m == NULL)
                continue;
            int wg;
            char layout = 0, end = 0;
            name[0] = '\0';
            loc.desktop = NULL;
            sscanf(line + level, "%s %i %c %c", name, &wg, &layout, &end);
            locate_desktop(name, &loc);
            d = loc.desktop;
            if (d == NULL)
                continue;
            d->window_gap = wg;
            if (layout == 'M')
                d->layout = LAYOUT_MONOCLE;
            else if (layout == 'T')
                d->layout = LAYOUT_TILED;
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
                char floating, transient, fullscreen, urgent, locked, sd, sm, end = 0;
                sscanf(line + level, "%c %s %X %u %hux%hu%hi%hi %c %c%c%c%c%c%c %c", &br, c->class_name, &c->window, &c->border_width, &c->floating_rectangle.width, &c->floating_rectangle.height, &c->floating_rectangle.x, &c->floating_rectangle.y, &sd, &floating, &transient, &fullscreen, &urgent, &locked, &sm, &end);
                c->floating = (floating == '-' ? false : true);
                c->transient = (transient == '-' ? false : true);
                c->fullscreen = (fullscreen == '-' ? false : true);
                c->urgent = (urgent == '-' ? false : true);
                c->locked = (locked == '-' ? false : true);
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
                uint32_t values[] = {(focus_follows_pointer ? CLIENT_EVENT_MASK_FFP : CLIENT_EVENT_MASK)};
                xcb_change_window_attributes(dpy, n->client->window, XCB_CW_EVENT_MASK, values);
                if (n->client->floating) {
                    n->vacant = true;
                    update_vacant_state(n->parent);
                }
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
            if (win != XCB_NONE && !locate_window(win, &loc)) {
                warn("Can't locate window 0x%X.\n", win);
                continue;
            }
            stack_insert_after(stack_tail, loc.node);
        } else {
            warn("Can't parse stack entry: '%s'\n", line);
        }
    }

    fclose(snapshot);
}
