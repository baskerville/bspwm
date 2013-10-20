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

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "bspwm.h"
#include "desktop.h"
#include "ewmh.h"
#include "history.h"
#include "query.h"
#include "settings.h"
#include "tree.h"
#include "window.h"
#include "monitor.h"

monitor_t *make_monitor(xcb_rectangle_t rect)
{
    monitor_t *m = malloc(sizeof(monitor_t));
    snprintf(m->name, sizeof(m->name), "%s%02d", DEFAULT_MON_NAME, ++monitor_uid);
    m->prev = m->next = NULL;
    m->desk = m->desk_head = m->desk_tail = NULL;
    m->rectangle = rect;
    m->top_padding = m->right_padding = m->bottom_padding = m->left_padding = 0;
    m->wired = true;
    m->num_sticky = 0;
    uint32_t mask = XCB_CW_EVENT_MASK;
    uint32_t values[] = {XCB_EVENT_MASK_ENTER_WINDOW};
    m->root = xcb_generate_id(dpy);
    xcb_create_window(dpy, XCB_COPY_FROM_PARENT, m->root, root, rect.x, rect.y, rect.width, rect.height, 0, XCB_WINDOW_CLASS_INPUT_ONLY, XCB_COPY_FROM_PARENT, mask, values);
    window_lower(m->root);
    if (focus_follows_pointer)
        window_show(m->root);
    return m;
}

monitor_t *find_monitor(char *name)
{
    for (monitor_t *m = mon_head; m != NULL; m = m->next)
        if (streq(m->name, name))
            return m;
    return NULL;
}

monitor_t *get_monitor_by_id(xcb_randr_output_t id)
{
    for (monitor_t *m = mon_head; m != NULL; m = m->next)
        if (m->id == id)
            return m;
    return NULL;
}

void fit_monitor(monitor_t *ms, monitor_t *md, node_t *n)
{
    if (frozen_pointer->action != ACTION_NONE)
        return;
    xcb_rectangle_t a = ms->rectangle;
    xcb_rectangle_t b = md->rectangle;
    xcb_rectangle_t *r = &n->client->floating_rectangle;
    if (ms != md) {
        double w = b.width;
        double h = b.height;
        int dx = (r->x - a.x) * (w / a.width);
        int dy = (r->y - a.y) * (h / a.height);
        r->x = b.x + dx;
        r->y = b.y + dy;
    }
    if (r->x <= b.x || (r->x + r->width) >= (b.x + b.width)) {
        if (r->width >= b.width)
            r->x = b.x;
        else
            r->x = b.x + (b.width - r->width) / 2;
    }
    if (r->y <= b.y || (r->y + r->height) >= (b.y + b.height)) {
        if (r->height >= b.height)
            r->y = b.y;
        else
            r->y = b.y + (b.height - r->height) / 2;
    }
}

void update_root(monitor_t *m)
{
    xcb_rectangle_t rect = m->rectangle;
    window_move_resize(m->root, rect.x, rect.y, rect.width, rect.height);
}

void focus_monitor(monitor_t *m)
{
    if (mon == m)
        return;

    PRINTF("focus monitor %s\n", m->name);

    mon = m;

    if (pointer_follows_monitor)
        center_pointer(m);

    ewmh_update_current_desktop();
    put_status();
}

monitor_t *add_monitor(xcb_rectangle_t rect)
{
    monitor_t *m = make_monitor(rect);
    if (mon == NULL) {
        mon = m;
        mon_head = m;
        mon_tail = m;
    } else {
        mon_tail->next = m;
        m->prev = mon_tail;
        mon_tail = m;
    }
    num_monitors++;
    return m;
}

