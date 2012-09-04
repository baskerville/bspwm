#include <stdio.h>
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
    node_t *tmp;
    if (n == NULL)
        return;
    rotate_tree(n->first_child, rot);
    rotate_tree(n->second_child, rot);
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
}

void dump_tree(node_t *n, char *rsp, int depth)
{
    int i;

    if (n == NULL)
        return;

    for (i = 0; i < depth; i++)
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
    desktop_t *d = desk_head;
    while (d != NULL) {
        d->root->rectangle = (xcb_rectangle_t) {left_padding, top_padding, screen_width - (left_padding + right_padding), screen_height - (top_padding + bottom_padding)};
        d = d->next;
    }
}

void apply_layout(desktop_t *d, node_t *n)
{
    if (n == NULL)
        return;
    if (is_leaf(n)) {
        switch (desk->layout) {
            case LAYOUT_MONOCLE:
                xcb_configure_window(dpy, n->client->window, XCB_MOVE_RESIZE, (uint32_t *) &d->root->rectangle);
                break;
            case LAYOUT_TILED:
                xcb_configure_window(dpy, n->client->window, XCB_MOVE_RESIZE, (uint32_t *) &n->rectangle);
                break;
        }
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


