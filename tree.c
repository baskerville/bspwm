#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "settings.h"
#include "helpers.h"
#include "utils.h"
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

void dump_tree(desktop_t *d, node_t *n, char *rsp, int depth)
{
    if (n == NULL)
        return;

    char line[MAXLEN];

    for (int i = 0; i < depth; i++)
        strcat(rsp, "  ");

    if (is_leaf(n))
        /* sprintf(line, "0x%X [%i %i %u %u]", n->client->window, n->rectangle.x, n->rectangle.y, n->rectangle.width, n->rectangle.height); */ 
        sprintf(line, "C %X (%s%s) [%i %i %u %u]", n->client->window, (n->client->floating ? "f" : "-"), (n->client->transient ? "t" : "-"), n->client->rectangle.x, n->client->rectangle.y, n->client->rectangle.width, n->client->rectangle.height); 
    else
        /* sprintf(line, "%s %.2f [%i %i %u %u]", (n->split_type == TYPE_HORIZONTAL ? "H" : "V"), n->split_ratio, n->rectangle.x, n->rectangle.y, n->rectangle.width, n->rectangle.height); */
        sprintf(line, "%s %.2f [%i %i %u %u]", (n->split_type == TYPE_HORIZONTAL ? "H" : "V"), n->split_ratio, n->rectangle.x, n->rectangle.y, n->rectangle.width, n->rectangle.height);

    strcat(rsp, line);

    if (n == d->focus)
        strcat(rsp, " *\n");
    else
        strcat(rsp, "\n");

    dump_tree(d, n->first_child, rsp, depth + 1);
    dump_tree(d, n->second_child, rsp, depth + 1);
}

void list_desktops(char *rsp)
{
    desktop_t *d = desk_head;

    while (d != NULL) {
        strcat(rsp, d->name);
        if (desk == d)
            strcat(rsp, " @\n");
        else
            strcat(rsp, "\n");
        dump_tree(d, d->root, rsp, 1);
        d = d->next;
    }
}

void update_root_dimensions(void)
{
    root_rect.x = left_padding + window_gap;
    root_rect.y = top_padding + window_gap;
    root_rect.width = screen_width - (left_padding + right_padding + window_gap);
    root_rect.height = screen_height - (top_padding + bottom_padding + window_gap);
}

void apply_layout(desktop_t *d, node_t *n, xcb_rectangle_t rect)
{
    if (d == NULL || n == NULL)
        return;

    n->rectangle = rect;

    if (is_leaf(n)) {
        if (n->client->fullscreen)
            return;
        xcb_rectangle_t r;
        if (is_tiled(n->client)) {
            if (d->layout == LAYOUT_TILED)
                r = rect;
            else if (d->layout == LAYOUT_MONOCLE)
                r = root_rect;
            int bleed = window_gap + 2 * border_width;
            r.width = (bleed < r.width ? r.width - bleed : MIN_WIDTH);
            r.height = (bleed < r.height ? r.height - bleed : MIN_HEIGHT);
        } else {
            r = n->client->rectangle;
        }

        r.width = MAX(r.width, MIN_WIDTH);
        r.height = MAX(r.height, MIN_HEIGHT);

        window_move_resize(n->client->window, r.x, r.y, r.width, r.height);
        window_border_width(n->client->window, border_width);
        draw_triple_border(n, (n == d->focus ? active_border_color_pxl : normal_border_color_pxl));

        if (d->layout == LAYOUT_MONOCLE && n == d->focus)
            window_raise(n->client->window);

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

        apply_layout(d, n->first_child, first_rect);
        apply_layout(d, n->second_child, second_rect);
    }
}

