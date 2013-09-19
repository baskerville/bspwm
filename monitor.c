#include <stdlib.h>
#include <string.h>
#include "bspwm.h"
#include "tree.h"
#include "monitor.h"
#include "desktop.h"
#include "window.h"
#include "ewmh.h"

monitor_t *make_monitor(xcb_rectangle_t rect)
{
    monitor_t *m = malloc(sizeof(monitor_t));
    snprintf(m->name, sizeof(m->name), "%s%02d", DEFAULT_MON_NAME, ++monitor_uid);
    m->prev = m->next = NULL;
    m->desk = m->last_desk = NULL;
    m->rectangle = rect;
    m->top_padding = m->right_padding = m->bottom_padding = m->left_padding = 0;
    m->wired = true;
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
    while (m->desk_head != NULL)
        remove_desktop(m, m->desk_head);
    monitor_t *prev = m->prev;
    monitor_t *next = m->next;
    if (prev != NULL)
        prev->next = next;
    if (next != NULL)
        next->prev = prev;
    if (mon_head == m)
        mon_head = next;
    if (mon_tail == m)
        mon_tail = prev;
    if (last_mon == m)
        last_mon = NULL;
    if (pri_mon == m)
        pri_mon = NULL;
    if (mon == m) {
        monitor_t *mm = (last_mon == NULL ? (prev == NULL ? next : prev) : last_mon);
        if (mm != NULL) {
            focus_node(mm, mm->desk, mm->desk->focus);
            last_mon = NULL;
        } else {
            mon = NULL;
        }
    }
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

bool import_monitors(void)
{
    PUTS("import monitors");
    xcb_randr_get_screen_resources_current_reply_t *sres = xcb_randr_get_screen_resources_current_reply(dpy, xcb_randr_get_screen_resources_current(dpy, root), NULL);
    if (sres == NULL)
        return false;

    int len = xcb_randr_get_screen_resources_current_outputs_length(sres);
    xcb_randr_output_t *outputs = xcb_randr_get_screen_resources_current_outputs(sres);

    xcb_randr_get_output_info_cookie_t cookies[len];
    for (int i = 0; i < len; i++)
        cookies[i] = xcb_randr_get_output_info(dpy, outputs[i], XCB_CURRENT_TIME);

    for (monitor_t *m = mon_head; m != NULL; m = m->next)
        m->wired = false;

    monitor_t *mm = NULL;
    unsigned int num = 0;

    for (int i = 0; i < len; i++) {
        xcb_randr_get_output_info_reply_t *info = xcb_randr_get_output_info_reply(dpy, cookies[i], NULL);
        if (info != NULL && info->crtc != XCB_NONE) {

            xcb_randr_get_crtc_info_reply_t *cir = xcb_randr_get_crtc_info_reply(dpy, xcb_randr_get_crtc_info(dpy, info->crtc, XCB_CURRENT_TIME), NULL);
            if (cir != NULL) {
                xcb_rectangle_t rect = (xcb_rectangle_t) {cir->x, cir->y, cir->width, cir->height};
                mm = get_monitor_by_id(outputs[i]);
                if (mm != NULL) {
                    mm->rectangle = rect;
                    for (desktop_t *d = mm->desk_head; d != NULL; d = d->next)
                        for (node_t *n = first_extrema(d->root); n != NULL; n = next_leaf(n, d->root))
                            fit_monitor(mm, n->client);
                    arrange(mm, mm->desk);
                    mm->wired = true;
                    PRINTF("update monitor %s (0x%X)\n", mm->name, mm->id);
                } else {
                    mm = add_monitor(rect);
                    char *name = (char *)xcb_randr_get_output_info_name(info);
                    size_t name_len = MIN(sizeof(mm->name), (size_t)xcb_randr_get_output_info_name_length(info));
                    strncpy(mm->name, name, name_len);
                    mm->name[name_len] = '\0';
                    mm->id = outputs[i];
                    PRINTF("add monitor %s (0x%X)\n", mm->name, mm->id);
                }
                num++;
            }
            free(cir);
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

    /* add one desktop to each new monitor */
    for (monitor_t *m = mon_head; m != NULL; m = m->next)
        if (m->desk == NULL && (running || pri_mon == NULL || m != pri_mon))
            add_desktop(m, make_desktop(NULL));

    /* merge and remove disconnected monitors */
    monitor_t *m = mon_head;
    while (m != NULL) {
        monitor_t *next = m->next;
        if (!m->wired) {
            PRINTF("remove monitor %s (0x%X)\n", m->name, m->id);
            merge_monitors(m, mm);
            remove_monitor(m);
        }
        m = next;
    }

    free(sres);
    update_motion_recorder();
    return (num_monitors > 0);
}