void remove_monitor(monitor_t *m)
{
    PRINTF("remove monitor %s (0x%X)\n", m->name, m->id);

    while (m->desk_head != NULL)
        remove_desktop(m, m->desk_head);
    monitor_t *prev = m->prev;
    monitor_t *next = m->next;
    monitor_t *last_mon = history_get_monitor(m);
    if (prev != NULL)
        prev->next = next;
    if (next != NULL)
        next->prev = prev;
    if (mon_head == m)
        mon_head = next;
    if (mon_tail == m)
        mon_tail = prev;
    if (pri_mon == m)
        pri_mon = NULL;
    if (mon == m) {
        mon = (last_mon == NULL ? (prev == NULL ? next : prev) : last_mon);
        if (mon != NULL && mon->desk != NULL)
            update_current();
    }
    xcb_destroy_window(dpy, m->root);
    free(m);
    num_monitors--;
    put_status();
}

void merge_monitors(monitor_t *ms, monitor_t *md)
{
    PRINTF("merge %s into %s\n", ms->name, md->name);

    desktop_t *d = ms->desk_head;
    while (d != NULL) {
        desktop_t *next = d->next;
        if (d->root != NULL || strstr(d->name, DEFAULT_DESK_NAME) == NULL)
            transfer_desktop(ms, md, d);
        d = next;
    }
}

void swap_monitors(monitor_t *m1, monitor_t *m2)
{
    if (m1 == NULL || m2 == NULL || m1 == m2)
        return;

    if (mon_head == m1)
        mon_head = m2;
    else if (mon_head == m2)
        mon_head = m1;
    if (mon_tail == m1)
        mon_tail = m2;
    else if (mon_tail == m2)
        mon_tail = m1;

    monitor_t *p1 = m1->prev;
    monitor_t *n1 = m1->next;
    monitor_t *p2 = m2->prev;
    monitor_t *n2 = m2->next;

    if (p1 != NULL && p1 != m2)
        p1->next = m2;
    if (n1 != NULL && n1 != m2)
        n1->prev = m2;
    if (p2 != NULL && p2 != m1)
        p2->next = m1;
    if (n2 != NULL && n2 != m1)
        n2->prev = m1;

    m1->prev = p2 == m1 ? m2 : p2;
    m1->next = n2 == m1 ? m2 : n2;
    m2->prev = p1 == m2 ? m1 : p1;
    m2->next = n1 == m2 ? m1 : n1;

    ewmh_update_wm_desktops();
    ewmh_update_desktop_names();
    ewmh_update_current_desktop();
    put_status();
}

monitor_t *closest_monitor(monitor_t *m, cycle_dir_t dir, desktop_select_t sel)
{
    monitor_t *f = (dir == CYCLE_PREV ? m->prev : m->next);
    if (f == NULL)
        f = (dir == CYCLE_PREV ? mon_tail : mon_head);

    while (f != m) {
        coordinates_t loc = {m, m->desk, NULL};
        if (desktop_matches(&loc, &loc, sel))
            return f;
        f = (dir == CYCLE_PREV ? m->prev : m->next);
        if (f == NULL)
            f = (dir == CYCLE_PREV ? mon_tail : mon_head);
    }

    return NULL;
}

