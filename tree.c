#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include "settings.h"
#include "helpers.h"
#include "utils.h"
#include "types.h"
#include "bspwm.h"
#include "tree.h"

bool is_leaf(node_t *n)
{
    return (n != NULL && n->first_child == NULL && n->second_child == NULL);
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

    if ((rot == ROTATE_CLOCK_WISE && n->split_type == TYPE_HORIZONTAL)
            || (rot == ROTATE_COUNTER_CW && n->split_type == TYPE_VERTICAL)
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

void dump_tree(node_t *n, char *rsp, int depth)
{
    if (n == NULL)
        return;

    for (int i = 0; i < depth; i++)
        sprintf(rsp, "%s", "  ");

    if (n->client == NULL)
        sprintf(rsp, "%s %.2f\n", (n->split_type == TYPE_HORIZONTAL ? "H" : "V"), n->split_ratio);
    else
        sprintf(rsp, "%X\n", n->client->window); 

    dump_tree(n->first_child, rsp, depth + 1);
    dump_tree(n->second_child, rsp, depth + 1);
}

void update_root_dimensions(void)
{
    xcb_rectangle_t root_rect = {
        left_padding + window_gap,
        top_padding + window_gap,
        screen_width - (left_padding + right_padding + window_gap),
        screen_height - (top_padding + bottom_padding + window_gap)
    };
    desktop_t *d = desk_head;
    while (d != NULL) {
        d->root->rectangle = root_rect;
        d = d->next;
    }
}

void apply_layout(desktop_t *d, node_t *n)
{
    if (n == NULL)
        return;
    if (is_leaf(n)) {
        uint32_t values[4];
        xcb_rectangle_t rect;
        if (d->layout == LAYOUT_TILED)
            rect = n->rectangle;
        else if (d->layout == LAYOUT_MONOCLE)
            rect = d->root->rectangle;
        values[0] = rect.x;
        values[1] = rect.y;
        values[2] = rect.width - window_gap;
        values[3] = rect.height - window_gap;
        xcb_configure_window(dpy, n->client->window, MOVE_RESIZE_MASK, values);
    } else {
        unsigned int fence;
        xcb_rectangle_t rect = n->rectangle;
        if (n->split_type == TYPE_VERTICAL) {
            fence = rect.width * n->split_ratio;
            n->first_child->rectangle = (xcb_rectangle_t) {rect.x, rect.y, fence, rect.height};
            n->second_child->rectangle = (xcb_rectangle_t) {rect.x + fence, rect.y, rect.width - fence, rect.height};

        } else if (n->split_type == TYPE_HORIZONTAL) {
            fence = rect.height * n->split_ratio;
            n->first_child->rectangle = (xcb_rectangle_t) {rect.x, rect.y, rect.width, fence};
            n->second_child->rectangle = (xcb_rectangle_t) {rect.x, rect.y + fence, rect.width, rect.height - fence};
        }
        apply_layout(d, n->first_child);
        apply_layout(d, n->second_child);
    }
}

void insert_node(desktop_t *d, node_t *n)
{
    if (d == NULL || n == NULL)
        return;

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
                        rotate_tree(fopar, ROTATE_CLOCK_WISE);
                    } else {
                        dad->first_child = fopar;
                        dad->second_child = n;
                        rotate_tree(fopar, ROTATE_COUNTER_CW);
                    }
                }
                break;
            case MODE_MANUAL:
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
}

void focus_node(desktop_t *d, node_t *n)
{
    if (d == NULL || n == NULL || d->focus == n)
        return;

    draw_triple_border(d->focus, normal_border_color_pxl);
    draw_triple_border(n, active_border_color_pxl);

    xcb_set_input_focus(dpy, XCB_INPUT_FOCUS_POINTER_ROOT, n->client->window, XCB_CURRENT_TIME);
}

void remove_node(desktop_t *d, node_t *n)
{
    if (d == NULL || n == NULL)
        return;

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
        free(p);
    }

    /* free(n->client); */
    /* free(n); */
}

void transfer_node(desktop_t *ds, desktop_t *dd, node_t *n)
{
    if (ds == NULL || dd == NULL || n == NULL)
        return;
    remove_node(ds, n);
    insert_node(dd, n);
}

void update_vacant_state(node_t *n)
{
    if (n == NULL)
        return;
    if (!is_leaf(n))
        n->vacant = (n->first_child->vacant && n->second_child->vacant);
    update_vacant_state(n->parent);
}

void select_desktop(desktop_t *d)
{
    if (d == NULL)
        return;
}
