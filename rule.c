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

#include <stdio.h>
#include <string.h>
#include "bspwm.h"
#include "ewmh.h"
#include "window.h"
#include "messages.h"
#include "query.h"
#include "tag.h"
#include "rule.h"

rule_t *make_rule(void)
{
    rule_t *r = malloc(sizeof(rule_t));
    r->effect.floating = false;
    r->effect.fullscreen = false;
    r->effect.locked = false;
    r->effect.sticky = false;
    r->effect.follow = false;
    r->effect.focus = false;
    r->effect.frame = false;
    r->effect.private = false;
    r->effect.unmanage = false;
    r->one_shot = false;
    r->effect.desc[0] = '\0';
    r->effect.tags[0] = '\0';
    r->prev = NULL;
    r->next = NULL;
    return r;
}

void add_rule(rule_t *r)
{
    if (rule_head == NULL) {
        rule_head = rule_tail = r;
    } else {
        rule_tail->next = r;
        r->prev = rule_tail;
        rule_tail = r;
    }
}

void remove_rule(rule_t *r)
{
    if (r == NULL)
        return;
    rule_t *prev = r->prev;
    rule_t *next = r->next;
    if (prev != NULL)
        prev->next = next;
    if (next != NULL)
        next->prev = prev;
    if (r == rule_head)
        rule_head = next;
    if (r == rule_tail)
        rule_tail = prev;
    free(r);
}

void remove_rule_by_name(char *name)
{
    rule_t *r = rule_head;
    while (r != NULL) {
        rule_t *next = r->next;
        if (streq(r->cause.name, name))
            remove_rule(r);
        r = next;
    }
}

bool remove_rule_by_index(int idx)
{
    for (rule_t *r = rule_head; r != NULL; r = r->next, idx--)
        if (idx == 0) {
            remove_rule(r);
            return true;
        }
    return false;
}

bool is_match(rule_t *r, xcb_window_t win)
{
    xcb_icccm_get_wm_class_reply_t reply;
    int8_t success = 0;
    if (streq(r->cause.name, MATCH_ALL) ||
            ((success = xcb_icccm_get_wm_class_reply(dpy, xcb_icccm_get_wm_class(dpy, win), &reply, NULL)) == 1
            && (streq(reply.class_name, r->cause.name)
                || streq(reply.instance_name, r->cause.name)))) {
        if (success == 1)
            xcb_icccm_get_wm_class_reply_wipe(&reply);
        return true;
    }
    return false;
}

