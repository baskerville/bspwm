#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "settings.h"
#include "helpers.h"
#include "window.h"
#include "types.h"
#include "bspwm.h"
#include "ewmh.h"
#include "tree.h"

bool is_leaf(node_t *n)
{
    return (n != NULL && n->first_child == NULL && n->second_child == NULL);
}

bool is_tiled(client_t *c)
{
    if (c == NULL)
        return false;
    return (!c->floating && !c->transient && !c->fullscreen);
}

bool is_floating(client_t *c)
{
    if (c == NULL)
        return false;
    return (c->floating && !c->fullscreen);
}

bool is_first_child(node_t *n)
{
    return (n != NULL && n->parent != NULL && n->parent->first_child == n);
}

bool is_second_child(node_t *n)
{
    return (n != NULL && n->parent != NULL && n->parent->second_child == n);
}

void change_split_ratio(node_t *n, value_change_t chg) {
    n->split_ratio = pow(n->split_ratio, (chg == CHANGE_INCREASE ? INC_EXP : DEC_EXP));
}

node_t *first_extrema(node_t *n)
{
    if (n == NULL)
        return NULL;
    else if (n->first_child == NULL)
        return n;
    else
        return first_extrema(n->first_child);
}

node_t *second_extrema(node_t *n)
{
    if (n == NULL)
        return NULL;
    else if (n->second_child == NULL)
        return n;
    else
        return second_extrema(n->second_child);
}

node_t *next_leaf(node_t *n)
{
    if (n == NULL)
        return NULL;
    node_t *p = n;
    while (is_second_child(p))
        p = p->parent;
    if (p->parent == NULL)
        return NULL;
    return first_extrema(p->parent->second_child);
}

node_t *prev_leaf(node_t *n)
{
    if (n == NULL)
        return NULL;
    node_t *p = n;
    while (is_first_child(p))
        p = p->parent;
    if (p->parent == NULL)
        return NULL;
    return second_extrema(p->parent->first_child);
}

node_t *find_fence(node_t *n, direction_t dir)
{
    node_t *p;

    if (n == NULL)
        return NULL;

    p = n->parent;

    while (p != NULL) {
        if ((dir == DIR_UP && p->split_type == TYPE_HORIZONTAL && p->rectangle.y < n->rectangle.y)
                || (dir == DIR_LEFT && p->split_type == TYPE_VERTICAL && p->rectangle.x < n->rectangle.x)
                || (dir == DIR_DOWN && p->split_type == TYPE_HORIZONTAL && (p->rectangle.y + p->rectangle.height) > (n->rectangle.y + n->rectangle.height))
                || (dir == DIR_RIGHT && p->split_type == TYPE_VERTICAL && (p->rectangle.x + p->rectangle.width) > (n->rectangle.x + n->rectangle.width)))
            return p;
        p = p->parent;
    }

    return NULL;
}

node_t *find_neighbor(node_t *n, direction_t dir)
{
    node_t *fence = find_fence(n, dir);

    if (fence == NULL)
        return NULL;

    if (dir == DIR_UP || dir == DIR_LEFT)
        return second_extrema(fence->first_child);
    else if (dir == DIR_DOWN || dir == DIR_RIGHT)
        return first_extrema(fence->second_child);

    return NULL;
}

void move_fence(node_t *n, direction_t dir, fence_move_t mov)
{
    node_t *fence = find_fence(n, dir);

    if (fence == NULL)
        return;

    if ((mov == MOVE_PUSH && (dir == DIR_RIGHT || dir == DIR_DOWN))
            || (mov == MOVE_PULL && (dir == DIR_LEFT || dir == DIR_UP)))
        change_split_ratio(fence, CHANGE_INCREASE);
    else
        change_split_ratio(fence, CHANGE_DECREASE);
}