monitor_t *nearest_monitor(monitor_t *m, direction_t dir, desktop_select_t sel)
{
    int dmin = INT_MAX;
    monitor_t *nearest = NULL;
    xcb_rectangle_t rect = m->rectangle;
    for (monitor_t *f = mon_head; f != NULL; f = f->next) {
        if (f == m)
            continue;
        coordinates_t loc = {f, f->desk, NULL};
        if (!desktop_matches(&loc, &loc, sel))
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

bool import_monitors(void)
{
    PUTS("import monitors");
    xcb_randr_get_screen_resources_current_reply_t *sres = xcb_randr_get_screen_resources_current_reply(dpy, xcb_randr_get_screen_resources_current(dpy, root), NULL);
    if (sres == NULL)
        return false;

    monitor_t *m, *mm = NULL;

    int len = xcb_randr_get_screen_resources_current_outputs_length(sres);
    xcb_randr_output_t *outputs = xcb_randr_get_screen_resources_current_outputs(sres);

    xcb_randr_get_output_info_cookie_t cookies[len];
    for (int i = 0; i < len; i++)
        cookies[i] = xcb_randr_get_output_info(dpy, outputs[i], XCB_CURRENT_TIME);

    for (m = mon_head; m != NULL; m = m->next)
        m->wired = false;

    for (int i = 0; i < len; i++) {
        xcb_randr_get_output_info_reply_t *info = xcb_randr_get_output_info_reply(dpy, cookies[i], NULL);
        if (info != NULL) {
            if (info->crtc != XCB_NONE) {
                xcb_randr_get_crtc_info_reply_t *cir = xcb_randr_get_crtc_info_reply(dpy, xcb_randr_get_crtc_info(dpy, info->crtc, XCB_CURRENT_TIME), NULL);
                if (cir != NULL) {
                    xcb_rectangle_t rect = (xcb_rectangle_t) {cir->x, cir->y, cir->width, cir->height};
                    mm = get_monitor_by_id(outputs[i]);
                    if (mm != NULL) {
                        mm->rectangle = rect;
                        update_root(mm);
                        for (desktop_t *d = mm->desk_head; d != NULL; d = d->next)
                            for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root))
                                fit_monitor(mm, mm, n);
                        arrange(mm, mm->desk);
                        mm->wired = true;
                        PRINTF("update monitor %s (0x%X)\n", mm->name, mm->id);
                    } else {
                        mm = add_monitor(rect);
                        char *name = (char *)xcb_randr_get_output_info_name(info);
                        size_t name_len = MIN(sizeof(mm->name), (size_t)xcb_randr_get_output_info_name_length(info) + 1);
                        snprintf(mm->name, name_len, "%s", name);
                        mm->id = outputs[i];
                        PRINTF("add monitor %s (0x%X)\n", mm->name, mm->id);
                    }
                }
                free(cir);
            } else if (info->connection != XCB_RANDR_CONNECTION_DISCONNECTED) {
                m = get_monitor_by_id(outputs[i]);
                if (m != NULL)
                    m->wired = true;
            }
        }
        free(info);
    }

    /* initially focus the primary monitor and add the first desktop to it */
    xcb_randr_get_output_primary_reply_t *gpo = xcb_randr_get_output_primary_reply(dpy, xcb_randr_get_output_primary(dpy, root), NULL);
    if (gpo != NULL) {
        pri_mon = get_monitor_by_id(gpo->output);
        if (!running && pri_mon != NULL) {
            if (mon != pri_mon)
                mon = pri_mon;
            add_desktop(pri_mon, make_desktop(NULL));
            ewmh_update_current_desktop();
        }
    }
    free(gpo);

    /* handle overlapping monitors */
    m = mon_head;
    while (m != NULL) {
        monitor_t *next = m->next;
        if (m->wired) {
            for (monitor_t *mb = mon_head; mb != NULL; mb = mb->next)
                if (mb != m && mb->wired && (m->desk == NULL || mb->desk == NULL)
                        && contains(mb->rectangle, m->rectangle)) {
                    if (mm == m)
                        mm = mb;
                    merge_monitors(m, mb);
                    remove_monitor(m);
                    break;
                }
        }
        m = next;
    }

    /* merge and remove disconnected monitors */
    m = mon_head;
    while (m != NULL) {
        monitor_t *next = m->next;
        if (!m->wired) {
            merge_monitors(m, mm);
            remove_monitor(m);
        }
        m = next;
    }

    /* add one desktop to each new monitor */
    for (m = mon_head; m != NULL; m = m->next)
        if (m->desk == NULL && (running || pri_mon == NULL || m != pri_mon))
            add_desktop(m, make_desktop(NULL));

    free(sres);
    update_motion_recorder();
    return (num_monitors > 0);
}
