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
#include <ctype.h>
#include "bspwm.h"
#include "ewmh.h"
#include "window.h"
#include "messages.h"
#include "settings.h"
#include "query.h"
#include "rule.h"

rule_consequence_t *make_rule_conquence(void)
{
    rule_consequence_t *r = calloc(1, sizeof(rule_consequence_t));
    r->manage = r->focus = true;
    return r;
}

void handle_rules(xcb_window_t win, monitor_t **m, desktop_t **d, rule_consequence_t *csq)
{
    xcb_size_hints_t size_hints;
    if (xcb_icccm_get_wm_normal_hints_reply(dpy, xcb_icccm_get_wm_normal_hints(dpy, win), &size_hints, NULL) == 1) {
        if (size_hints.min_width > 0 && size_hints.min_height > 0
                && size_hints.min_width == size_hints.max_width
                && size_hints.min_height == size_hints.max_height)
            csq->floating = true;
    }
    xcb_window_t transient_for = XCB_NONE;
    xcb_icccm_get_wm_transient_for_reply(dpy, xcb_icccm_get_wm_transient_for(dpy, win), &transient_for, NULL);
    if (transient_for != XCB_NONE)
        csq->transient = csq->floating = true;
    char cmd[MAXLEN];
    snprintf(cmd, sizeof(cmd), rule_command, win);
    FILE *results = popen(cmd, "r");
    if (results == NULL) {
        warn("Failed to run rule command: '%s'.", cmd);
        return;
    }
    char line[MAXLEN];
    bool v;
    while (fgets(line, sizeof(line), results) != NULL) {
        size_t i = strlen(line) - 1;
        while (i > 0 && isspace(line[i])) {
            line[i] = '\0';
            i--;
        }
        char *key = strtok(line, CSQ_BLK);
        char *value = strtok(NULL, CSQ_BLK);
        while (key != NULL && value != NULL) {
            PRINTF("%s = %s\n", key, value);
            if (streq("desktop", key)) {
                coordinates_t ref = {mon, mon->desk, NULL};
                coordinates_t trg = {NULL, NULL, NULL};
                if (desktop_from_desc(value, &ref, &trg)) {
                    *m = trg.monitor;
                    *d = trg.desktop;
                }
            } else if (streq("monitor", key)) {
                coordinates_t ref = {mon, NULL, NULL};
                coordinates_t trg = {NULL, NULL, NULL};
                if (monitor_from_desc(value, &ref, &trg)) {
                    *m = trg.monitor;
                    *d = trg.monitor->desk;
                }
            } else if (parse_bool(value, &v)) {
                if (streq("floating", key))
                    csq->floating = v;
#define SETCSQ(name) \
                else if (streq(#name, key)) \
                    csq->name = v;
                SETCSQ(fullscreen)
                SETCSQ(locked)
                SETCSQ(sticky)
                SETCSQ(private)
                SETCSQ(frame)
                SETCSQ(center)
                SETCSQ(lower)
                SETCSQ(follow)
                SETCSQ(manage)
                SETCSQ(focus)
#undef SETCSQ

            }
            key = strtok(NULL, CSQ_BLK);
            value = strtok(NULL, CSQ_BLK);
        }
    }
    if (csq->sticky) {
        *m = mon;
        *d = mon->desk;
    }
}
