#include <ctype.h>
#include <string.h>
#include "types.h"
#include "tree.h"
#include "settings.h"
#include "ewmh.h"
#include "bspwm.h"
#include "query.h"
#include "restore.h"

void restore_tree(char *file_path)
{
    if (file_path == NULL)
        return;

    FILE *snapshot = fopen(file_path, "r");
    if (snapshot == NULL) {
        warn("Restore: can't open file\n");
        return;
    }

    PUTS("restore tree");

    char line[MAXLEN];
    monitor_t *m = NULL;
    desktop_t *d = NULL;
    node_t *n = NULL;
    num_clients = 0;
    unsigned int level, last_level = 0;
    bool aborted = false;

    while (fgets(line, sizeof(line), snapshot) != NULL) {
        unsigned int len = strlen(line);
        level = 0;
        while (level < strlen(line) && isspace(line[level]))
            level++;
        if (level == 0) {
            if (m == NULL)
                m = mon_head;
            else
                m = m->next;
            if (len >= 2)
                switch (line[len - 2]) {
                    case '#':
                        mon = m;
                        break;
                    case '~':
                        last_mon = m;
                        break;
                }
        } else if (level == 2) {
            if (d == NULL)
                d = m->desk_head;
            else
                d = d->next;
            int i = len - 1;
            while (i > 0 && !isupper(line[i]))
                i--;
            if (line[i] == 'M')
                d->layout = LAYOUT_MONOCLE;
            else if (line[i] == 'T')
                d->layout = LAYOUT_TILED;
            if (len >= 2)
                switch (line[len - 2]) {
                    case '@':
                        m->desk = d;
                        break;
                    case '~':
                        m->last_desk = d;
                        break;
                }
        } else {
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
                    if (n == NULL) {
                        warn("Restore: malformed file\n");
                        aborted = true;
                        break;
                    }
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
                char floating, transient, fullscreen, urgent, locked;
                sscanf(line + level, "%c %s %X %u %hux%hu%hi%hi %c%c%c%c%c", &br, c->class_name, &c->window, &c->border_width, &c->floating_rectangle.width, &c->floating_rectangle.height, &c->floating_rectangle.x, &c->floating_rectangle.y, &floating, &transient, &fullscreen, &urgent, &locked);
                c->floating = (floating == '-' ? false : true);
                c->transient = (transient == '-' ? false : true);
                c->fullscreen = (fullscreen == '-' ? false : true);
                c->urgent = (urgent == '-' ? false : true);
                c->locked = (locked == '-' ? false : true);
                n->client = c;
                if (len >= 2 && line[len - 2] == '*')
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

    if (!aborted) {
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
}

void restore_history(char *file_path)
{
    if (file_path == NULL)
        return;

    FILE *snapshot = fopen(file_path, "r");
    if (snapshot == NULL) {
        warn("Restore history: can't open file\n");
        return;
    }

    PUTS("restore history");

    char line[MAXLEN];
    desktop_t *d = NULL;
    unsigned int level;

    while (fgets(line, sizeof(line), snapshot) != NULL) {
        unsigned int i = strlen(line) - 1;
        while (i > 0 && isspace(line[i]))
            line[i--] = '\0';
        level = 0;
        while (level < strlen(line) && isspace(line[level]))
            level++;
        if (level == 0) {
            coordinates_t loc;
            if (locate_desktop(line + level, &loc))
                d = loc.desktop;
        } else if (d != NULL) {
            xcb_window_t win;
            if (sscanf(line + level, "%X", &win) == 1) {
                coordinates_t loc;
                if (locate_window(win, &loc))
                    history_add(d->history, loc.node);
            }
        }
    }

    fclose(snapshot);
}
