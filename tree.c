#include <string.h>
#include <math.h>
#include <limits.h>
#include <float.h>
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
    return (!c->floating && !c->fullscreen);
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

/**
 * Check if the specified node matches the selection criteria.
 *
 * Arguments:
 *  node_t *c           - the active node
 *  node_t *t           - the node to test
 *  client_sel_t sel    - the selection criteria
 *
 * Returns true if the node matches.
 **/
bool node_matches(node_t *c, node_t *t, client_select_t sel)
{
    if (sel.type != CLIENT_TYPE_ALL &&
            is_tiled(t->client)
            ? sel.type == CLIENT_TYPE_FLOATING
            : sel.type == CLIENT_TYPE_TILED
       ) return false;

    if (sel.class != CLIENT_CLASS_ALL &&
            streq(c->client->class_name, t->client->class_name)
            ? sel.class == CLIENT_CLASS_DIFFER
            : sel.class == CLIENT_CLASS_EQUAL
       ) return false;

    return true;
}

bool desktop_matches(desktop_t *t, desktop_select_t sel) {
    if (sel != DESKTOP_ALL &&
            t->root == NULL
            ? sel == DESKTOP_OCCUPIED
            : sel == DESKTOP_FREE
       ) return false;

    return true;
}

void change_split_ratio(node_t *n, value_change_t chg)
{
    n->split_ratio = pow(n->split_ratio,
            (chg == CHANGE_INCREASE ? (1 / GROWTH_FACTOR) : GROWTH_FACTOR));
}

void change_layout(monitor_t *m, desktop_t *d, layout_t l)
{
    d->layout = l;
    arrange(m, d);
    if (d == mon->desk)
        put_status();
}