void rotate_tree(node_t *n, rotate_t rot)
{
    if (n == NULL || is_leaf(n))
        return;

    node_t *tmp;

    if ((rot == ROTATE_CLOCKWISE && n->split_type == TYPE_HORIZONTAL)
            || (rot == ROTATE_COUNTER_CLOCKWISE && n->split_type == TYPE_VERTICAL)
            || rot == ROTATE_FULL_CYCLE) {
        tmp = n->first_child;
        n->first_child = n->second_child;
        n->second_child = tmp;
        n->split_ratio = 1.0 - n->split_ratio;
    }

    if (rot != ROTATE_FULL_CYCLE) {
        if (n->split_type == TYPE_HORIZONTAL)
            n->split_type = TYPE_VERTICAL;
        else if (n->split_type == TYPE_VERTICAL)
            n->split_type = TYPE_HORIZONTAL;
    }

    rotate_tree(n->first_child, rot);
    rotate_tree(n->second_child, rot);
}


void arrange(monitor_t *m, desktop_t *d)
{
    PRINTF("arrange %s%s%s\n", (num_monitors > 1 ? m->name : ""), (num_monitors > 1 ? " " : ""), d->name);

    xcb_rectangle_t rect = m->rectangle;
    int wg = (gapless_monocle && d->layout == LAYOUT_MONOCLE ? 0 : window_gap);
    rect.x += m->left_padding + wg;
    rect.y += m->top_padding + wg;
    rect.width -= m->left_padding + m->right_padding + wg;
    rect.height -= m->top_padding + m->bottom_padding + wg;
    if (focus_follows_mouse)
        get_pointer_position(&pointer_position);
    apply_layout(m, d, d->root, rect, rect);
}

void apply_layout(monitor_t *m, desktop_t *d, node_t *n, xcb_rectangle_t rect, xcb_rectangle_t root_rect)
{
    if (n == NULL)
        return;

    n->rectangle = rect;

    if (is_leaf(n)) {
        if (n->client->fullscreen)
            return;

        if (is_floating(n->client) && n->client->border_width != border_width) {
            int ds = 2 * (border_width - n->client->border_width);
            n->client->floating_rectangle.width += ds;
            n->client->floating_rectangle.height += ds;
        }

        if (borderless_monocle && is_tiled(n->client) && d->layout == LAYOUT_MONOCLE)
            n->client->border_width = 0;
        else
            n->client->border_width = border_width;

        xcb_rectangle_t r;
        if (is_tiled(n->client)) {
            if (d->layout == LAYOUT_TILED)
                r = rect;
            else if (d->layout == LAYOUT_MONOCLE)
                r = root_rect;
            int wg = (gapless_monocle && d->layout == LAYOUT_MONOCLE ? 0 : window_gap);
            int bleed = wg + 2 * n->client->border_width;
            r.width = (bleed < r.width ? r.width - bleed : 1);
            r.height = (bleed < r.height ? r.height - bleed : 1);
            n->client->tiled_rectangle = r;
        } else {
            r = n->client->floating_rectangle;
        }

        window_move_resize(n->client->window, r.x, r.y, r.width, r.height);
        window_border_width(n->client->window, n->client->border_width);
        window_draw_border(n, n == d->focus, m == mon);

    } else {
        xcb_rectangle_t first_rect;
        xcb_rectangle_t second_rect;

        if (n->first_child->vacant || n->second_child->vacant) {
            first_rect = second_rect = rect;
        } else {
            unsigned int fence;
            if (n->split_type == TYPE_VERTICAL) {
                fence = rect.width * n->split_ratio;
                first_rect = (xcb_rectangle_t) {rect.x, rect.y, fence, rect.height};
                second_rect = (xcb_rectangle_t) {rect.x + fence, rect.y, rect.width - fence, rect.height};

            } else if (n->split_type == TYPE_HORIZONTAL) {
                fence = rect.height * n->split_ratio;
                first_rect = (xcb_rectangle_t) {rect.x, rect.y, rect.width, fence};
                second_rect = (xcb_rectangle_t) {rect.x, rect.y + fence, rect.width, rect.height - fence};
            }
        }

        apply_layout(m, d, n->first_child, first_rect, root_rect);
        apply_layout(m, d, n->second_child, second_rect, root_rect);
    }
}

