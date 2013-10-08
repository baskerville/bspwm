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

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "bspwm.h"
#include "settings.h"

void run_config(void)
{
    if (fork() == 0) {
        if (dpy != NULL)
            close(xcb_get_file_descriptor(dpy));
        if (fork() == 0) {
            setsid();
            execl(config_path, config_path, NULL);
            err("Couldn't execute the configuration file.\n");
        }
        exit(EXIT_SUCCESS);
    }

    wait(NULL);
}

void load_settings(void)
{
    snprintf(normal_border_color, sizeof(normal_border_color), "%s", NORMAL_BORDER_COLOR);
    snprintf(focused_border_color, sizeof(focused_border_color), "%s", FOCUSED_BORDER_COLOR);
    snprintf(active_border_color, sizeof(active_border_color), "%s", ACTIVE_BORDER_COLOR);
    snprintf(presel_border_color, sizeof(presel_border_color), "%s", PRESEL_BORDER_COLOR);
    snprintf(focused_locked_border_color, sizeof(focused_locked_border_color), "%s", FOCUSED_LOCKED_BORDER_COLOR);
    snprintf(active_locked_border_color, sizeof(active_locked_border_color), "%s", ACTIVE_LOCKED_BORDER_COLOR);
    snprintf(normal_locked_border_color, sizeof(normal_locked_border_color), "%s", NORMAL_LOCKED_BORDER_COLOR);
    snprintf(focused_sticky_border_color, sizeof(focused_sticky_border_color), "%s", FOCUSED_STICKY_BORDER_COLOR);
    snprintf(active_sticky_border_color, sizeof(active_sticky_border_color), "%s", ACTIVE_STICKY_BORDER_COLOR);
    snprintf(normal_sticky_border_color, sizeof(normal_sticky_border_color), "%s", NORMAL_STICKY_BORDER_COLOR);
    snprintf(urgent_border_color, sizeof(urgent_border_color), "%s", URGENT_BORDER_COLOR);

    split_ratio = SPLIT_RATIO;
    growth_factor = GROWTH_FACTOR;

    borderless_monocle = BORDERLESS_MONOCLE;
    gapless_monocle = GAPLESS_MONOCLE;
    focus_follows_pointer = FOCUS_FOLLOWS_POINTER;
    pointer_follows_monitor = POINTER_FOLLOWS_MONITOR;
    apply_floating_atom = APPLY_FLOATING_ATOM;
    auto_alternate = AUTO_ALTERNATE;
    auto_cancel = AUTO_CANCEL;
    history_aware_focus = HISTORY_AWARE_FOCUS;
    honor_ewmh_focus = HONOR_EWMH_FOCUS;
}
