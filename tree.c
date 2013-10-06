#include <float.h>
#include <limits.h>
#include <math.h>
#include "bspwm.h"
#include "desktop.h"
#include "ewmh.h"
#include "history.h"
#include "monitor.h"
#include "query.h"
#include "settings.h"
#include "stack.h"
#include "tag.h"
#include "window.h"
#include "tree.h"

void arrange(monitor_t *m, desktop_t *d)
{
    if (d->root == NULL)
        return;

    PRINTF("arrange %s %s\n", m->name, d->name);

    xcb_rectangle_t rect = m->rectangle;
    int wg = (gapless_monocle && d->layout == LAYOUT_MONOCLE ? 0 : d->window_gap);
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

        if (is_floating(n->client) && n->client->border_width != d->border_width) {
            int ds = 2 * (d->border_width - n->client->border_width);
            n->client->floating_rectangle.width += ds;
            n->client->floating_rectangle.height += ds;
        }

        if ((borderless_monocle && is_tiled(n->client) && d->layout == LAYOUT_MONOCLE) ||
                n->client->fullscreen)
            n->client->border_width = 0;
        else
            n->client->border_width = d->border_width;

        xcb_rectangle_t r;
        if (!n->client->fullscreen) {
            if (!n->client->floating) {
                /* tiled clients */
                if (d->layout == LAYOUT_TILED)
                    r = rect;
                else if (d->layout == LAYOUT_MONOCLE)
                    r = root_rect;
                else
                    return;
                int wg = (gapless_monocle && d->layout == LAYOUT_MONOCLE ? 0 : d->window_gap);
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

    if (f == NULL)
        f = d->root;

    if (f == NULL) {
        d->root = n;
        if (is_visible(d, n))
            d->focus = n;
    } else {
        node_t *c = make_node();
        node_t *p = f->parent;
        if (p != NULL && f->split_mode == MODE_AUTOMATIC
                && (p->first_child->vacant || p->second_child->vacant)) {
            f = p;
            p = f->parent;
        }
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
    if (n->client->sticky)
        num_sticky++;
    put_status();
}

void pseudo_focus(desktop_t *d, node_t *n)
{
    d->focus = n;
    if (n != NULL)
        stack(n);
}

void focus_node(monitor_t *m, desktop_t *d, node_t *n)
{
    if (mon->desk != d || n == NULL)
        clear_input_focus();

    if (num_sticky > 0 && d != mon->desk) {
        node_t *a = first_extrema(mon->desk->root);
        sticky_still = false;
        while (a != NULL) {
            node_t *b = next_leaf(a, mon->desk->root);
            if (a->client->sticky)
                transfer_node(mon, mon->desk, a, m, d, d->focus);
            a = b;
        }
        sticky_still = true;
        if (n == NULL)
            n = d->focus;
    }

    if (n != NULL && d->focus != NULL && n != d->focus && d->focus->client->fullscreen) {
        set_fullscreen(d->focus, false);
        arrange(m, d);
    }

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

    focus_desktop(m, d);
    pseudo_focus(d, n);

    if (n == NULL) {
        history_add(m, d, NULL);
        ewmh_update_active_window();
        return;
    }

    PRINTF("focus node %X\n", n->client->window);

    n->client->urgent = false;

    if (!is_visible(d, n))
        tag_node(m, d, n, d, n->client->tags_field | d->tags_field);
    history_add(m, d, n);
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

client_t *make_client(xcb_window_t win)
{
    client_t *c = malloc(sizeof(client_t));
    snprintf(c->class_name, sizeof(c->class_name), "%s", MISSING_VALUE);
    c->border_width = BORDER_WIDTH;
    c->window = win;
    c->floating = c->transient = c->fullscreen = c->locked = c->sticky = c->urgent = false;
    c->icccm_focus = false;
    xcb_icccm_get_wm_protocols_reply_t protocols;
    if (xcb_icccm_get_wm_protocols_reply(dpy, xcb_icccm_get_wm_protocols(dpy, win, ewmh->WM_PROTOCOLS), &protocols, NULL) == 1) {
        if (has_proto(WM_TAKE_FOCUS, &protocols))
            c->icccm_focus = true;
        xcb_icccm_get_wm_protocols_reply_wipe(&protocols);
    }
    return c;
}

bool is_visible(desktop_t *d, node_t *n)
{
    return (d->tags_field & n->client->tags_field) != 0;
}

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

void change_split_ratio(node_t *n, value_change_t chg)
{
    n->split_ratio = pow(n->split_ratio,
            (chg == CHANGE_INCREASE ? (1 / growth_factor) : growth_factor));
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

node_t *brother_tree(node_t *n)
{
    if (n == NULL || n->parent == NULL)
        return NULL;
    if (is_first_child(n))
        return n->parent->second_child;
    else
        return n->parent->first_child;
}

node_t *closest_visible(desktop_t *d, node_t *n)
{
    if (n == NULL)
        return NULL;
    node_t *prev = prev_leaf(n, d->root);
    node_t *next = next_leaf(n, d->root);
    while (prev != NULL || next != NULL) {
        if (prev != NULL) {
            if (is_visible(d, prev))
                return prev;
            else
                prev = prev_leaf(prev, d->root);
        }
        if (next != NULL) {
            if (is_visible(d, next))
                return next;
            else
                next = next_leaf(next, d->root);
        }
    }
    return NULL;
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

node_t *next_visible_leaf(desktop_t *d, node_t *n, node_t *r)
{
    node_t *next = next_leaf(n, r);
    if (next == NULL || is_visible(d, next))
        return next;
    else
        return next_visible_leaf(d, next, r);
}

node_t *prev_visible_leaf(desktop_t *d, node_t *n, node_t *r)
{
    node_t *prev = prev_leaf(n, r);
    if (prev == NULL || is_visible(d, prev))
        return prev;
    else
        return prev_visible_leaf(d, prev, r);
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
        nearest = nearest_from_history(d, n, dir, sel);
    if (nearest == NULL)
        nearest = nearest_from_distance(d, n, dir, sel);
    return nearest;
}

node_t *nearest_from_history(desktop_t *d, node_t *n, direction_t dir, client_select_t sel)
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

        int rank = history_rank(d, a);
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
        if (a == n ||
                !is_visible(d, a) ||
                !node_matches(n, a, sel) ||
                is_tiled(a->client) != is_tiled(n->client) ||
                (is_tiled(a->client) && !is_adjacent(n, a, dir)))
            continue;

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
        if (!is_visible(d, f) || !is_tiled(f->client) || !node_matches(c, f, sel))
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
    if (n == NULL)
        return;

    if ((mov == MOVE_PUSH && (dir == DIR_RIGHT || dir == DIR_DOWN))
            || (mov == MOVE_PULL && (dir == DIR_LEFT || dir == DIR_UP)))
        change_split_ratio(n, CHANGE_INCREASE);
    else
        change_split_ratio(n, CHANGE_DECREASE);
}

void rotate_tree(node_t *n, int deg)
{
    if (n == NULL || is_leaf(n) || deg == 0)
        return;

    node_t *tmp;

    if ((deg == 90 && n->split_type == TYPE_HORIZONTAL)
            || (deg == 270 && n->split_type == TYPE_VERTICAL)
            || deg == 180) {
        tmp = n->first_child;
        n->first_child = n->second_child;
        n->second_child = tmp;
        n->split_ratio = 1.0 - n->split_ratio;
    }

    if (deg != 180) {
        if (n->split_type == TYPE_HORIZONTAL)
            n->split_type = TYPE_VERTICAL;
        else if (n->split_type == TYPE_VERTICAL)
            n->split_type = TYPE_HORIZONTAL;
    }

    rotate_tree(n->first_child, deg);
    rotate_tree(n->second_child, deg);
}

void rotate_brother(node_t *n)
{
    rotate_tree(brother_tree(n), n->birth_rotation);
}

void unrotate_tree(node_t *n, int rot)
{
    if (rot == 0)
        return;
    rotate_tree(n, 360 - rot);
}

void unrotate_brother(node_t *n)
{
    unrotate_tree(brother_tree(n), n->birth_rotation);
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
        if (n == d->focus) {
            d->focus = history_get_node(d, n);
            if (d->focus == NULL)
                d->focus = closest_visible(d, n);
        }

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
        update_vacant_state(b->parent);
    }
    if (n->client->sticky)
        num_sticky--;
    put_status();
}

void remove_node(desktop_t *d, node_t *n)
{
    if (n == NULL)
        return;

    PRINTF("remove node %X\n", n->client->window);

    unlink_node(d, n);
    history_remove(d, n);
    remove_stack_node(n);
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

bool swap_nodes(monitor_t *m1, desktop_t *d1, node_t *n1, monitor_t *m2, desktop_t *d2, node_t *n2)
{
    if (n1 == NULL || n2 == NULL || n1 == n2 || (d1 != d2 && (n1->client->sticky || n2->client->sticky)))
        return false;

    PRINTF("swap nodes %X %X", n1->client->window, n2->client->window);

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

    if (d1 != d2) {
        if (d1->root == n1)
            d1->root = n2;
        if (d1->focus == n1)
            d1->focus = n2;
        if (d2->root == n2)
            d2->root = n1;
        if (d2->focus == n2)
            d2->focus = n1;

        if (m1 != m2) {
            fit_monitor(m1, n2->client);
            fit_monitor(m2, n1->client);
        }

        ewmh_set_wm_desktop(n1, d2);
        ewmh_set_wm_desktop(n2, d1);
        history_swap_nodes(m1, d1, n1, m2, d2, n2);

        if (m1->desk != d1 && m2->desk == d2) {
            window_show(n1->client->window);
            window_hide(n2->client->window);
        } else if (m1->desk == d1 && m2->desk != d2) {
            window_hide(n1->client->window);
            window_show(n2->client->window);
        }

        tag_node(m1, d1, n2, d2, n2->client->tags_field);
        tag_node(m2, d2, n1, d1, n1->client->tags_field);

        update_input_focus();
    }

    return true;
}

bool transfer_node(monitor_t *ms, desktop_t *ds, node_t *ns, monitor_t *md, desktop_t *dd, node_t *nd)
{
    if (ns == NULL || ns == nd || (sticky_still && ns->client->sticky))
        return false;

    PRINTF("transfer node %X\n", ns->client->window);

    bool focused = (ns == mon->desk->focus);
    bool active = (ns == ds->focus);

    if (focused)
        clear_input_focus();

    unlink_node(ds, ns);
    insert_node(md, dd, ns, nd);

    if (md != ms)
        fit_monitor(md, ns->client);

    if (ds != dd) {
        ewmh_set_wm_desktop(ns, dd);
        if (!ns->client->sticky) {
            if (ds == ms->desk && dd != md->desk)
                window_hide(ns->client->window);
            else if (ds != ms->desk && dd == md->desk)
                window_show(ns->client->window);
        }
    }

    history_transfer_node(md, dd, ns);
    stack_under(ns);

    if (ds == dd) {
        if (focused)
            focus_node(md, dd, ns);
        else if (active)
            pseudo_focus(dd, ns);
    } else {
        if (focused)
            update_current();
        else if (ns == mon->desk->focus)
            update_input_focus();
    }

    tag_node(md, dd, ns, ds, ns->client->tags_field);
    arrange(ms, ds);
    if (ds != dd)
        arrange(md, dd);

    return true;
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
    if (d == NULL || d->root == NULL || d->focus == NULL || is_leaf(d->root))
        return;
    node_t *p = d->focus->parent;
    bool focus_first_child = is_first_child(d->focus);
    node_t *head, *tail;
    for (head = first_extrema(d->root); head != NULL && !is_visible(d, head); head = next_leaf(head, d->root))
        ;
    for (tail = second_extrema(d->root); tail != NULL && !is_visible(d, tail); tail = prev_leaf(tail, d->root))
        ;
    if (head == tail)
        return;
    if (dir == CIRCULATE_FORWARD)
        for (node_t *s = tail, *f = prev_visible_leaf(d, s, d->root); f != NULL; s = prev_visible_leaf(d, f, d->root), f = prev_visible_leaf(d, s, d->root))
            swap_nodes(m, d, f, m, d, s);
    else
        for (node_t *f = head, *s = next_visible_leaf(d, f, d->root); s != NULL; f = next_visible_leaf(d, s, d->root), s = next_visible_leaf(d, f, d->root))
            swap_nodes(m, d, f, m, d, s);
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