void insert_node(monitor_t *m, desktop_t *d, node_t *n)
{
    if (d == NULL || n == NULL)
        return;

    PRINTF("insert node %X\n", n->client->window);

    node_t *focus = d->focus;

    if (focus == NULL) {
        d->root = n;
    } else {
        node_t *dad = make_node();
        node_t *fopar = focus->parent;
        n->parent = dad;
        n->client->born_as = split_mode;
        switch (split_mode) {
            case MODE_AUTOMATIC:
                if (fopar == NULL) {
                    dad->first_child = n;
                    dad->second_child = focus;
                    if (m->rectangle.width > m->rectangle.height)
                        dad->split_type = TYPE_VERTICAL;
                    else
                        dad->split_type = TYPE_HORIZONTAL;
                    focus->parent = dad;
                    d->root = dad;
                } else {
                    node_t *grandpa = fopar->parent;
                    dad->parent = grandpa;
                    if (grandpa != NULL) {
                        if (is_first_child(fopar))
                            grandpa->first_child = dad;
                        else
                            grandpa->second_child = dad;
                    } else {
                        d->root = dad;
                    }
                    dad->split_type = fopar->split_type;
                    dad->split_ratio = fopar->split_ratio;
                    fopar->parent = dad;
                    if (is_first_child(focus)) {
                        dad->first_child = n;
                        dad->second_child = fopar;
                        rotate_tree(fopar, ROTATE_CLOCKWISE);
                    } else {
                        dad->first_child = fopar;
                        dad->second_child = n;
                        rotate_tree(fopar, ROTATE_COUNTER_CLOCKWISE);
                    }
                }
                break;
            case MODE_MANUAL:
                if (fopar != NULL) {
                    if (is_first_child(focus))
                        fopar->first_child = dad;
                    else
                        fopar->second_child = dad;
                }
                dad->split_ratio = focus->split_ratio;
                dad->parent = fopar;
                focus->parent = dad;
                switch (split_dir) {
                    case DIR_LEFT:
                        dad->split_type = TYPE_VERTICAL;
                        dad->first_child = n;
                        dad->second_child = focus;
                        break;
                    case DIR_RIGHT:
                        dad->split_type = TYPE_VERTICAL;
                        dad->first_child = focus;
                        dad->second_child = n;
                        break;
                    case DIR_UP:
                        dad->split_type = TYPE_HORIZONTAL;
                        dad->first_child = n;
                        dad->second_child = focus;
                        break;
                    case DIR_DOWN:
                        dad->split_type = TYPE_HORIZONTAL;
                        dad->first_child = focus;
                        dad->second_child = n;
                        break;
                }
                if (d->root == focus)
                    d->root = dad;
                split_mode = MODE_AUTOMATIC;
                break;
        }
        if (focus->vacant)
            update_vacant_state(fopar);
    }
}

void focus_node(monitor_t *m, desktop_t *d, node_t *n, bool is_mapped)
{
    if (n == NULL)
        return;

    PRINTF("focus node %X\n", n->client->window);

    split_mode = MODE_AUTOMATIC;
    n->client->urgent = false;

    if (is_mapped) {
        if (mon != m) {
            for (desktop_t *cd = mon->desk_head; cd != NULL; cd = cd->next)
                window_draw_border(cd->focus, true, false);
            for (desktop_t *cd = m->desk_head; cd != NULL; cd = cd->next)
                if (cd != d)
                    window_draw_border(cd->focus, true, true);
            if (d->focus == n)
                window_draw_border(n, true, true);
        }
        if (d->focus != n) {
            window_draw_border(d->focus, false, true);
            window_draw_border(n, true, true);
        }
        if (focus_follows_mouse)
            get_pointer_position(&pointer_position);
        xcb_set_input_focus(dpy, XCB_INPUT_FOCUS_POINTER_ROOT, n->client->window, XCB_CURRENT_TIME);
    }

    if (!is_tiled(n->client)) {
        if (!adaptative_raise || !might_cover(d, n))
            window_raise(n->client->window);
    } else {
        window_pseudo_raise(d, n->client->window);
    }

    if (d->focus != n) {
        d->last_focus = d->focus;
        d->focus = n;
    }

    ewmh_update_active_window();
    put_status();
}

void update_current(void)
{
    if (mon->desk->focus == NULL)
        ewmh_update_active_window();
    else
        focus_node(mon, mon->desk, mon->desk->focus, true);
    put_status();
}