void reset_mode(coordinates_t *loc)
{
    if (loc->node != NULL) {
        loc->node->split_mode = MODE_AUTOMATIC;
        window_draw_border(loc->node, loc->desktop->focus == loc->node, mon == loc->monitor);
    } else if (loc->desktop != NULL) {
        for (node_t *a = first_extrema(loc->desktop->root); a != NULL; a = next_leaf(a, loc->desktop->root)) {
            a->split_mode = MODE_AUTOMATIC;
            window_draw_border(a, loc->desktop->focus == a, mon == loc->monitor);
        }
    }
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

node_t *next_leaf(node_t *n, node_t *r)
{
    if (n == NULL)
        return NULL;
    node_t *p = n;
    while (is_second_child(p) && p != r)
        p = p->parent;
    if (p == r)
        return NULL;
    return first_extrema(p->parent->second_child);
}

node_t *prev_leaf(node_t *n, node_t *r)
{
    if (n == NULL)
        return NULL;
    node_t *p = n;
    while (is_first_child(p) && p != r)
        p = p->parent;
    if (p == r)
        return NULL;
    return second_extrema(p->parent->first_child);
}

/* bool is_adjacent(node_t *a, node_t *r) */
/* { */
/*     node_t *f = r->parent; */
/*     node_t *p = a; */
/*     bool first_child = is_first_child(r); */
/*     while (p != r) { */
/*         if (p->parent->split_type == f->split_type && is_first_child(p) == first_child) */
/*             return false; */
/*         p = p->parent; */
/*     } */
/*     return true; */
/* } */

/* Returns true if *b* is adjacent to *a* in the direction *dir* */
bool is_adjacent(node_t *a, node_t *b, direction_t dir)
{
    switch (dir) {
        case DIR_RIGHT:
            return (a->rectangle.x + a->rectangle.width) == b->rectangle.x;
            break;
        case DIR_DOWN:
            return (a->rectangle.y + a->rectangle.height) == b->rectangle.y;
            break;
        case DIR_LEFT:
            return (b->rectangle.x + b->rectangle.width) == a->rectangle.x;
            break;
        case DIR_UP:
            return (b->rectangle.y + b->rectangle.height) == a->rectangle.y;
            break;
    }
    return false;
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


node_t *nearest_neighbor(desktop_t *d, node_t *n, direction_t dir, client_select_t sel)
{
    if (n == NULL || n->client->fullscreen
            || (d->layout == LAYOUT_MONOCLE && is_tiled(n->client)))
        return NULL;

    node_t *nearest = NULL;
    if (history_aware_focus)
        nearest = nearest_from_history(d->history, n, dir, sel);
    if (nearest == NULL)
        nearest = nearest_from_distance(d, n, dir, sel);
    return nearest;
}

node_t *nearest_from_history(focus_history_t *f, node_t *n, direction_t dir, client_select_t sel)
{
    if (n == NULL || !is_tiled(n->client))
        return NULL;

    node_t *target = find_fence(n, dir);
    if (target == NULL)
        return NULL;
    if (dir == DIR_UP || dir == DIR_LEFT)
        target = target->first_child;
    else if (dir == DIR_DOWN || dir == DIR_RIGHT)
        target = target->second_child;

    node_t *nearest = NULL;
    int min_rank = INT_MAX;

    for (node_t *a = first_extrema(target); a != NULL; a = next_leaf(a, target)) {
        if (a->vacant || !is_adjacent(n, a, dir) || a == n)
            continue;
        if (!node_matches(n, a, sel))
            continue;

        int rank = history_rank(f, a);
        if (rank >= 0 && rank < min_rank) {
            nearest = a;
            min_rank = rank;
        }
    }

    return nearest;
}

node_t *nearest_from_distance(desktop_t *d, node_t *n, direction_t dir, client_select_t sel)
{
    if (n == NULL)
        return NULL;

    node_t *target = NULL;

    if (is_tiled(n->client)) {
        target = find_fence(n, dir);
        if (target == NULL)
            return NULL;
        if (dir == DIR_UP || dir == DIR_LEFT)
            target = target->first_child;
        else if (dir == DIR_DOWN || dir == DIR_RIGHT)
            target = target->second_child;
    } else {
        target = d->root;
    }

    node_t *nearest = NULL;
    direction_t dir2;
    xcb_point_t pt;
    xcb_point_t pt2;
    get_side_handle(n->client, dir, &pt);
    get_opposite(dir, &dir2);
    double ds = DBL_MAX;

    for (node_t *a = first_extrema(target); a != NULL; a = next_leaf(a, target)) {
        if (a == n) continue;
        if (!node_matches(n, a, sel)) continue;
        if (is_tiled(a->client) != is_tiled(n->client)) continue;
        if (is_tiled(a->client) && !is_adjacent(n, a, dir)) continue;

        get_side_handle(a->client, dir2, &pt2);
        double ds2 = distance(pt, pt2);
        if (ds2 < ds) {
            ds = ds2;
            nearest = a;
        }
    }

    return nearest;
}

void get_opposite(direction_t src, direction_t *dst)
{
    switch (src) {
        case DIR_RIGHT:
            *dst = DIR_LEFT;
            break;
        case DIR_DOWN:
            *dst = DIR_UP;
            break;
        case DIR_LEFT:
            *dst = DIR_RIGHT;
            break;
        case DIR_UP:
            *dst = DIR_DOWN;
            break;
    }
}

int tiled_area(node_t *n)
{
    if (n == NULL)
        return -1;
    xcb_rectangle_t rect = n->client->tiled_rectangle;
    return rect.width * rect.height;
}

node_t *find_biggest(desktop_t *d, node_t *c, client_select_t sel)
{
    if (d == NULL)
        return NULL;

    node_t *r = NULL;
    int r_area = tiled_area(r);

    for (node_t *f = first_extrema(d->root); f != NULL; f = next_leaf(f, d->root)) {
        if (!is_tiled(f->client) || !node_matches(c, f, sel))
            continue;
        int f_area = tiled_area(f);
        if (r == NULL) {
            r = f;
            r_area = f_area;
        } else if (f_area > r_area) {
            r = f;
            r_area = f_area;
        }
    }

    return r;
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

void rotate_tree(node_t *n, int rot)
{
    if (n == NULL || is_leaf(n) || rot == 0)
        return;

    node_t *tmp;

    if ((rot == 90 && n->split_type == TYPE_HORIZONTAL)
            || (rot == 270 && n->split_type == TYPE_VERTICAL)
            || rot == 180) {
        tmp = n->first_child;
        n->first_child = n->second_child;
        n->second_child = tmp;
        n->split_ratio = 1.0 - n->split_ratio;
    }

    if (rot != 180) {
        if (n->split_type == TYPE_HORIZONTAL)
            n->split_type = TYPE_VERTICAL;
        else if (n->split_type == TYPE_VERTICAL)
            n->split_type = TYPE_HORIZONTAL;
    }

    rotate_tree(n->first_child, rot);
    rotate_tree(n->second_child, rot);
}

void rotate_brother(node_t *n)
{
    if (n == NULL || n->parent == NULL)
        return;
    if (is_first_child(n))
        rotate_tree(n->parent->second_child, n->birth_rotation);
    else
        rotate_tree(n->parent->first_child, n->birth_rotation);
}

void unrotate_tree(node_t *n, int rot)
{
    if (rot == 0)
        return;
    rotate_tree(n, 360 - rot);
}

void unrotate_brother(node_t *n)
{
    if (n == NULL || n->parent == NULL)
        return;
    if (is_first_child(n))
        unrotate_tree(n->parent->second_child, n->birth_rotation);
    else
        unrotate_tree(n->parent->first_child, n->birth_rotation);
}

void flip_tree(node_t *n, flip_t flp)
{
    if (n == NULL || is_leaf(n))
        return;

    node_t *tmp;

    if ((flp == FLIP_HORIZONTAL && n->split_type == TYPE_HORIZONTAL)
            || (flp == FLIP_VERTICAL && n->split_type == TYPE_VERTICAL)) {
        tmp = n->first_child;
        n->first_child = n->second_child;
        n->second_child = tmp;
        n->split_ratio = 1.0 - n->split_ratio;
    }

    flip_tree(n->first_child, flp);
    flip_tree(n->second_child, flp);
}

int balance_tree(node_t *n)
{
    if (n == NULL || n->vacant) {
        return 0;
    } else if (is_leaf(n)) {
        return 1;
    } else {
        int b1 = balance_tree(n->first_child);
        int b2 = balance_tree(n->second_child);
        int b = b1 + b2;
        if (b1 > 0 && b2 > 0)
            n->split_ratio = (double) b1 / b;
        return b;
    }
}

void arrange(monitor_t *m, desktop_t *d)
{
    if (d->root == NULL)
        return;

    PRINTF("arrange %s%s%s\n", (num_monitors > 1 ? m->name : ""), (num_monitors > 1 ? " " : ""), d->name);

    xcb_rectangle_t rect = m->rectangle;
    int wg = (gapless_monocle && d->layout == LAYOUT_MONOCLE ? 0 : window_gap);
    rect.x += m->left_padding + wg;
    rect.y += m->top_padding + wg;
    rect.width -= m->left_padding + m->right_padding + wg;
    rect.height -= m->top_padding + m->bottom_padding + wg;
    apply_layout(m, d, d->root, rect, rect);
}

void apply_layout(monitor_t *m, desktop_t *d, node_t *n, xcb_rectangle_t rect, xcb_rectangle_t root_rect)
{
    if (n == NULL)
        return;

    n->rectangle = rect;

    if (is_leaf(n)) {

        if (is_floating(n->client) && n->client->border_width != border_width) {
            int ds = 2 * (border_width - n->client->border_width);
            n->client->floating_rectangle.width += ds;
            n->client->floating_rectangle.height += ds;
        }

        if ((borderless_monocle && is_tiled(n->client) && d->layout == LAYOUT_MONOCLE) ||
                n->client->fullscreen)
            n->client->border_width = 0;
        else
            n->client->border_width = border_width;

        xcb_rectangle_t r;
        if (!n->client->fullscreen) {
            if (!n->client->floating) {
                /* tiled clients */
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
                /* floating clients */
                r = n->client->floating_rectangle;
            }
        } else {
            /* fullscreen clients */
            r = m->rectangle;
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

void insert_node(monitor_t *m, desktop_t *d, node_t *n, node_t *f)
{
    if (d == NULL || n == NULL)
        return;

    PRINTF("insert node %X\n", n->client->window);

    /* n: new leaf node */
    /* c: new container node */
    /* f: focus or insertion anchor */
    /* p: parent of focus */
    /* g: grand parent of focus */

    if (f == NULL) {
        d->root = n;
    } else {
        node_t *c = make_node();
        node_t *p = f->parent;
        n->parent = c;
        c->birth_rotation = f->birth_rotation;
        switch (f->split_mode) {
            case MODE_AUTOMATIC:
                if (p == NULL) {
                    c->first_child = n;
                    c->second_child = f;
                    if (m->rectangle.width > m->rectangle.height)
                        c->split_type = TYPE_VERTICAL;
                    else
                        c->split_type = TYPE_HORIZONTAL;
                    f->parent = c;
                    d->root = c;
                } else {
                    node_t *g = p->parent;
                    c->parent = g;
                    if (g != NULL) {
                        if (is_first_child(p))
                            g->first_child = c;
                        else
                            g->second_child = c;
                    } else {
                        d->root = c;
                    }
                    c->split_type = p->split_type;
                    c->split_ratio = p->split_ratio;
                    p->parent = c;
                    int rot;
                    if (is_first_child(f)) {
                        c->first_child = n;
                        c->second_child = p;
                        rot = 90;
                    } else {
                        c->first_child = p;
                        c->second_child = n;
                        rot = 270;
                    }
                    if (!is_floating(n->client))
                        rotate_tree(p, rot);
                    n->birth_rotation = rot;
                }
                break;
            case MODE_MANUAL:
                if (p != NULL) {
                    if (is_first_child(f))
                        p->first_child = c;
                    else
                        p->second_child = c;
                }
                c->split_ratio = f->split_ratio;
                c->parent = p;
                f->parent = c;
                f->birth_rotation = 0;
                switch (f->split_dir) {
                    case DIR_LEFT:
                        c->split_type = TYPE_VERTICAL;
                        c->first_child = n;
                        c->second_child = f;
                        break;
                    case DIR_RIGHT:
                        c->split_type = TYPE_VERTICAL;
                        c->first_child = f;
                        c->second_child = n;
                        break;
                    case DIR_UP:
                        c->split_type = TYPE_HORIZONTAL;
                        c->first_child = n;
                        c->second_child = f;
                        break;
                    case DIR_DOWN:
                        c->split_type = TYPE_HORIZONTAL;
                        c->first_child = f;
                        c->second_child = n;
                        break;
                }
                if (d->root == f)
                    d->root = c;
                f->split_mode = MODE_AUTOMATIC;
                break;
        }
        if (f->vacant)
            update_vacant_state(p);
    }
    put_status();
}

void pseudo_focus(desktop_t *d, node_t *n)
{
    if (d->focus == n)
        return;
    d->focus = n;
    history_add(d->history, n);
}

void focus_node(monitor_t *m, desktop_t *d, node_t *n)
{
    if (n == NULL && d->root != NULL)
        return;

    if (mon->desk != d)
        clear_input_focus();

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

    select_desktop(m, d);

    if (n == NULL) {
        ewmh_update_active_window();
        return;
    }

    PRINTF("focus node %X\n", n->client->window);

    n->client->urgent = false;

    pseudo_focus(d, n);
    stack(d, n);

    set_input_focus(n);

    if (focus_follows_pointer) {
        xcb_window_t win = XCB_NONE;
        query_pointer(&win, NULL);
        if (win != n->client->window)
            enable_motion_recorder();
        else
            disable_motion_recorder();
    }

    ewmh_update_active_window();
}

void update_current(void)
{
    focus_node(mon, mon->desk, mon->desk->focus);
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
    } else {
        node_t *b;
        node_t *g = p->parent;
        if (is_first_child(n)) {
            b = p->second_child;
            if (!n->vacant)
                unrotate_tree(b, n->birth_rotation);
        } else {
            b = p->first_child;
            if (!n->vacant)
                unrotate_tree(b, n->birth_rotation);
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

        b->birth_rotation = p->birth_rotation;
        n->parent = NULL;
        free(p);

        if (n == d->focus)
            d->focus = history_get(d->history, 1);

        update_vacant_state(b->parent);
    }
    put_status();
}

void remove_node(desktop_t *d, node_t *n)
{
    if (n == NULL)
        return;

    PRINTF("remove node %X\n", n->client->window);

    unlink_node(d, n);
    history_remove(d->history, n);
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
    int br1 = n1->birth_rotation;
    int br2 = n2->birth_rotation;

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
    n1->birth_rotation = br2;
    n2->birth_rotation = br1;

    if (n1->vacant != n2->vacant) {
        update_vacant_state(n1->parent);
        update_vacant_state(n2->parent);
    }

    /* If we ever need to generalize: */
    /* if (d1 != d2) { */
    /*     if (d1->root == n1) */
    /*         d1->root = n2; */
    /*     if (d1->focus == n1) */
    /*         d1->focus = n2; */
    /*     if (d1->last_focus == n1) */
    /*         d1->last_focus = n2; */
    /*     if (d2->root == n2) */
    /*         d2->root = n1; */
    /*     if (d2->focus == n2) */
    /*         d2->focus = n1; */
    /*     if (d2->last_focus == n2) */
    /*         d2->last_focus = n1; */
    /* } */
}

void transfer_node(monitor_t *ms, desktop_t *ds, monitor_t *md, desktop_t *dd, node_t *n)
{
    if (n == NULL || dd == ds)
        return;

    PRINTF("transfer node %X\n", n->client->window);

    unlink_node(ds, n);
    history_remove(ds->history, n);
    insert_node(md, dd, n, dd->focus);
    ewmh_set_wm_desktop(n, dd);

    if (ds == ms->desk && dd != md->desk) {
        if (n == ds->focus)
            clear_input_focus();
        window_hide(n->client->window);
    }

    fit_monitor(md, n->client);

    if (ds != ms->desk && dd == md->desk)
        window_show(n->client->window);

    pseudo_focus(dd, n);

    if (md->desk == dd)
        stack(dd, n);

    arrange(ms, ds);
    arrange(md, dd);

    if (ds == ms->desk || dd == md->desk)
        update_current();
}

void transplant_node(monitor_t *m, desktop_t *d, node_t *n1, node_t *n2)
{
    bool was_focused = (d->focus == n1);
    unlink_node(d, n1);
    insert_node(m, d, n1, n2);
    if (was_focused)
        pseudo_focus(d, n1);
}

void select_monitor(monitor_t *m)
{
    if (mon == m)
        return;

    PRINTF("select monitor %s\n", m->name);

    last_mon = mon;
    mon = m;

    if (pointer_follows_monitor)
        center_pointer(m);

    ewmh_update_current_desktop();
    put_status();
}

monitor_t *nearest_monitor(monitor_t *m, direction_t dir, desktop_select_t sel)
{
    int dmin = INT_MAX;
    monitor_t *nearest = NULL;
    xcb_rectangle_t rect = m->rectangle;
    for (monitor_t *f = mon_head; f != NULL; f = f->next) {
        if (f == m)
            continue;
        if (!desktop_matches(f->desk, sel))
            continue;
        xcb_rectangle_t r = f->rectangle;
        if ((dir == DIR_LEFT && r.x < rect.x) ||
                (dir == DIR_RIGHT && r.x >= (rect.x + rect.width)) ||
                (dir == DIR_UP && r.y < rect.y) ||
                (dir == DIR_DOWN && r.y >= (rect.y + rect.height))) {
            int d = abs((r.x + r.width / 2) - (rect.x + rect.width / 2)) +
                abs((r.y + r.height / 2) - (rect.y + rect.height / 2));
            if (d < dmin) {
                dmin = d;
                nearest = f;
            }
        }
    }
    return nearest;
}

void select_desktop(monitor_t *m, desktop_t *d)
{
    select_monitor(m);

    if (d == mon->desk)
        return;

    PRINTF("select desktop %s\n", d->name);

    desktop_show(d);
    desktop_hide(mon->desk);

    mon->last_desk = mon->desk;
    mon->desk = d;

    ewmh_update_current_desktop();
    put_status();
}

monitor_t *closest_monitor(monitor_t *m, cycle_dir_t dir, desktop_select_t sel)
{
    monitor_t *f = (dir == CYCLE_PREV ? m->prev : m->next);
    if (f == NULL)
        f = (dir == CYCLE_PREV ? mon_tail : mon_head);

    while (f != m) {
        if (desktop_matches(f->desk, sel))
            return f;
        f = (dir == CYCLE_PREV ? m->prev : m->next);
        if (f == NULL)
            f = (dir == CYCLE_PREV ? mon_tail : mon_head);
    }

    return NULL;
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

node_t *closest_node(desktop_t *d, node_t *n, cycle_dir_t dir, client_select_t sel)
{
    if (n == NULL)
        return NULL;

    node_t *f = (dir == CYCLE_PREV ? prev_leaf(n, d->root) : next_leaf(n, d->root));
    if (f == NULL)
        f = (dir == CYCLE_PREV ? second_extrema(d->root) : first_extrema(d->root));

    while (f != n) {
        if (node_matches(n, f, sel))
            return f;
        f = (dir == CYCLE_PREV ? prev_leaf(f, d->root) : next_leaf(f, d->root));
        if (f == NULL)
            f = (dir == CYCLE_PREV ? second_extrema(d->root) : first_extrema(d->root));
    }
    return NULL;
}

void circulate_leaves(monitor_t *m, desktop_t *d, circulate_dir_t dir)
{
    if (d == NULL || d->root == NULL || is_leaf(d->root))
        return;
    node_t *p = d->focus->parent;
    bool focus_first_child = is_first_child(d->focus);
    if (dir == CIRCULATE_FORWARD)
        for (node_t *s = second_extrema(d->root), *f = prev_leaf(s, d->root); f != NULL; s = prev_leaf(f, d->root), f = prev_leaf(s, d->root))
            swap_nodes(f, s);
    else
        for (node_t *f = first_extrema(d->root), *s = next_leaf(f, d->root); s != NULL; f = next_leaf(s, d->root), s = next_leaf(f, d->root))
            swap_nodes(f, s);
    if (focus_first_child)
        focus_node(m, d, p->first_child);
    else
        focus_node(m, d, p->second_child);
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