void handle_rules(xcb_window_t win, monitor_t **m, desktop_t **d, unsigned int *tags_field, bool *floating, bool *fullscreen, bool *locked, bool *sticky, bool *follow, bool *transient, bool *takes_focus, bool *frame, bool *private, bool *manage)
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
            } else if (a == ewmh->_NET_WM_WINDOW_TYPE_DOCK || a == ewmh->_NET_WM_WINDOW_TYPE_DESKTOP || a == ewmh->_NET_WM_WINDOW_TYPE_NOTIFICATION) {
                *manage = false;
                if (a == ewmh->_NET_WM_WINDOW_TYPE_DESKTOP)
                    window_lower(win);
            }
        }
        xcb_ewmh_get_atoms_reply_wipe(&win_type);
    }

    xcb_size_hints_t size_hints;

    if (xcb_icccm_get_wm_normal_hints_reply(dpy, xcb_icccm_get_wm_normal_hints(dpy, win), &size_hints, NULL) == 1) {
        if (size_hints.min_width > 0 && size_hints.min_height > 0
                && size_hints.min_width == size_hints.max_width
                && size_hints.min_height == size_hints.max_height)
            *floating = true;
    }

    xcb_ewmh_get_atoms_reply_t win_state;

    if (xcb_ewmh_get_wm_state_reply(ewmh, xcb_ewmh_get_wm_state(ewmh, win), &win_state, NULL) == 1) {
        for (unsigned int i = 0; i < win_state.atoms_len; i++) {
            xcb_atom_t a = win_state.atoms[i];
            if (a == ewmh->_NET_WM_STATE_FULLSCREEN)
                *fullscreen = true;
            else if (a == ewmh->_NET_WM_STATE_STICKY)
                *sticky = true;
        }
        xcb_ewmh_get_atoms_reply_wipe(&win_state);
    }

    xcb_window_t transient_for = XCB_NONE;
    xcb_icccm_get_wm_transient_for_reply(dpy, xcb_icccm_get_wm_transient_for(dpy, win), &transient_for, NULL);
    *transient = (transient_for == XCB_NONE ? false : true);
    if (*transient)
        *floating = true;

    rule_t *rule = rule_head;

    while (rule != NULL) {
        if (is_match(rule, win)) {
            rule_effect_t efc = rule->effect;
            if (efc.floating)
                *floating = true;
            if (efc.fullscreen)
                *fullscreen = true;
            if (efc.locked)
                *locked = true;
            if (efc.sticky)
                *sticky = true;
            if (efc.follow)
                *follow = true;
            if (efc.focus)
                *takes_focus = true;
            if (efc.frame)
                *frame = true;
            if (efc.private)
                *private = private;
            if (efc.unmanage)
                *manage = false;
            if (efc.desc[0] != '\0') {
                coordinates_t ref = {*m, *d, NULL};
                coordinates_t loc;
                if (desktop_from_desc(efc.desc, &ref, &loc)) {
                    *m = loc.monitor;
                    *d = loc.desktop;
                }
            }
            if (efc.tags[0] != '\0') {
                char *name = strtok(efc.tags, LST_SEP);
                while (name != NULL) {
                    int idx;
                    tag_t *tag = NULL;
                    if (parse_index(name, &idx))
                        tag = get_tag_by_index(idx - 1);
                    else
                        tag = get_tag(name);
                    if (tag != NULL)
                        *tags_field |= tag->mask;
                    name = strtok(NULL, LST_SEP);
                }
            }
        }
        rule_t *next = rule->next;
        if (rule->one_shot)
            remove_rule(rule);
        rule = next;
    }

    if (*sticky) {
        *m = mon;
        *d = mon->desk;
    }
}

void list_rules(char *pattern, char *rsp)
{
    char line[MAXLEN];

    for (rule_t *r = rule_head; r != NULL; r = r->next) {
        if (pattern != NULL && !streq(pattern, r->cause.name))
            continue;
        snprintf(line, sizeof(line), "%s", r->cause.name);
        strncat(rsp, line, REMLEN(rsp));
        if (r->effect.floating)
            strncat(rsp, " --floating", REMLEN(rsp));
        if (r->effect.fullscreen)
            strncat(rsp, " --fullscreen", REMLEN(rsp));
        if (r->effect.locked)
            strncat(rsp, " --locked", REMLEN(rsp));
        if (r->effect.sticky)
            strncat(rsp, " --sticky", REMLEN(rsp));
        if (r->effect.follow)
            strncat(rsp, " --follow", REMLEN(rsp));
        if (r->effect.focus)
            strncat(rsp, " --focus", REMLEN(rsp));
        if (r->effect.frame)
            strncat(rsp, " --frame", REMLEN(rsp));
        if (r->effect.private)
            strncat(rsp, " --private", REMLEN(rsp));
        if (r->effect.unmanage)
            strncat(rsp, " --unmanage", REMLEN(rsp));
        if (r->one_shot)
            strncat(rsp, " --one-shot", REMLEN(rsp));
        if (r->effect.desc[0] != '\0') {
            snprintf(line, sizeof(line), " -d %s", r->effect.desc);
            strncat(rsp, line, REMLEN(rsp));
        }
        if (r->effect.tags[0] != '\0') {
            snprintf(line, sizeof(line), " --tags %s", r->effect.tags);
            strncat(rsp, line, REMLEN(rsp));
        }
        strncat(rsp, "\n", REMLEN(rsp));
    }
}