void unlink_node(desktop_t *d, node_t *n)
{
    if (d == NULL || n == NULL)
        return;

    PRINTF("unlink node %X\n", n->client->window);

    node_t *p = n->parent;

    if (p == NULL) {
        d->root = NULL;
        d->focus = NULL;
        d->last_focus = NULL;
    } else {
        node_t *b;
        node_t *g = p->parent;
        bool n_first_child = is_first_child(n);
        if (n_first_child) {
            b = p->second_child;
            if (n->client->born_as == MODE_AUTOMATIC)
                rotate_tree(b, ROTATE_COUNTER_CLOCKWISE);
        } else {
            b = p->first_child;
            if (n->client->born_as == MODE_AUTOMATIC)
                rotate_tree(b, ROTATE_CLOCKWISE);
        }
        b->parent = g;
        if (g != NULL) {
            if (is_first_child(p))
                g->first_child = b;
            else
                g->second_child = b;
        } else {
            d->root = b;
        }

        n->parent = NULL;
        free(p);

        if (n == d->last_focus) {
            d->last_focus = NULL;
        } else if (n == d->focus) {
            if (d->last_focus != NULL)
                d->focus = d->last_focus;
            else
                d->focus = (n_first_child ? first_extrema(b) : second_extrema(b));
            d->last_focus = NULL;
        }

        update_vacant_state(b->parent);
    }
}

void remove_node(desktop_t *d, node_t *n)
{
    if (d == NULL || n == NULL)
        return;

    PRINTF("remove node %X\n", n->client->window);

    unlink_node(d, n);
    free(n->client);
    free(n);

    num_clients--;
    ewmh_update_client_list();

    if (mon->desk == d)
        update_current();
}

void destroy_tree(node_t *n)
{
    if (n == NULL)
        return;
    node_t *first_tree = n->first_child;
    node_t *second_tree = n->second_child;
    if (n->client != NULL)
        free(n->client);
    free(n);
    destroy_tree(first_tree);
    destroy_tree(second_tree);
}

void swap_nodes(node_t *n1, node_t *n2)
{
    if (n1 == NULL || n2 == NULL || n1 == n2)
        return;

    PUTS("swap nodes");

    /* (n1 and n2 are leaves) */
    node_t *pn1 = n1->parent;
    node_t *pn2 = n2->parent;
    bool n1_first_child = is_first_child(n1);
    bool n2_first_child = is_first_child(n2);

    if (pn1 != NULL) {
        if (n1_first_child)
            pn1->first_child = n2;
        else
            pn1->second_child = n2;
    }

    if (pn2 != NULL) {
        if (n2_first_child)
            pn2->first_child = n1;
        else
            pn2->second_child = n1;
    }

    n1->parent = pn2;
    n2->parent = pn1;

    if (n1->vacant != n2->vacant) {
        update_vacant_state(n1->parent);
        update_vacant_state(n2->parent);
    }
}

void transfer_node(monitor_t *ms, desktop_t *ds, monitor_t *md, desktop_t *dd, node_t *n)
{
    if (n == NULL || ds == NULL || dd == NULL || ms == NULL || md == NULL || (ms == md && dd == ds))
        return;

    PRINTF("transfer node %X\n", n->client->window);

    unlink_node(ds, n);
    insert_node(md, dd, n);
    ewmh_set_wm_desktop(n, dd);

    if (ds == ms->desk && dd != md->desk) {
        window_hide(n->client->window);
    }

    fit_monitor(md, n->client);

    if (n->client->fullscreen)
        window_move_resize(n->client->window, md->rectangle.x, md->rectangle.y, md->rectangle.width, md->rectangle.height);

    if (ds != ms->desk && dd == md->desk) {
        window_show(n->client->window);
        focus_node(md, dd, n, true);
    } else {
        focus_node(md, dd, n, false);
    }

    if (ds == ms->desk || dd == md->desk)
        update_current();
}

void select_monitor(monitor_t *m)
{
    if (m == NULL || mon == m)
        return;

    PRINTF("select monitor %s\n", m->name);

    focus_node(m, m->desk, m->desk->focus, true);

    last_mon = mon;
    mon = m;

    ewmh_update_current_desktop();
    put_status();
}

