#include <string.h>
#include <xcb/xcb_icccm.h>
#include "types.h"
#include "bspwm.h"
#include "rules.h"

rule_t *next_match(rule_t *r, xcb_window_t w)
{
    if (r == NULL)
        return NULL;
    rule_t *cur = (r == NULL ? rule_head : r->next);
    rule_t *found = NULL;
    while (cur != NULL && found == NULL) {
        if (is_match(cur, w))
            found = cur;
        cur = cur->next;
    }
    return found;
}

bool is_match(rule_t *r, xcb_window_t w)
{
    xcb_icccm_get_wm_class_reply_t wc; 
    if (xcb_icccm_get_wm_class_reply(dpy, xcb_icccm_get_wm_class(dpy, w), &wc, NULL) == 1) {
        if ((r->cause.class_name == NULL || strstr(wc.class_name, r->cause.class_name) != NULL) && (r->cause.instance_name == NULL || strstr(wc.instance_name, r->cause.instance_name) != NULL))
            return true;
    }
    return false;
}
