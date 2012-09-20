#include <string.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_ewmh.h>
#include "types.h"
#include "bspwm.h"
#include "ewmh.h"
#include "rules.h"

rule_t *next_match(rule_t *rule, xcb_window_t win)
{
    if (rule == NULL)
        return NULL;
    rule_t *r = rule;
    rule_t *found = NULL;
    while (r != NULL && found == NULL) {
        if (is_match(r, win))
            found = r;
        r = r->next;
    }
    return found;
}

bool is_match(rule_t *r, xcb_window_t win)
{
    xcb_icccm_get_wm_class_reply_t reply; 
    if (xcb_icccm_get_wm_class_reply(dpy, xcb_icccm_get_wm_class(dpy, win), &reply, NULL) == 1
            && (strcmp(reply.class_name, r->cause.name) == 0
                || strcmp(reply.instance_name, r->cause.name) == 0)) {
        xcb_icccm_get_wm_class_reply_wipe(&reply);
        return true;
    }
    return false;
}

void handle_rules(xcb_window_t win, bool *floating, bool *transient, bool *fullscreen, bool *takes_focus)
{
    xcb_ewmh_get_atoms_reply_t win_type;

    if (xcb_ewmh_get_wm_window_type_reply(ewmh, xcb_ewmh_get_wm_window_type(ewmh, win), &win_type, NULL) == 1) {
        for (unsigned int i = 0; i < win_type.atoms_len; i++) {
            xcb_atom_t a = win_type.atoms[i];
            if (a == ewmh->_NET_WM_WINDOW_TYPE_TOOLBAR
                    || a == ewmh->_NET_WM_WINDOW_TYPE_UTILITY) {
                *takes_focus = false;
            } else if (a == ewmh->_NET_WM_WINDOW_TYPE_DIALOG) {
                *floating = true;
            }
        }
    }

    xcb_ewmh_get_atoms_reply_t win_state;

    if (xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, win), &win_state, NULL) == 1) {
        for (unsigned int i = 0; i < win_state.atoms_len; i++) {
            xcb_atom_t a = win_state.atoms[i];
            if (a == ewmh->_NET_WM_STATE_FULLSCREEN) {
                *fullscreen = true;
            }
        }
    }

    /* xcb_ewmh_get_atoms_reply_wipe(&win_type); */

    xcb_window_t transient_for = XCB_NONE;
    xcb_icccm_get_wm_transient_for_reply(dpy, xcb_icccm_get_wm_transient_for(dpy, win), &transient_for, NULL);
    *transient = (transient_for == XCB_NONE ? false : true);
    if (*transient)
        *floating = true;

    rule_t *rule = rule_head;

    while (rule != NULL) {
        if (is_match(rule, win)) {
            if (rule->effect.floating) {
                *floating = true;
            }
        }
        rule = rule->next;
    }
}