void select_desktop(desktop_t *d)
{
    if (d == NULL || d == mon->desk)
        return;

    PRINTF("select desktop %s\n", d->name);

    if (visible) {
        node_t *n = first_extrema(d->root);

        while (n != NULL) {
            window_show(n->client->window);
            n = next_leaf(n);
        }

        n = first_extrema(mon->desk->root);

        while (n != NULL) {
            window_hide(n->client->window);
            n = next_leaf(n);
        }
    }

    mon->last_desk = mon->desk;
    mon->desk = d;

    update_current();
    ewmh_update_current_desktop();
    put_status();
}

void cycle_monitor(cycle_dir_t dir)
{
    if (dir == CYCLE_NEXT)
        select_monitor((mon->next == NULL ? mon_head : mon->next));
    else if (dir == CYCLE_PREV)
        select_monitor((mon->prev == NULL ? mon_tail : mon->prev));
}

void cycle_desktop(monitor_t *m, desktop_t *d, cycle_dir_t dir, skip_desktop_t skip)
{
    desktop_t *f = (dir == CYCLE_PREV ? d->prev : d->next);
    if (f == NULL)
        f = (dir == CYCLE_PREV ? m->desk_tail : m->desk_head);

    while (f != d) {
        if (skip == DESKTOP_SKIP_NONE 
                || (skip == DESKTOP_SKIP_FREE && f->root != NULL)
                || (skip == DESKTOP_SKIP_OCCUPIED && f->root == NULL)) {
            select_desktop(f);
            return;
        }
        f = (dir == CYCLE_PREV ? f->prev : f->next);
        if (f == NULL)
            f = (dir == CYCLE_PREV ? m->desk_tail : m->desk_head);
    }
}

void cycle_leaf(monitor_t *m, desktop_t *d, node_t *n, cycle_dir_t dir, skip_client_t skip)
{
    if (n == NULL)
        return;

    PUTS("cycle leaf");

    node_t *f = (dir == CYCLE_PREV ? prev_leaf(n) : next_leaf(n));
    if (f == NULL)
        f = (dir == CYCLE_PREV ? second_extrema(d->root) : first_extrema(d->root));

    while (f != n) {
        bool tiled = is_tiled(f->client);
        if (skip == CLIENT_SKIP_NONE || (skip == CLIENT_SKIP_TILED && !tiled) || (skip == CLIENT_SKIP_FLOATING && tiled)
                || (skip == CLIENT_SKIP_CLASS_DIFFER && strcmp(f->client->class_name, n->client->class_name) == 0)
                || (skip == CLIENT_SKIP_CLASS_EQUAL && strcmp(f->client->class_name, n->client->class_name) != 0)) {
            focus_node(m, d, f, true);
            return;
        }
        f = (dir == CYCLE_PREV ? prev_leaf(f) : next_leaf(f));
        if (f == NULL)
            f = (dir == CYCLE_PREV ? second_extrema(d->root) : first_extrema(d->root));
    }
}

void nearest_leaf(monitor_t *m, desktop_t *d, node_t *n, nearest_arg_t dir, skip_client_t skip)
{
    if (n == NULL)
        return;

    PUTS("nearest leaf");

    node_t *x = NULL;

    for (node_t *f = first_extrema(d->root); f != NULL; f = next_leaf(f))
        if (skip == CLIENT_SKIP_NONE || (skip == CLIENT_SKIP_TILED && !is_tiled(f->client)) || (skip == CLIENT_SKIP_FLOATING && is_tiled(f->client))
                || (skip == CLIENT_SKIP_CLASS_DIFFER && strcmp(f->client->class_name, n->client->class_name) == 0)
                || (skip == CLIENT_SKIP_CLASS_EQUAL && strcmp(f->client->class_name, n->client->class_name) != 0))
            if ((dir == NEAREST_OLDER
                        && (f->client->uid < n->client->uid)
                        && (x == NULL || f->client->uid > x->client->uid))
                    || (dir == NEAREST_NEWER
                        && (f->client->uid > n->client->uid)
                        && (x == NULL || f->client->uid < x->client->uid)))
                x = f;

    focus_node(m, d, x, true);
}