void insert_node(desktop_t *d, node_t *n)
{
    if (d == NULL || n == NULL)
        return;

    PUTS("insert node\n");

    node_t *focus = d->focus;

    if (focus == NULL) {
        d->root = n;
    } else {
        node_t *dad = make_node();
        node_t *fopar = focus->parent;
        n->parent = dad;
        switch (split_mode) {
            case MODE_AUTOMATIC:
                if (fopar == NULL) {
                    dad->first_child = n;
                    dad->second_child = focus;
                    dad->split_type = TYPE_VERTICAL;
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
    }

    num_clients++;
    ewmh_update_client_list();
}

void focus_node(desktop_t *d, node_t *n, bool is_mapped)
{
    if (d == NULL || n == NULL || desk->focus == n)
        return;

    if (desk->focus != NULL && desk->focus->client->fullscreen)
        return;

    PRINTF("focus_node %x\n", n->client->window);

    split_mode = MODE_AUTOMATIC;

    select_desktop(d);

    if (is_mapped) {
        if (d->focus != n) {
            draw_triple_border(d->focus, normal_border_color_pxl);
            draw_triple_border(n, active_border_color_pxl);
        }
        xcb_set_input_focus(dpy, XCB_INPUT_FOCUS_POINTER_ROOT, n->client->window, XCB_CURRENT_TIME);
    }

    if (!is_tiled(n->client) || d->layout == LAYOUT_MONOCLE)
        window_raise(n->client->window);

    d->last_focus = d->focus;
    d->focus = n;

    ewmh_update_active_window();
}

void unlink_node(desktop_t *d, node_t *n)
{
    if (d == NULL || n == NULL)
        return;

    PUTS("unlink node\n");

    node_t *p = n->parent;

    if (p == NULL) {
        d->root = NULL;
        d->focus = NULL;
        d->last_focus = NULL;
    } else {
        node_t *b;
        node_t *g = p->parent;
        if (is_first_child(n))
            b = p->second_child;
        else
            b = p->first_child;
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

        PUTS("unlink: parent is free\n");

        if (n == d->focus) {
            if (d->last_focus != NULL && d->last_focus != n)
                d->focus = d->last_focus;
            else
                d->focus = (is_first_child(b) ? second_extrema(b) : first_extrema(b));
            d->last_focus = NULL;
        }
    }

}

void remove_node(desktop_t *d, node_t *n)
{
    if (d == NULL || n == NULL)
        return;

    PUTS("remove node\n");

    unlink_node(d, n);
    free(n->client);
    free(n);

    num_clients--;
    ewmh_update_client_list();

    if (desk == d && d->focus != NULL)
        xcb_set_input_focus(dpy, XCB_INPUT_FOCUS_POINTER_ROOT, d->focus->client->window, XCB_CURRENT_TIME);
}

void swap_nodes(node_t *n1, node_t *n2)
{
    if (n1 == NULL || n2 == NULL || n1 == n2)
        return;

    PUTS("swap nodes\n");

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
}

void transfer_node(desktop_t *ds, desktop_t *dd, node_t *n)
{
    if (n == NULL || ds == NULL || dd == NULL || dd == ds)
        return;

    PUTS("transfer node\n");

    unlink_node(ds, n);
    insert_node(dd, n);
}

void select_desktop(desktop_t *d)
{
    if (d == NULL || d == desk)
        return;

    PUTS("select desktop\n");

    if (d->focus != NULL)
        xcb_map_window(dpy, d->focus->client->window);

    node_t *n = first_extrema(d->root);

    while (n != NULL) {
        if (n != d->focus)
            xcb_map_window(dpy, n->client->window);
        n = next_leaf(n);
    }

    n = first_extrema(desk->root);
    while (n != NULL) {
        if (n != desk->focus)
            xcb_unmap_window(dpy, n->client->window);
        n = next_leaf(n);
    }

    if (desk->focus != NULL)
        xcb_unmap_window(dpy, desk->focus->client->window);

    last_desk = desk;
    desk = d;

    if (d->focus != NULL)
        xcb_set_input_focus(dpy, XCB_INPUT_FOCUS_POINTER_ROOT, d->focus->client->window, XCB_CURRENT_TIME);

    ewmh_update_current_desktop();
}

void cycle_leaf(desktop_t *d, node_t *n, cycle_dir_t dir, skip_client_t skip)
{
    if (n == NULL)
        return;

    PUTS("cycle leaf\n");

    node_t *f = (dir == DIR_PREV ? prev_leaf(n) : next_leaf(n));

    while (f != NULL) {
        bool tiled = is_tiled(f->client);
        if (skip == SKIP_NONE || (skip == SKIP_TILED && !tiled) || (skip == SKIP_FLOATING && tiled)) {
            focus_node(d, f, true);
            return;
        }
        f = (dir == DIR_PREV ? prev_leaf(f) : next_leaf(f));
    }
}

void toggle_floating(node_t *n)
{
    if (n == NULL || n->client->transient)
        return;

    PUTS("toggle floating\n");

    client_t *c = n->client;
    c->floating = !c->floating;
    n->vacant = !n->vacant;
    update_vacant_state(n->parent);
    if (c->floating)
        window_raise(c->window);
}

void update_vacant_state(node_t *n)
{
    if (n == NULL)
        return;

    PUTS("update vacant state\n");

    /* n is not a leaf */
    node_t *p = n;

    while (p != NULL) {
        p->vacant = (p->first_child->vacant && p->second_child->vacant);
        p = p->parent;
    }
}

desktop_t *find_desktop(char *name)
{
    desktop_t *d = desk_head;
    while (d != NULL) {
        if (strcmp(d->name, name) == 0)
            return d;
        d = d->next;
    }
    return NULL;
}

void add_desktop(char *name)
{
    desktop_t *d = make_desktop(name);
    desk_tail->next = d;
    d->prev = desk_tail;
    desk_tail = d;
    num_desktops++;
    ewmh_update_number_of_desktops();
    ewmh_update_desktop_names();
}

void alternate_desktop(void)
{
    if (last_desk == NULL)
        return;
    desktop_t *tmp = desk;
    desk = last_desk;
    last_desk = tmp;
    select_desktop(desk);
}