void circulate_leaves(monitor_t *m, desktop_t *d, circulate_dir_t dir) {
    if (d == NULL || d->root == NULL || is_leaf(d->root))
        return;
    node_t *par = d->focus->parent;
    bool focus_first_child = is_first_child(d->focus);
    if (dir == CIRCULATE_FORWARD)
        for (node_t *s = second_extrema(d->root), *f = prev_leaf(s); f != NULL; s = prev_leaf(f), f = prev_leaf(s))
            swap_nodes(f, s);
    else
        for (node_t *f = first_extrema(d->root), *s = next_leaf(f); s != NULL; f = next_leaf(s), s = next_leaf(f))
            swap_nodes(f, s);
    if (focus_first_child)
        focus_node(m, d, par->first_child, true);
    else
        focus_node(m, d, par->second_child, true);
}

void update_vacant_state(node_t *n)
{
    if (n == NULL)
        return;

    PUTS("update vacant state");

    /* n is not a leaf */
    node_t *p = n;

    while (p != NULL) {
        p->vacant = (p->first_child->vacant && p->second_child->vacant);
        p = p->parent;
    }
}

void fit_monitor(monitor_t *m, client_t *c)
{
    xcb_rectangle_t crect = c->floating_rectangle;
    xcb_rectangle_t mrect = m->rectangle;
    while (crect.x < mrect.x)
        crect.x += mrect.width;
    while (crect.x > (mrect.x + mrect.width - 1))
        crect.x -= mrect.width;
    while (crect.y < mrect.y)
        crect.y += mrect.height;
    while (crect.y > (mrect.y + mrect.height - 1))
        crect.y -= mrect.height;
    c->floating_rectangle = crect;
}

void put_status(void)
{
    if (status_fifo == NULL)
        return;
    bool urgent = false;
    for (monitor_t *m = mon_head; m != NULL; m = m->next) {
        fprintf(status_fifo, "%c%s:", (mon == m ? 'M' : 'm'), m->name);
        for (desktop_t *d = m->desk_head; d != NULL; d = d->next, urgent = false) {
            for (node_t *n = first_extrema(d->root); n != NULL && !urgent; n = next_leaf(n))
                urgent |= n->client->urgent;
            fprintf(status_fifo, "%c%c%s:", (m->desk == d ? 'D' : (d->root != NULL ? 'd' : '_')), (urgent ? '!' : '_'), d->name);
        }
    }
    fprintf(status_fifo, "L%s:W%X\n", (mon->desk->layout == LAYOUT_TILED ? "tiled" : "monocle"), (mon->desk->focus == NULL ? 0 : mon->desk->focus->client->window));
    fflush(status_fifo);
}

void list_monitors(list_option_t opt, char *rsp)
{
    char line[MAXLEN];
    for (monitor_t *m = mon_head; m != NULL; m = m->next) {
        snprintf(line, sizeof(line), "%s %ux%u%+i%+i", m->name, m->rectangle.width, m->rectangle.height, m->rectangle.x, m->rectangle.y);
        strncat(rsp, line, REMLEN(rsp));
        if (m == mon)
            strncat(rsp, " #\n", REMLEN(rsp));
        else if (m == last_mon)
            strncat(rsp, " ~\n", REMLEN(rsp));
        else
            strncat(rsp, "\n", REMLEN(rsp));
        if (opt == LIST_OPTION_VERBOSE)
            list_desktops(m, opt, 1, rsp);
    }
}

void list_desktops(monitor_t *m, list_option_t opt, unsigned int depth, char *rsp)
{
    char line[MAXLEN];
    for (desktop_t *d = m->desk_head; d != NULL; d = d->next) {
        for (unsigned int i = 0; i < depth; i++)
            strncat(rsp, "  ", REMLEN(rsp));
        snprintf(line, sizeof(line), "%s %c", d->name, (d->layout == LAYOUT_TILED ? 'T' : 'M'));
        strncat(rsp, line, REMLEN(rsp));
        if (d == m->desk)
            strncat(rsp, " @\n", REMLEN(rsp));
        else if (d == m->last_desk)
            strncat(rsp, " ~\n", REMLEN(rsp));
        else
            strncat(rsp, "\n", REMLEN(rsp));
        if (opt == LIST_OPTION_VERBOSE)
            list(d, d->root, rsp, depth + 1);
    }
}

void list(desktop_t *d, node_t *n, char *rsp, unsigned int depth)
{
    if (n == NULL)
        return;

    char line[MAXLEN];

    for (unsigned int i = 0; i < depth; i++)
        strncat(rsp, "  ", REMLEN(rsp));

    if (is_leaf(n)) {
        client_t *c = n->client;
        snprintf(line, sizeof(line), "%c %s %X %u %u %ux%u%+i%+i %c%c%c%c%c", (c->born_as == MODE_AUTOMATIC ? 'a' : 'm'), c->class_name, c->window, c->uid, c->border_width, c->floating_rectangle.width, c->floating_rectangle.height, c->floating_rectangle.x, c->floating_rectangle.y, (c->floating ? 'f' : '-'), (c->transient ? 't' : '-'), (c->fullscreen ? 'F' : '-'), (c->urgent ? 'u' : '-'), (c->locked ? 'l' : '-'));
    } else {
        snprintf(line, sizeof(line), "%c %.2f", (n->split_type == TYPE_HORIZONTAL ? 'H' : 'V'), n->split_ratio);
    }

    strncat(rsp, line, REMLEN(rsp));

    if (n == d->focus)
        strncat(rsp, " *\n", REMLEN(rsp));
    else if (n == d->last_focus)
        strncat(rsp, " ~\n", REMLEN(rsp));
    else
        strncat(rsp, "\n", REMLEN(rsp));

    list(d, n->first_child, rsp, depth + 1);
    list(d, n->second_child, rsp, depth + 1);
}

void restore(char *file_path)
{
    if (file_path == NULL)
        return;

    FILE *snapshot = fopen(file_path, "r");
    if (snapshot == NULL) {
        warn("restore: can't open file\n");
        return;
    }

    char line[MAXLEN];
    monitor_t *m = NULL;
    desktop_t *d = NULL;
    node_t *n = NULL;
    num_clients = 0;
    unsigned int level, last_level = 0, max_uid = 0;
    bool aborted = false;

    while (!aborted && fgets(line, sizeof(line), snapshot) != NULL) {
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
            } else {
                if (level > last_level) {
                    n->first_child = birth;
                } else {
                    do {
                        n = n->parent;
                    } while (n != NULL && n->second_child != NULL);
                    if (n == NULL) {
                        warn("restore: file is malformed\n");
                        aborted = true;
                    }
                    n->second_child = birth;
                }
                birth->parent = n;
            }
            n = birth;

            if (isupper(line[level])) {
                char st;
                sscanf(line + level, "%c %lf", &st, &n->split_ratio);
                if (st == 'H')
                    n->split_type = TYPE_HORIZONTAL;
                else if (st == 'V')
                    n->split_type = TYPE_VERTICAL;
            } else {
                client_t *c = make_client(XCB_NONE);
                num_clients++;
                char ba, floating, transient, fullscreen, urgent, locked;
                sscanf(line + level, "%c %s %X %u %u %hux%hu%hi%hi %c%c%c%c%c", &ba, c->class_name, &c->window, &c->uid, &c->border_width, &c->floating_rectangle.width, &c->floating_rectangle.height, &c->floating_rectangle.x, &c->floating_rectangle.y, &floating, &transient, &fullscreen, &urgent, &locked);
                if (ba == 'a')
                    c->born_as = MODE_AUTOMATIC;
                else if (ba == 'm')
                    c->born_as = MODE_MANUAL;
                c->floating = (floating == '-' ? false : true);
                c->transient = (transient == '-' ? false : true);
                c->fullscreen = (fullscreen == '-' ? false : true);
                c->urgent = (urgent == '-' ? false : true);
                c->locked = (locked == '-' ? false : true);
                if (c->uid > max_uid)
                    max_uid = c->uid;
                n->client = c;
                if (len >= 2)
                    switch (line[len - 2]) {
                        case '*':
                            d->focus = n;
                            break;
                        case '~':
                            d->last_focus = n;
                            break;
                    }
            }
        }
        last_level = level;
    }

    if (!aborted) {
        client_uid = max_uid + 1;
        for (monitor_t *m = mon_head; m != NULL; m = m->next)
            for (desktop_t *d = m->desk_head; d != NULL; d = d->next)
                for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n))
                    if (n->client->floating) {
                        n->vacant = true;
                        update_vacant_state(n->parent);
                    }
    }

    fclose(snapshot);
}
